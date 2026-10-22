/// @file main.cpp
/// @brief Ponto de entrada do firmware: inicializa o sistema e cria as tarefas FreeRTOS.
///
/// Navegação e telemetria rodam em tarefas separadas e concorrentes, como no
/// diagrama de atividades. Os encoders físicos de quadratura são decodificados por
/// interrupção de hardware no ESP32-C3, alimentam a odometria (pose na malha) e são reportados na
/// telemetria. Os 3 sensores de distância VL53L0X são lidos pelo I²C na tarefa de navegação.

#include <Arduino.h>
#include <Wire.h>

// O log (`Serial`) tem que sair pela USB nativa: GPIO20/21, a UART0, são entradas do driver dos
// motores. Se esta opção faltar no `platformio.ini`, o log iria para os pinos do motor direito.
#if !defined(ARDUINO_USB_CDC_ON_BOOT) || !ARDUINO_USB_CDC_ON_BOOT
#error "Serial pela USB nativa: defina ARDUINO_USB_MODE=1 e ARDUINO_USB_CDC_ON_BOOT=1 no platformio.ini (Pinos.h)"
#endif

#include "ArmazenamentoMapa.h"
#include "Configuracao.h"
#include "ConversorDeslocamento.h"
#include "EncoderEsp32.h"
#include "MedidorDeslocamento.h"
#include "Odometria.h"
#include "Pinos.h"
#include "MotoresDrv8833.h"
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

static micromouse::ConversorDeslocamento conversorDeslocamento;
static micromouse::MedidorDeslocamento medidorDeslocamento(encoderEsq, encoderDir, conversorDeslocamento);

/// Pose (célula e direção) estimada pela odometria diferencial. A tarefa de navegação a atualiza a
/// cada 10 ms e a telemetria a lê, então todo acesso passa pela seção crítica `travaOdometria`.
static micromouse::Odometria odometria;
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
  for (;;) {
    // Odometria: o intervalo curto (10 ms) mantém cada atualização com giro ou avanço, não os dois.
    portENTER_CRITICAL(&travaOdometria);
    odometria.atualizar(encoderEsq, encoderDir);
    portEXIT_CRITICAL(&travaOdometria);

    // Sensores de distância: a leitura não espera medida nova, só junta as últimas amostras.
    LeituraTofTelemetria leituraTof;
    leituraTof.valida = tof.lerDistanciasLaterais(leituraTof.distancias);
    leituraTof.idadeMs = tof.idadeUltimaLeituraMs();
    portENTER_CRITICAL(&travaTof);
    ultimaLeituraTof = leituraTof;
    portEXIT_CRITICAL(&travaTof);

    // Ciclo percepção, decisão e atuação: percepção, mapeamento, navegação e atuação entram aqui.
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/// Tarefa de telemetria: coleta e envia os dados a cada `PERIODO_TELEMETRIA_MS`.
///
/// Prioridade 1. Transmite as leituras dos encoders e a pose da odometria em formato JSON
/// estruturado e aceita o comando 'Z' para zerar os encoders e voltar a odometria à pose atual.
static void tarefaTelemetria(void*) {
  for (;;) {
    // Leitura dos encoders
    const micromouse::DeslocamentoRodas d = medidorDeslocamento.medir();
    const int32_t medio = medidorDeslocamento.deslocamentoMedioMm();
    const int32_t pulsosEsq = encoderEsq.lerContagem();
    const int32_t pulsosDir = encoderDir.lerContagem();

    // Pose da odometria: uma cópia feita na seção crítica, para a linha e a coluna serem da mesma
    // atualização.
    portENTER_CRITICAL(&travaOdometria);
    const micromouse::Pose pose = odometria.pose();
    const bool poseValida = odometria.posicaoValida();
    portEXIT_CRITICAL(&travaOdometria);

    portENTER_CRITICAL(&travaTof);
    const LeituraTofTelemetria leituraTof = ultimaLeituraTof;
    portEXIT_CRITICAL(&travaTof);

    Serial.printf("{\"esq_pulsos\":%ld,\"dir_pulsos\":%ld,\"esq_mm\":%ld,\"dir_mm\":%ld,\"medio_mm\":%ld,"
                  "\"linha\":%u,\"coluna\":%u,\"orientacao\":\"%c\",\"pose_valida\":%s,"
                  "\"tof_frente_mm\":%u,\"tof_esq_mm\":%u,\"tof_dir_mm\":%u,\"tof_valida\":%s,"
                  "\"tof_idade_ms\":%lu}\n",
                  static_cast<long>(pulsosEsq),
                  static_cast<long>(pulsosDir),
                  static_cast<long>(d.esquerdaMm),
                  static_cast<long>(d.direitaMm),
                  static_cast<long>(medio),
                  static_cast<unsigned>(pose.celula.linha),
                  static_cast<unsigned>(pose.celula.coluna),
                  micromouse::letraDaDirecao(pose.direcao),
                  poseValida ? "true" : "false",
                  static_cast<unsigned>(leituraTof.distancias.frenteMm),
                  static_cast<unsigned>(leituraTof.distancias.esquerdaMm),
                  static_cast<unsigned>(leituraTof.distancias.direitaMm),
                  leituraTof.valida ? "true" : "false",
                  static_cast<unsigned long>(leituraTof.idadeMs));

    // Comando serial para zerar medição (usado na bancada)
    while (Serial.available()) {
      const char c = static_cast<char>(Serial.read());
      if (c == 'Z' || c == 'z') {
        // Os encoders voltam a zero: a odometria mantém a pose e passa a contar a partir de zero.
        portENTER_CRITICAL(&travaOdometria);
        medidorDeslocamento.zerar();
        odometria.reiniciar(odometria.pose(), 0, 0);
        portEXIT_CRITICAL(&travaOdometria);
        Serial.println("{\"status\":\"encoders_zerados\"}");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(micromouse::PERIODO_TELEMETRIA_MS));
  }
}

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

/// Inicializa o log serial pela USB nativa, os encoders, os sensores de distância e cria as duas
/// tarefas.
void setup() {
  Serial.begin(115200);
  // Sem um computador conectado, a escrita na USB CDC esperaria o tempo limite a cada chamada e
  // atrasaria a telemetria. Com tempo zero, o dado é descartado se ninguém estiver lendo.
  Serial.setTxTimeoutMs(0);
  Serial.println("micromouse: iniciando");

  motores.iniciar();  // começa freado
  encoderEsq.iniciar();
  encoderDir.iniciar();

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
