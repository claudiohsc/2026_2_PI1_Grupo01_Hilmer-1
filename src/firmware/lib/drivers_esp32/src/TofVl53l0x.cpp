#include "TofVl53l0x.h"

namespace micromouse {

namespace {

/// Bits de "medida pronta" do registrador `RESULT_INTERRUPT_STATUS` do VL53L0X.
constexpr uint8_t MASCARA_MEDIDA_PRONTA = 0x07;

constexpr Lado LADOS[3] = {Lado::Frente, Lado::Esquerda, Lado::Direita};

}  // namespace

TofVl53l0x::TofVl53l0x(TwoWire& barramento, const ConexaoTof& frente, const ConexaoTof& esquerda,
                       const ConexaoTof& direita, const ConfiguracaoTof& config)
    : barramento_(barramento),
      config_(config),
      conexoes_{frente, esquerda, direita},
      combinador_(config) {}

bool TofVl53l0x::iniciar() {
  // Todos em reset: com XSHUT baixo, nenhum sensor responde no endereço de fábrica.
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(conexoes_[i].pinoXshut, OUTPUT);
    digitalWrite(conexoes_[i].pinoXshut, LOW);
    iniciados_[i] = false;
  }
  delay(config_.esperaPartidaMs);
  combinador_.reiniciar();

  bool todos = true;
  for (uint8_t i = 0; i < 3; i++) {
    iniciados_[i] = iniciarSensor(i);
    todos = todos && iniciados_[i];
  }
  return todos;
}

bool TofVl53l0x::reiniciarSensor(Lado lado) {
  const uint8_t indice = static_cast<uint8_t>(lado);
  // Em reset, o sensor esquece o endereço novo e volta ao de fábrica, que o `iniciarSensor` muda.
  digitalWrite(conexoes_[indice].pinoXshut, LOW);
  iniciados_[indice] = false;
  delay(config_.esperaPartidaMs);
  iniciados_[indice] = iniciarSensor(indice);
  return iniciados_[indice];
}

bool TofVl53l0x::iniciarSensor(uint8_t indice) {
  const ConexaoTof& conexao = conexoes_[indice];
  // Objeto novo: depois do reset, o sensor voltou ao endereço de fábrica, que é o do construtor.
  VL53L0X& sensor = sensores_[indice];
  sensor = VL53L0X();
  sensor.setBus(&barramento_);
  sensor.setTimeout(config_.tempoLimiteI2cMs);

  digitalWrite(conexao.pinoXshut, HIGH);
  delay(config_.esperaPartidaMs);

  const bool iniciou = sensor.init();
  if (iniciou) {
    sensor.setAddress(conexao.endereco);
  }
  if (!iniciou || sensor.last_status != 0 ||
      !sensor.setMeasurementTimingBudget(config_.tempoMedicaoUs)) {
    // Fica em reset, para não ocupar o endereço de fábrica do próximo sensor.
    digitalWrite(conexao.pinoXshut, LOW);
    return false;
  }
  sensor.startContinuous(config_.periodoMedicaoMs);
  return sensor.last_status == 0;
}

bool TofVl53l0x::lerDistanciasLaterais(DistanciasLaterais& saida) {
  const uint32_t agoraMs = millis();
  for (uint8_t i = 0; i < 3; i++) {
    lerSensor(i, agoraMs);
  }
  return combinador_.distancias(agoraMs, saida);
}

uint32_t TofVl53l0x::idadeUltimaLeituraMs() const { return combinador_.idadeMs(millis()); }

void TofVl53l0x::lerSensor(uint8_t indice, uint32_t agoraMs) {
  const Lado lado = LADOS[indice];
  if (!iniciados_[indice]) {
    combinador_.registrar(lado, 0, true, agoraMs);
    return;
  }
  VL53L0X& sensor = sensores_[indice];

  // Confere se há medida pronta antes de ler: `readRangeContinuousMillimeters()` esperaria por ela.
  const uint8_t interrupcao = sensor.readReg(VL53L0X::RESULT_INTERRUPT_STATUS);
  if (sensor.last_status != 0) {
    combinador_.registrar(lado, 0, true, agoraMs);
    return;
  }
  if ((interrupcao & MASCARA_MEDIDA_PRONTA) == 0) {
    // Sem medida nova: a amostra anterior envelhece, e o combinador recusa quando passar do limite.
    return;
  }
  const uint16_t distanciaMm = sensor.readRangeContinuousMillimeters();
  const bool falhou = sensor.timeoutOccurred() || sensor.last_status != 0;
  combinador_.registrar(lado, distanciaMm, falhou, agoraMs);
}

}  // namespace micromouse
