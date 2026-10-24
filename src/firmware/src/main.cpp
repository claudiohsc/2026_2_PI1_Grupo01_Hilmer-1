/// @file main.cpp
/// @brief Ponto de entrada do firmware: inicializa o sistema e cria as tarefas FreeRTOS.
///
/// Navegação e telemetria rodam em tarefas separadas e concorrentes, como no
/// diagrama de atividades. Os encoders físicos de quadratura são decodificados por
/// interrupção de hardware no ESP32-C3, alimentam a odometria (pose na malha) e são reportados na
/// telemetria. Os 3 sensores de distância VL53L0X são lidos pelo I²C na tarefa de navegação.
/// Enquanto os encoders não são instalados (`ENCODERS_INSTALADOS` em `ConfiguracaoDeslocamento.h`),
/// o deslocamento é estimado pela potência dos motores e corrigido pelos ToF (#380).

#include <Arduino.h>
#include <Wire.h>

// O log (`Serial`) tem que sair pela USB nativa: GPIO20/21, a UART0, são entradas do driver dos
// motores. Se esta opção faltar no `platformio.ini`, o log iria para os pinos do motor direito.
#if !defined(ARDUINO_USB_CDC_ON_BOOT) || !ARDUINO_USB_CDC_ON_BOOT
#error "Serial pela USB nativa: defina ARDUINO_USB_MODE=1 e ARDUINO_USB_CDC_ON_BOOT=1 no platformio.ini (Pinos.h)"
#endif

#include "ArmazenamentoMapa.h"
#include "Configuracao.h"
#include "ConfiguracaoDeslocamento.h"
#include "ConversorDeslocamento.h"
#include "DeslocamentoEstimado.h"
#include "DetectorObjetivo.h"
#include "EncoderEsp32.h"
#include "MedidorDeslocamento.h"
#include "MotoresDrv8833.h"
#include "Odometria.h"
#include "Pinos.h"
#include "TelemetriaJson.h"
#include "TofVl53l0x.h"

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

/// Motores de tração pelo DRV8833 (IN1/IN2 no motor esquerdo, IN3/IN4 no direito). O direito fica
/// invertido, como o encoder, pela montagem espelhada; confirme na bancada que
/// `definirPotencia({300, 300})` faz as duas rodas andarem para a frente.
static micromouse::MotoresDrv8833 motores(micromouse::pinos::MOTOR_ESQ_IN1,
                                          micromouse::pinos::MOTOR_ESQ_IN2,
                                          micromouse::pinos::MOTOR_DIR_IN3,
                                          micromouse::pinos::MOTOR_DIR_IN4,
                                          false,
                                          true);

/// Deslocamento sem encoder (#380): estima o avanço pela potência comandada e o corrige pelos ToF.
/// Quem acionar as rodas (o `ControleMovimento`, na #184) deve usar `motoresTracao`, para o
/// estimador saber a potência; com os encoders instalados, `motoresTracao` é o próprio driver.
///
/// A estimativa fica colada no driver, para ver só o que chega de fato às rodas. Os outros
/// decoradores envolvem `motoresTracao`, nesta ordem: `MotoresComTrava` (#50) →
/// `MotoresComProtecao` (#341) → `MotoresComEstimativa` → `MotoresDrv8833`. Assim um comando
/// recusado pela trava ou pela proteção chega ao estimador como freio.
static micromouse::DeslocamentoEstimado deslocamento(motores);
static micromouse::IMotores& motoresTracao =
    micromouse::ENCODERS_INSTALADOS ? static_cast<micromouse::IMotores&>(motores) : deslocamento.motores();

/// Encoders usados pela odometria e pela telemetria: os reais, se instalados; senão, os estimados.
/// A troca é só aqui: a `Odometria` e o `MedidorDeslocamento` não mudam.
static micromouse::IEncoder& rodaEsq =
    micromouse::ENCODERS_INSTALADOS ? static_cast<micromouse::IEncoder&>(encoderEsq) : deslocamento.encoderEsquerdo();
static micromouse::IEncoder& rodaDir =
    micromouse::ENCODERS_INSTALADOS ? static_cast<micromouse::IEncoder&>(encoderDir) : deslocamento.encoderDireito();

static micromouse::ConversorDeslocamento conversorDeslocamento;
static micromouse::MedidorDeslocamento medidorDeslocamento(rodaEsq, rodaDir, conversorDeslocamento);

/// Pose (célula e direção) estimada pela odometria diferencial. A tarefa de navegação a atualiza a
/// cada 10 ms e a telemetria a lê, então todo acesso passa pela seção crítica `travaOdometria`. A
/// mesma trava protege o estimador do deslocamento, que as duas tarefas também usam.
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

/// Sensores de distância VL53L0X (#344). Só a tarefa de navegação fala com eles pelo I²C; a
/// telemetria lê a cópia `ultimaLeituraTof`, protegida pela seção crítica `travaTof`.
static micromouse::TofVl53l0x tof(Wire,
                                  {micromouse::pinos::XSHUT_FRONTAL, micromouse::pinos::ENDERECO_TOF_FRONTAL},
                                  {micromouse::pinos::XSHUT_ESQ, micromouse::pinos::ENDERECO_TOF_ESQ},
                                  {micromouse::pinos::XSHUT_DIR, micromouse::pinos::ENDERECO_TOF_DIR});

/// Última leitura dos sensores de distância, copiada pela navegação para a telemetria.
struct LeituraTofTelemetria {
  micromouse::DistanciasLaterais distancias = {0, 0, 0};
  bool valida = false;
  uint32_t idadeMs = 0;
};
static LeituraTofTelemetria ultimaLeituraTof;
static portMUX_TYPE travaTof = portMUX_INITIALIZER_UNLOCKED;

/// Tarefa de navegação: ciclo percepção → decisão → atuação, a cada 10 ms.
///
/// Prioridade 2 (maior que a da telemetria), para que o envio de dados nunca
/// atrase o controle do robô.
static void tarefaNavegacao(void*) {
  // Instante da amostra mais antiga da última leitura usada: se não mudou, a leitura é a mesma.
  uint32_t instanteUltimaLeituraUsada = 0;
  for (;;) {
    // Sensores de distância: a leitura não espera medida nova, só junta as últimas amostras. Fica
    // antes da detecção do objetivo para a telemetria continuar recebendo o ToF depois da chegada.
    LeituraTofTelemetria leituraTof;
    leituraTof.valida = tof.lerDistanciasLaterais(leituraTof.distancias);
    leituraTof.idadeMs = tof.idadeUltimaLeituraMs();
    portENTER_CRITICAL(&travaTof);
    ultimaLeituraTof = leituraTof;
    portEXIT_CRITICAL(&travaTof);

    const uint32_t instanteLeitura = millis() - leituraTof.idadeMs;
    const bool leituraNova = leituraTof.valida && instanteLeitura != instanteUltimaLeituraUsada;
    if (leituraNova) {
      instanteUltimaLeituraUsada = instanteLeitura;
    }

    bool alcancou = false;
    // Odometria e detecção de chegada executam na seção crítica travaOdometria para sincronizar com
    // tarefaTelemetria. O intervalo curto (10 ms) mantém cada atualização com giro ou avanço, não os dois.
    portENTER_CRITICAL(&travaOdometria);
    if (micromouse::ENCODERS_INSTALADOS) {
      odometria.atualizar(rodaEsq, rodaDir);
    } else {
      // Estima o avanço dos 10 ms e corrige a posição pela leitura nova do ToF, se houver.
      deslocamento.ciclo(10, odometria, leituraNova ? &leituraTof.distancias : nullptr);
    }
    const micromouse::Pose pose = odometria.pose();
    // No modo estimado, uma pose sem correção recente não serve para declarar a chegada (#380).
    const bool poseValida = odometria.posicaoValida() &&
                            (micromouse::ENCODERS_INSTALADOS || deslocamento.poseConfiavel());
    if (poseValida) {
      // Chegada detectada pela posição na malha: para os motores imediatamente e congela o mapa
      // final (HU-07 / #189). Freia por `motoresTracao`, para o estimador saber que o robô parou.
      alcancou = detectorObjetivo.processarChegada(pose, motoresTracao, &armazenamentoMapa);
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
///
/// No modo sem encoder, `pose_valida` também fica `false` depois de o robô andar mais que
/// `DISTANCIA_MAXIMA_SEM_CORRECAO_MM` sem correção pelo ToF, e `deslocamento` diz qual fonte está
/// em uso (`"estimado"` ou `"encoder"`).
static void tarefaTelemetria(void*) {
  static uint32_t sequencia = 0;

  for (;;) {
    // Encoders (reais ou estimados) e pose: uma cópia feita na seção crítica, para tudo ser da
    // mesma atualização.
    portENTER_CRITICAL(&travaOdometria);
    const micromouse::DeslocamentoRodas d = medidorDeslocamento.medir();
    const int32_t medio = medidorDeslocamento.deslocamentoMedioMm();
    const int32_t pulsosEsq = rodaEsq.lerContagem();
    const int32_t pulsosDir = rodaDir.lerContagem();
    const micromouse::Pose pose = odometria.pose();
    const bool poseConfiavel = micromouse::ENCODERS_INSTALADOS || deslocamento.poseConfiavel();
    const bool poseValida = odometria.posicaoValida() && poseConfiavel;
    const uint32_t semCorrecaoMm = deslocamento.distanciaSemCorrecaoMm();
    const bool dispararEvento = detectorObjetivo.consumirEventoChegada();
    portEXIT_CRITICAL(&travaOdometria);

    portENTER_CRITICAL(&travaTof);
    const LeituraTofTelemetria leituraTof = ultimaLeituraTof;
    portEXIT_CRITICAL(&travaTof);

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
    pacote.tofFrenteMm = leituraTof.distancias.frenteMm;
    pacote.tofEsqMm = leituraTof.distancias.esquerdaMm;
    pacote.tofDirMm = leituraTof.distancias.direitaMm;
    pacote.tofValida = leituraTof.valida;
    pacote.tofIdadeMs = leituraTof.idadeMs;
    pacote.modoDeslocamento = micromouse::ENCODERS_INSTALADOS ? "encoder" : "estimado";
    pacote.semCorrecaoMm = micromouse::ENCODERS_INSTALADOS ? 0 : semCorrecaoMm;

    // Comporta o pior caso do JSON; com menos, a linha seria descartada em silêncio.
    char buffer[micromouse::TAMANHO_MAX_JSON_TELEMETRIA];
    if (micromouse::TelemetriaJson::serializar(pacote, buffer, sizeof(buffer)) > 0) {
      Serial.println(buffer);
    }

    // Comando serial para zerar medição (usado na bancada)
    while (Serial.available()) {
      const char c = static_cast<char>(Serial.read());
      if (c == 'Z' || c == 'z') {
        // Os encoders voltam a zero: a odometria mantém a pose e passa a contar a partir de zero.
        // No modo estimado, o robô também volta ao centro da célula e a pose volta a ser confiável
        // (na bancada, 'Z' é usado com o robô posicionado à mão).
        portENTER_CRITICAL(&travaOdometria);
        if (micromouse::ENCODERS_INSTALADOS) {
          medidorDeslocamento.zerar();
          odometria.reiniciar(odometria.pose(), 0, 0);
        } else {
          deslocamento.reiniciar(odometria, odometria.pose());
        }
        detectorObjetivo.resetar();
        portEXIT_CRITICAL(&travaOdometria);
        Serial.println("{\"status\":\"encoders_zerados\"}");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(micromouse::PERIODO_TELEMETRIA_MS));
  }
}

/// Inicializa o log serial pela USB nativa, os encoders, os sensores de distância e cria as duas
/// tarefas.
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
  motoresTracao.parar();
  if (micromouse::ENCODERS_INSTALADOS) {
    encoderEsq.iniciar();
    encoderDir.iniciar();
  }

  // O barramento I²C é dos 3 VL53L0X e do MPU6050; o driver dos ToF recebe ele já iniciado.
  Wire.begin(micromouse::pinos::I2C_SDA, micromouse::pinos::I2C_SCL);
  if (!tof.iniciar()) {
    Serial.printf("micromouse: falha nos sensores de distancia (frente %s, esquerda %s, direita %s)\n",
                  tof.sensorIniciado(micromouse::Lado::Frente) ? "ok" : "falhou",
                  tof.sensorIniciado(micromouse::Lado::Esquerda) ? "ok" : "falhou",
                  tof.sensorIniciado(micromouse::Lado::Direita) ? "ok" : "falhou");
  }

  xTaskCreate(tarefaNavegacao, "navegacao", 4096, nullptr, 2, nullptr);
  xTaskCreate(tarefaTelemetria, "telemetria", 4096, nullptr, 1, nullptr);
}

/// Laço principal do Arduino. Fica ocioso: o trabalho acontece nas tarefas.
void loop() {
  delay(1000);
}
