#include "BarramentoI2cEsp32.h"

namespace micromouse {

bool BarramentoI2cEsp32::responde(uint8_t endereco) {
  barramento_.beginTransmission(endereco);
  return barramento_.endTransmission() == 0;
}

bool BarramentoI2cEsp32::lerRegistrador(uint8_t endereco, uint8_t registrador, uint8_t& valor) {
  barramento_.beginTransmission(endereco);
  barramento_.write(registrador);
  // Sem STOP entre a escrita do registrador e a leitura (início repetido), como pede o MPU6050.
  if (barramento_.endTransmission(false) != 0) {
    return false;
  }
  if (barramento_.requestFrom(endereco, static_cast<uint8_t>(1)) != 1) {
    return false;
  }
  valor = static_cast<uint8_t>(barramento_.read());
  return true;
}

}  // namespace micromouse
