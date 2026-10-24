/// @file main.cpp
/// @brief Ponto de entrada do firmware: inicializa o sistema e cria as tarefas FreeRTOS.
///
/// Navegação e telemetria rodam em tarefas separadas e concorrentes, como no
/// diagrama de atividades. Os encoders físicos de quadratura são decodificados por
/// interrupção de hardware no ESP32-C3, alimentam a odometria (pose na malha) e são reportados na
/// telemetria. Os 3 sensores de distância VL53L0X são lidos pelo I²C na tarefa de navegação, e o
/// giroscópio do MPU6050 por uma tarefa própria, em intervalo fixo, no mesmo barramento (com mutex).
/// Enquanto os encoders não são instalados (`ENCODERS_INSTALADOS` em `ConfiguracaoDeslocamento.h`),
/// o deslocamento é estimado pela potência dos motores e corrigido pelos ToF (#380).
/// Os motores só são liberados depois do autoteste do boot (#50) encontrar os 4 dispositivos do I²C.

#include <Arduino.h>
#include <Wire.h>

// O log (`Serial`) tem que sair pela USB nativa: GPIO20/21, a UART0, são entradas do driver dos
// motores. Se esta opção faltar no `platformio.ini`, o log iria para os pinos do motor direito.
#if !defined(ARDUINO_USB_CDC_ON_BOOT) || !ARDUINO_USB_CDC_ON_BOOT
#error "Serial pela USB nativa: defina ARDUINO_USB_MODE=1 e ARDUINO_USB_CDC_ON_BOOT=1 no platformio.ini (Pinos.h)"
#endif

#include "AcompanhadorGiro.h"
#include "ArmazenamentoMapa.h"
#include "AutotesteBoot.h"
#include "BarramentoI2c.h"
#include "BarramentoI2cEsp32.h"
#include "Configuracao.h"
#include "ConfiguracaoDeslocamento.h"
#include "ConfiguracaoImu.h"
#include "ConversorDeslocamento.h"
#include "DeslocamentoEstimado.h"
#include "DetectorObjetivo.h"
#include "EncoderEsp32.h"
#include "ImuMpu6050.h"
#include "MedidorDeslocamento.h"
#include "MotoresComTrava.h"
#include "MotoresDrv8833.h"
#include "Odometria.h"
#include "Pinos.h"
#include "RelatorioBoot.h"
#include "RotinaCalibracao.h"
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
/// estimador saber a potência; com os encoders instalados, a estimativa sai e a trava envolve o
/// próprio driver.
///
/// A estimativa fica colada no driver, para ver só o que chega de fato às rodas. Os outros
/// decoradores a envolvem, nesta ordem: `MotoresComTrava` (#50) → `MotoresComProtecao` (#341) →
/// `MotoresComEstimativa` → `MotoresDrv8833`. Assim um comando recusado pela trava ou pela proteção
/// chega ao estimador como freio.
static micromouse::DeslocamentoEstimado deslocamento(motores);
static micromouse::IMotores& motoresEstimativa =
    micromouse::ENCODERS_INSTALADOS ? static_cast<micromouse::IMotores&>(motores) : deslocamento.motores();

/// Trava dos motores (#50): começa travada e só é liberada com o autoteste do boot aprovado.
static micromouse::MotoresComTrava motoresComTrava(motoresEstimativa);

/// Motores que a navegação aciona: sempre a camada de fora, nunca o driver direto, para a trava
/// valer. Cada camada tem o próprio nome e esta referência aponta para a de fora. Com a trava por
/// fora, a proteção (#341) recebe freio enquanto o robô não foi liberado e não acusa um motor
/// travado falso.
static micromouse::IMotores& motoresTracao = motoresComTrava;

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

/// Resultado do autoteste do boot, para a telemetria repetir em todo pacote. Só o `setup()` escreve,
/// antes de criar as tarefas; depois é só lido, então não precisa de seção crítica.
struct EstadoBoot {
  bool ok = false;
  char falhas[micromouse::TAMANHO_MAX_FALHAS_BOOT] = "";
  uint32_t duracaoMs = 0;
  micromouse::ResultadoBoot resultado;
};
static EstadoBoot estadoBoot;
static const micromouse::AutotesteBoot autoteste;

/// Escreve no log a linha JSON do relatório do boot (`{"tipo":"boot",...}`).
static void imprimirRelatorioBoot() {
  char relatorio[micromouse::TAMANHO_MAX_JSON_RELATORIO_BOOT];
  if (micromouse::RelatorioBoot::serializar(estadoBoot.resultado, autoteste, estadoBoot.duracaoMs, relatorio,
                                            sizeof(relatorio)) > 0) {
    Serial.println(relatorio);
  }
}

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

/// Barramento I²C dos 3 VL53L0X e do MPU6050. Toda transação passa pelo mutex dele (`BarramentoI2c::Uso`),
/// porque a tarefa da IMU e a navegação (sensores de distância) usam o barramento ao mesmo tempo.
static micromouse::BarramentoI2c barramentoI2c(Wire);

/// Giroscópio do MPU6050 (#378) e o acompanhamento do ângulo de giro. Só a tarefa da IMU os usa; as
/// outras tarefas leem a cópia `leituraGiro` por `lerGiro()`.
static micromouse::ImuMpu6050 imu(barramentoI2c);
static micromouse::AcompanhadorGiro acompanhadorGiro(imu);
static micromouse::LeituraGiro leituraGiro;
static portMUX_TYPE travaGiro = portMUX_INITIALIZER_UNLOCKED;

/// Cópia da última leitura do giroscópio: taxa e ângulo de giro (para os giros de 90° e 180°, #190,
/// e para a proteção dos motores, #341), com a validade e os contadores.
[[maybe_unused]] static micromouse::LeituraGiro lerGiro() {
  portENTER_CRITICAL(&travaGiro);
  const micromouse::LeituraGiro copia = leituraGiro;
  portEXIT_CRITICAL(&travaGiro);
  return copia;
}

/// Calibração do boot (#240, RNF-08): desvio de cada ToF e viés do giroscópio, em até 1 s, com o robô
/// parado na célula de partida. O `corrigirTof` é aplicado a toda leitura dos ToF (navegação) e o viés
/// vai para o `acompanhadorGiro`. Falhou, o robô segue sem correção de ToF e a tarefa da IMU calibra o
/// viés sozinha.
static micromouse::RotinaCalibracao rotinaCalibracao;

/// Roda a `RotinaCalibracao` com os sensores reais, a cada 10 ms, até ela terminar (sucesso, falha ou
/// 1 s). Só pode ser chamada no `setup()`, antes de criar as tarefas, porque usa o ToF e a IMU sem
/// outra tarefa disputando o I²C. Conta como amostra de ToF só a leitura com medida nova.
static micromouse::EstadoRotinaCalibracao calibrarSensores() {
  rotinaCalibracao.iniciar(micromouse::REFERENCIAS_TOF_NA_LARGADA);
  uint32_t anteriorMs = millis();
  uint32_t instanteUltimaLeituraTof = 0;
  bool leuTof = false;
  while (rotinaCalibracao.estado() == micromouse::EstadoRotinaCalibracao::EmAndamento) {
    delay(micromouse::PERIODO_AMOSTRAGEM_GIRO_MS);
    const uint32_t agoraMs = millis();

    int16_t taxaGiro = 0;
    const bool giroValido = imu.lerTaxaGiroZ(taxaGiro);

    micromouse::DistanciasLaterais distancias = {0, 0, 0};
    const bool tofLido = tof.lerDistanciasLaterais(distancias);
    const uint32_t instanteLeitura = agoraMs - tof.idadeUltimaLeituraMs();
    const bool tofNovo = tofLido && (!leuTof || instanteLeitura != instanteUltimaLeituraTof);
    if (tofNovo) {
      leuTof = true;
      instanteUltimaLeituraTof = instanteLeitura;
    }

    rotinaCalibracao.atualizar(distancias, tofNovo, taxaGiro, giroValido, agoraMs - anteriorMs);
    anteriorMs = agoraMs;
  }
  return rotinaCalibracao.estado();
}

/// Tarefa da IMU: lê o giroscópio a cada `PERIODO_LEITURA_IMU_MS` (10 ms), em intervalo fixo.
///
/// Prioridade 3, a maior: a leitura é curta (menos de 1 ms) e o ângulo só fica certo se o intervalo
/// entre leituras não variar. Se a calibração do boot (`calibrarSensores`) não deu o viés, calibra
/// por conta própria nos primeiros 2 s, com o robô parado na largada; se o robô se mexer, a
/// calibração recomeça sozinha.
static void tarefaImu(void*) {
  if (acompanhadorGiro.estadoCalibracao() != micromouse::EstadoCalibracao::Calibrado) {
    acompanhadorGiro.iniciarCalibracao();
  }
  bool calibradoAnunciado = false;
  TickType_t ultimoInstante = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&ultimoInstante, pdMS_TO_TICKS(micromouse::PERIODO_LEITURA_IMU_MS));
    // O `vTaskDelayUntil` acorda em múltiplos exatos do período, então o intervalo é o período.
    const micromouse::LeituraGiro leitura = acompanhadorGiro.ciclo(micromouse::PERIODO_LEITURA_IMU_MS);

    portENTER_CRITICAL(&travaGiro);
    leituraGiro = leitura;
    portEXIT_CRITICAL(&travaGiro);

    if (!calibradoAnunciado && leitura.calibracao == micromouse::EstadoCalibracao::Calibrado) {
      calibradoAnunciado = true;
      Serial.printf("{\"status\":\"giroscopio_calibrado\",\"recusadas\":%lu}\n",
                    static_cast<unsigned long>(leitura.calibracoesRecusadas));
    }
  }
}

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
    // A tarefa da IMU usa o mesmo I²C: a leitura só acontece com o barramento segurado. Se ele não
    // vier a tempo, o ciclo segue sem leitura nova do ToF.
    LeituraTofTelemetria leituraTof;
    {
      micromouse::BarramentoI2c::Uso uso(barramentoI2c);
      leituraTof.valida = uso.conseguiu() && tof.lerDistanciasLaterais(leituraTof.distancias);
    }
    if (leituraTof.valida) {
      // Desvio de cada sensor medido na calibração do boot (zero se ela não aconteceu).
      leituraTof.distancias = rotinaCalibracao.corrigirTof(leituraTof.distancias);
    }
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
/// estruturado e aceita os comandos 'Z', para zerar os encoders e voltar a odometria à pose atual, e
/// 'B', para repetir o relatório do boot (a linha do boot se perde se o monitor abrir depois).
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
    pacote.bootOk = estadoBoot.ok;
    pacote.falhasBoot = estadoBoot.falhas;
    pacote.bootMs = estadoBoot.duracaoMs;

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
      } else if (c == 'B' || c == 'b') {
        imprimirRelatorioBoot();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(micromouse::PERIODO_TELEMETRIA_MS));
  }
}

/// Sequência de boot da Rev 2.0 (#50): motores freados e travados, barramento I²C, sensores de
/// distância com os endereços novos, autoteste do I²C e, só com ele aprovado, motores liberados;
/// depois a IMU (#378) e as tarefas. As tarefas sobem mesmo com falha, para a telemetria continuar
/// informando o que falhou.
void setup() {
  // Primeira linha: freia os motores antes de qualquer outra coisa, porque o GPIO20/21 (IN3/IN4)
  // recebe um tranco da ROM durante o reset.
  motores.iniciar();
  Serial.begin(115200);
  // Sem um computador conectado, a escrita na USB CDC esperaria o tempo limite a cada chamada e
  // atrasaria a telemetria. Com tempo zero, o dado é descartado se ninguém estiver lendo.
  Serial.setTxTimeoutMs(0);
  Serial.println("micromouse: iniciando");

  // Configuração provisória para bancada (#189): fixa Labirinto4x4.
  // Em 8x4 e 12x4 a corrida deve receber a dimensão por comando antes de iniciar.
  armazenamentoMapa.iniciarCorrida(micromouse::DimensaoMapa::Labirinto4x4);
  motoresTracao.parar();
  if (micromouse::ENCODERS_INSTALADOS) {
    encoderEsq.iniciar();
    encoderDir.iniciar();
  }

  // O barramento I²C é dos 3 VL53L0X e do MPU6050: `barramentoI2c.iniciar` cria o mutex e inicia o
  // `Wire`. O `tof.iniciar()` e o autoteste usam o `Wire` sem o mutex porque rodam aqui, antes dos
  // `xTaskCreate` (inclusive o da IMU). Se falhar, o autoteste aponta os dispositivos sem resposta.
  if (!barramentoI2c.iniciar(micromouse::pinos::I2C_SDA, micromouse::pinos::I2C_SCL)) {
    Serial.println("{\"status\":\"falha_barramento_i2c\"}");
  }
  tof.iniciar();  // o autoteste abaixo diz qual sensor falhou

  // Autoteste: os 3 ToF em 0x30, 0x31 e 0x32, o MPU6050 em 0x68 e ninguém no 0x29.
  micromouse::BarramentoI2cEsp32 barramento(Wire);
  estadoBoot.resultado = autoteste.verificar(barramento, {tof.sensorIniciado(micromouse::Lado::Esquerda),
                                                          tof.sensorIniciado(micromouse::Lado::Frente),
                                                          tof.sensorIniciado(micromouse::Lado::Direita)});
  micromouse::AutotesteBoot::descreverFalhas(estadoBoot.resultado, estadoBoot.falhas, sizeof(estadoBoot.falhas));
  estadoBoot.ok = estadoBoot.resultado.aprovado();
  if (estadoBoot.ok) {
    motoresComTrava.liberar();
  } else {
    motoresComTrava.travar();
    Serial.printf("micromouse: falha no boot (%s): motores travados ate reiniciar\n", estadoBoot.falhas);
  }

  // Tempo desde o início do programa (RNF-09); a ROM e o bootloader rodam antes e ficam de fora.
  estadoBoot.duracaoMs = millis();
  imprimirRelatorioBoot();

  // MPU6050 (#378). Sem a IMU o firmware segue, sem ângulo de giro válido.
  const bool imuIniciada = imu.iniciar();
  if (imuIniciada) {
    Serial.printf("{\"status\":\"imu_iniciada\",\"who_am_i\":\"0x%02X\"}\n", imu.identificacao());
  } else {
    Serial.println("{\"status\":\"falha_imu\"}");
  }

  // Calibração dos ToF e do giroscópio (#240, RNF-08), com o robô parado na célula de partida. Só
  // com o autoteste aprovado e a IMU de pé: sem isso, não há o que calibrar.
  if (estadoBoot.ok && imuIniciada) {
    const micromouse::EstadoRotinaCalibracao calibracao = calibrarSensores();
    if (calibracao == micromouse::EstadoRotinaCalibracao::Sucesso) {
      acompanhadorGiro.definirVies(rotinaCalibracao.viesGiroDecimosDps());
    }
    Serial.printf("{\"status\":\"calibracao\",\"estado\":\"%s\",\"duracao_ms\":%lu,\"no_prazo\":%s}\n",
                  micromouse::nomeEstadoRotina(calibracao),
                  static_cast<unsigned long>(rotinaCalibracao.duracaoMedidaMs()),
                  rotinaCalibracao.dentroDoOrcamento() ? "true" : "false");
  }

  xTaskCreate(tarefaImu, "imu", 3072, nullptr, 3, nullptr);
  xTaskCreate(tarefaNavegacao, "navegacao", 4096, nullptr, 2, nullptr);
  xTaskCreate(tarefaTelemetria, "telemetria", 4096, nullptr, 1, nullptr);
}

/// Laço principal do Arduino. Fica ocioso: o trabalho acontece nas tarefas.
void loop() {
  delay(1000);
}
