/// @file main.cpp
/// @brief Ponto de entrada do firmware: inicializa o sistema e cria as tarefas FreeRTOS.
///
/// Navegação e telemetria rodam em tarefas separadas e concorrentes, como no
/// diagrama de atividades. Os encoders físicos de quadratura são decodificados por
/// interrupção de hardware no ESP32-C3, alimentam a odometria (pose na malha) e são reportados na
/// telemetria. Uma terceira tarefa, a de rede, envia a telemetria ao Supabase sem nunca fazer a
/// navegação esperar (#238).

#include <Arduino.h>

// O log (`Serial`) tem que sair pela USB nativa: GPIO20/21, a UART0, são entradas do driver dos
// motores. Se esta opção faltar no `platformio.ini`, o log iria para os pinos do motor direito.
#if !defined(ARDUINO_USB_CDC_ON_BOOT) || !ARDUINO_USB_CDC_ON_BOOT
#error "Serial pela USB nativa: defina ARDUINO_USB_MODE=1 e ARDUINO_USB_CDC_ON_BOOT=1 no platformio.ini (Pinos.h)"
#endif

#include "ArmazenamentoMapa.h"
#include "Configuracao.h"
#include "ConfiguracaoRede.h"
#include "ContratoTelemetria.h"
#include "ControladorEnvio.h"
#include "ConversorDeslocamento.h"
#include "CredenciaisSupabase.h"
#include "DetectorObjetivo.h"
#include "EncoderEsp32.h"
#include "EnvioHttpSupabase.h"
#include "MedidorDeslocamento.h"
#include "MotoresDrv8833.h"
#include "Odometria.h"
#include "Pinos.h"
#include "PoliticaReenvio.h"
#include "ReconexaoWifi.h"
#include "RedeWifiEsp32.h"
#include "TelemetriaJson.h"
#include "TravaSecaoCritica.h"

// Credenciais do Wi-Fi e do Supabase: ficam em `include/credenciais.h`, que o Git ignora (o modelo é
// `include/credenciais.exemplo.h`). Sem o arquivo, o firmware compila e funciona sem enviar nada.
#if __has_include("credenciais.h")
#include "credenciais.h"
#define MICROMOUSE_TEM_CREDENCIAIS 1
#else
#define MICROMOUSE_TEM_CREDENCIAIS 0
#endif

/// Dono do mapa da corrida. Fica fora das tarefas para sobreviver ao reinício delas (HU-04).
/// A navegação recebe `armazenamentoMapa.mapa()` para registrar paredes; a telemetria, a versão
/// `const`, só para leitura. `iniciarCorrida()` é chamado quando o tamanho do labirinto for escolhido.
static micromouse::ArmazenamentoMapa armazenamentoMapa;

/// Encoders das rodas (roda direita invertida devido à montagem mecânica espelhada). Na Rev 2.0 os
/// pinos estão reservados, mas os encoders ainda não estão instalados.
static micromouse::EncoderEsp32 encoderEsq(micromouse::pinos::ENCODER_ESQ_A,
                                           micromouse::pinos::ENCODER_ESQ_B,
                                           false);
static micromouse::EncoderEsp32 encoderDir(micromouse::pinos::ENCODER_DIR_A,
                                           micromouse::pinos::ENCODER_DIR_B,
                                           true);

static micromouse::ConversorDeslocamento conversorDeslocamento;
static micromouse::MedidorDeslocamento medidorDeslocamento(encoderEsq, encoderDir, conversorDeslocamento);

/// Pose (célula e direção) estimada pela odometria diferencial. A tarefa de navegação a atualiza a
/// cada 10 ms e a telemetria a lê, então todo acesso passa pela seção crítica `travaOdometria`.
static micromouse::Odometria odometria;

/// Detector de chegada à célula objetivo.
/// NOTA PROVISÓRIA (#189): Inicializado provisoriamente em DimensaoMapa::Labirinto4x4 para validação
/// em bancada. Em labirintos 8x4 ou 12x4, a chegada não seria detectada com essa dimensão fixa.
/// A dimensão deverá ser configurada dinamicamente via comando/configuração de corrida quando a seleção
/// de tamanho for recebida pelo robô.
/// Além disso, esta instância estática coexiste temporariamente com Navegacao::detector_ e será unificada
/// assim que o ciclo de navegação assumir o controle do laço de percepção/decisão.
static micromouse::DetectorObjetivo detectorObjetivo(micromouse::DimensaoMapa::Labirinto4x4);
static portMUX_TYPE travaOdometria = portMUX_INITIALIZER_UNLOCKED;

/// Motores de tração pelo DRV8833 (IN1/IN2 no motor esquerdo, IN3/IN4 no direito). O direito fica
/// invertido, como o encoder, pela montagem espelhada; confirme na bancada que
/// `definirPotencia({300, 300})` faz as duas rodas andarem para a frente. O `ControleMovimento`
/// (#191) entra na tarefa de navegação junto com os sensores de distância.
static micromouse::MotoresDrv8833 motores(micromouse::pinos::MOTOR_ESQ_IN1,
                                          micromouse::pinos::MOTOR_ESQ_IN2,
                                          micromouse::pinos::MOTOR_DIR_IN3,
                                          micromouse::pinos::MOTOR_DIR_IN4,
                                          false,
                                          true);

// ---------------------------------------------------------------- envio da telemetria (#238, #236)

/// Credenciais do envio, lidas de `include/credenciais.h` (vazias sem o arquivo).
static micromouse::CredenciaisSupabase lerCredenciais() {
  micromouse::CredenciaisSupabase c;
#if MICROMOUSE_TEM_CREDENCIAIS
  c.ssidWifi = CREDENCIAL_WIFI_SSID;
  c.senhaWifi = CREDENCIAL_WIFI_SENHA;
  c.urlProjeto = CREDENCIAL_SUPABASE_URL;
  c.chaveAnon = CREDENCIAL_SUPABASE_ANON;
  c.tokenDispositivo = CREDENCIAL_DEVICE_TOKEN;
  c.certificadoRaiz = CREDENCIAL_CERTIFICADO_RAIZ;
  c.semValidarCertificado = CREDENCIAL_SEM_VALIDAR_CERTIFICADO != 0;
#endif
  return c;
}
static const micromouse::CredenciaisSupabase credenciais = lerCredenciais();

/// Wi-Fi, cliente HTTPS e o buffer com reenvio. Só a tarefa de rede faz chamadas de rede; a coleta
/// (tarefa de telemetria) só chama `controladorEnvio.registrar()`, que nunca usa a rede.
static micromouse::RedeWifiEsp32 redeWifi(credenciais.ssidWifi, credenciais.senhaWifi);
static micromouse::ReconexaoWifi reconexaoWifi(redeWifi);
static micromouse::EnvioHttpSupabase clienteSupabase(credenciais);
static micromouse::TravaSecaoCritica travaBufferEnvio;
static micromouse::ControladorEnvio controladorEnvio(clienteSupabase, travaBufferEnvio);

/// O que a coleta e a tarefa de rede combinam sobre a corrida, protegido por `travaCorrida`.
struct EstadoCorridaEnvio {
  bool ativa = false;               ///< A corrida existe no servidor: a coleta pode registrar.
  uint32_t inicioMs = 0;            ///< `millis()` da criação da corrida.
  bool encerramentoPedido = false;  ///< A coleta registrou a última amostra (objetivo ou tempo).
  bool concluida = false;           ///< Chegou ao objetivo.
};
static EstadoCorridaEnvio estadoCorrida;
static portMUX_TYPE travaCorrida = portMUX_INITIALIZER_UNLOCKED;

static EstadoCorridaEnvio lerEstadoCorrida() {
  portENTER_CRITICAL(&travaCorrida);
  const EstadoCorridaEnvio copia = estadoCorrida;
  portEXIT_CRITICAL(&travaCorrida);
  return copia;
}

/// `true` se `agoraMs` já alcançou `alvoMs`, correto mesmo quando o contador de ms dá a volta.
static bool chegouAoInstante(uint32_t agoraMs, uint32_t alvoMs) {
  return static_cast<int32_t>(agoraMs - alvoMs) >= 0;
}

/// Coleta: chamada pela tarefa de telemetria a cada `PERIODO_TELEMETRIA_MS`. Guarda a amostra no
/// buffer, com o número de sequência dado agora (e que nunca muda num reenvio). A chegada ao objetivo
/// ou o fim do tempo (10 min) registram a última amostra e pedem o encerramento da corrida.
static void coletarAmostra(const micromouse::Pose& pose, bool chegouAoObjetivo) {
  const EstadoCorridaEnvio corrida = lerEstadoCorrida();
  if (!corrida.ativa || corrida.encerramentoPedido) {
    return;
  }
  const uint32_t decorridoMs = static_cast<uint32_t>(millis() - corrida.inicioMs);
  const bool acabouOTempo = decorridoMs >= micromouse::LIMITE_TEMPO_CORRIDA_MS;
  const bool ultima = chegouAoObjetivo || acabouOTempo;
  const micromouse::AmostraTelemetria amostra = micromouse::montarAmostra(
      pose, decorridoMs, 0,  // velocidade: 0 até a estimativa sem encoder (#380) entrar na telemetria
      ultima ? micromouse::EstadoCorrida::Parado : micromouse::EstadoCorrida::Explorando,
      chegouAoObjetivo ? micromouse::EventoTelemetria::ObjetivoAlcancado : micromouse::EventoTelemetria::Nenhum);
  controladorEnvio.registrar(amostra);
  if (ultima) {
    portENTER_CRITICAL(&travaCorrida);
    estadoCorrida.encerramentoPedido = true;
    estadoCorrida.concluida = chegouAoObjetivo;
    portEXIT_CRITICAL(&travaCorrida);
  }
}

/// Uma linha de log com os contadores do envio, para a bancada e o dashboard.
static void registrarContadoresEnvio() {
  const micromouse::ContadoresEnvio& c = controladorEnvio.contadores();
  Serial.printf(
      "{\"status\":\"envio\",\"ocupacao\":%u,\"perdidas\":%lu,\"confirmadas\":%lu,\"existentes\":%lu,"
      "\"descartadas\":%lu,\"tentativas\":%lu,\"falhas\":%lu,\"reenvios\":%lu,\"wifi_quedas\":%lu,"
      "\"pausado\":%s}\n",
      static_cast<unsigned>(controladorEnvio.ocupacao()), static_cast<unsigned long>(controladorEnvio.amostrasPerdidas()),
      static_cast<unsigned long>(c.amostrasConfirmadas), static_cast<unsigned long>(c.amostrasJaExistentes),
      static_cast<unsigned long>(c.amostrasDescartadas), static_cast<unsigned long>(c.tentativas),
      static_cast<unsigned long>(c.falhas), static_cast<unsigned long>(c.reenvios),
      static_cast<unsigned long>(reconexaoWifi.quedas()), controladorEnvio.pausado() ? "true" : "false");
}

/// Tarefa de rede: Wi-Fi, criação da corrida, envio dos lotes e encerramento.
///
/// Prioridade 1, a menor, com pilha maior por causa do TLS. As chamadas HTTPS bloqueiam esta tarefa
/// por até `TEMPO_LIMITE_REDE_MS`, nunca a navegação: a coleta só deixa a amostra no buffer.
static void tarefaRede(void*) {
  if (!micromouse::credenciaisValidas(credenciais)) {
    Serial.println("{\"status\":\"envio_desligado\",\"motivo\":\"sem_credenciais\"}");
    vTaskDelete(nullptr);
    return;
  }
  if (credenciais.semValidarCertificado) {
    Serial.println("{\"status\":\"aviso\",\"motivo\":\"https_sem_validar_certificado\"}");
  }
  redeWifi.iniciar();
  clienteSupabase.iniciar();

  micromouse::PoliticaReenvio esperaCriacao;  // 1 s, 2 s, 4 s... até 30 s entre tentativas de criar a corrida
  uint32_t proximaCriacaoMs = millis();
  uint32_t proximoLogMs = millis() + micromouse::PERIODO_LOG_ENVIO_MS;
  bool corridaCriada = false;
  bool avisouPausa = false;
  bool avisouEncerrada = false;

  for (;;) {
    const uint32_t agora = millis();
    if (reconexaoWifi.ciclo(agora)) {
      if (!corridaCriada) {
        if (chegouAoInstante(agora, proximaCriacaoMs)) {
          uint32_t corridaId = 0;
          const micromouse::ResultadoEnvio r =
              clienteSupabase.criarCorrida(micromouse::LABIRINTO_CORRIDA_PADRAO, corridaId);
          if (r.codigoHttp >= 200 && r.codigoHttp < 300) {
            controladorEnvio.iniciarCorrida(corridaId);
            corridaCriada = true;
            portENTER_CRITICAL(&travaCorrida);
            estadoCorrida.ativa = true;
            estadoCorrida.inicioMs = agora;
            portEXIT_CRITICAL(&travaCorrida);
            Serial.printf("{\"status\":\"corrida_criada\",\"corrida_id\":%lu}\n", static_cast<unsigned long>(corridaId));
          } else {
            proximaCriacaoMs = agora + esperaCriacao.registrarFalha();
            Serial.printf("{\"status\":\"falha_criar_corrida\",\"codigo\":%d}\n", r.codigoHttp);
          }
        }
      } else {
        const EstadoCorridaEnvio corrida = lerEstadoCorrida();
        if (corrida.encerramentoPedido && !controladorEnvio.encerramentoSolicitado()) {
          clienteSupabase.definirResultadoCorrida(corrida.concluida);
          controladorEnvio.solicitarEncerramento(agora);  // sai só depois de o buffer esvaziar
        }
        const micromouse::ResultadoCiclo r = controladorEnvio.ciclo(agora);
        if (r == micromouse::ResultadoCiclo::Pausado && !avisouPausa) {
          avisouPausa = true;  // 401/403: a chave ou o token estão errados; não adianta insistir
          Serial.println("{\"status\":\"envio_pausado\",\"motivo\":\"credenciais_recusadas\"}");
        }
        if (controladorEnvio.encerrada() && !avisouEncerrada) {
          avisouEncerrada = true;
          Serial.println("{\"status\":\"corrida_encerrada\"}");
        }
      }
    }
    if (chegouAoInstante(agora, proximoLogMs)) {
      proximoLogMs = agora + micromouse::PERIODO_LOG_ENVIO_MS;
      registrarContadoresEnvio();
    }
    vTaskDelay(pdMS_TO_TICKS(micromouse::PERIODO_CICLO_REDE_MS));
  }
}

/// Tarefa de navegação: ciclo percepção → decisão → atuação, a cada 10 ms.
///
/// Prioridade 2 (maior que a da telemetria), para que o envio de dados nunca
/// atrase o controle do robô.
static void tarefaNavegacao(void*) {
  for (;;) {
    bool alcancou = false;
    // Odometria e detecção de chegada executam na seção crítica travaOdometria para sincronizar com tarefaTelemetria.
    portENTER_CRITICAL(&travaOdometria);
    odometria.atualizar(encoderEsq, encoderDir);
    const micromouse::Pose pose = odometria.pose();
    const bool poseValida = odometria.posicaoValida();
    if (poseValida) {
      // Chegada detectada pela posição na malha: para os motores imediatamente e congela o mapa final (HU-07 / #189)
      alcancou = detectorObjetivo.processarChegada(pose, motores, &armazenamentoMapa);
    } else {
      alcancou = detectorObjetivo.objetivoAlcancado();
    }
    portEXIT_CRITICAL(&travaOdometria);

    if (alcancou) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    // Ciclo percepção, decisão e atuação: percepção, mapeamento, navegação e atuação entram aqui.
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/// Tarefa de telemetria: coleta e envia os dados a cada `PERIODO_TELEMETRIA_MS`.
///
/// Prioridade 1. Transmite as leituras dos encoders, pose da odometria e eventos pontuais em formato JSON
/// estruturado e aceita o comando 'Z' para zerar os encoders e voltar a odometria à pose atual.
static void tarefaTelemetria(void*) {
  static uint32_t sequencia = 0;

  for (;;) {
    // Leitura dos encoders
    const micromouse::DeslocamentoRodas d = medidorDeslocamento.medir();
    const int32_t medio = medidorDeslocamento.deslocamentoMedioMm();
    const int32_t pulsosEsq = encoderEsq.lerContagem();
    const int32_t pulsosDir = encoderDir.lerContagem();

    // Pose da odometria e evento pontual consumidos de forma atômica na seção crítica
    portENTER_CRITICAL(&travaOdometria);
    const micromouse::Pose pose = odometria.pose();
    const bool poseValida = odometria.posicaoValida();
    const bool dispararEvento = detectorObjetivo.consumirEventoChegada();
    portEXIT_CRITICAL(&travaOdometria);

    micromouse::PacoteTelemetria pacote;
    pacote.numeroSequencia = ++sequencia;
    pacote.pulsosEsq = pulsosEsq;
    pacote.pulsosDir = pulsosDir;
    pacote.esqMm = d.esquerdaMm;
    pacote.dirMm = d.direitaMm;
    pacote.medioMm = medio;
    pacote.pose = pose;
    pacote.poseValida = poseValida;
    pacote.velocidade = 0.0f;     // Provisório até inclusão de cálculo de velocidade instantânea
    pacote.tensaoBateria = 0.0f;  // Provisório até integração de leitura analógica da bateria
    pacote.evento = dispararEvento ? "objetivo_alcancado" : nullptr;

    // Envio ao Supabase: a amostra vai para o buffer; quem envia é a tarefa de rede.
    coletarAmostra(pose, dispararEvento);

    char buffer[256];
    if (micromouse::TelemetriaJson::serializar(pacote, buffer, sizeof(buffer)) > 0) {
      Serial.println(buffer);
    }

    // Comando serial para zerar medição (usado na bancada)
    while (Serial.available()) {
      const char c = static_cast<char>(Serial.read());
      if (c == 'Z' || c == 'z') {
        // Os encoders voltam a zero: a odometria mantém a pose e passa a contar a partir de zero.
        portENTER_CRITICAL(&travaOdometria);
        medidorDeslocamento.zerar();
        odometria.reiniciar(odometria.pose(), 0, 0);
        detectorObjetivo.resetar();
        portEXIT_CRITICAL(&travaOdometria);
        Serial.println("{\"status\":\"encoders_zerados\"}");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(micromouse::PERIODO_TELEMETRIA_MS));
  }
}

/// Inicializa o log serial pela USB nativa e os encoders, e cria as tarefas de navegação, telemetria
/// e rede.
void setup() {
  Serial.begin(115200);
  // Sem um computador conectado, a escrita na USB CDC esperaria o tempo limite a cada chamada e
  // atrasaria a telemetria. Com tempo zero, o dado é descartado se ninguém estiver lendo.
  Serial.setTxTimeoutMs(0);
  Serial.println("micromouse: iniciando");

  // Configuração provisória para bancada (#189): fixa Labirinto4x4.
  // Em 8x4 e 12x4 a corrida deve receber a dimensão por comando antes de iniciar.
  armazenamentoMapa.iniciarCorrida(micromouse::DimensaoMapa::Labirinto4x4);
  motores.iniciar();  // começa freado
  encoderEsq.iniciar();
  encoderDir.iniciar();

  xTaskCreate(tarefaNavegacao, "navegacao", 4096, nullptr, 2, nullptr);
  xTaskCreate(tarefaTelemetria, "telemetria", 4096, nullptr, 1, nullptr);
  xTaskCreate(tarefaRede, "rede", 10240, nullptr, 1, nullptr);
}

/// Laço principal do Arduino. Fica ocioso: o trabalho acontece nas tarefas.
void loop() {
  delay(1000);
}
