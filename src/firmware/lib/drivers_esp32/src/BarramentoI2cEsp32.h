#pragma once

/// @file BarramentoI2cEsp32.h
/// @brief `IBarramentoI2c` pelo `Wire` do Arduino, para o autoteste do boot (#50).

#include <Wire.h>

#include "IBarramentoI2c.h"

namespace micromouse {

/// Barramento I²C real. Recebe o `TwoWire` já iniciado (`Wire.begin(SDA, SCL)` no `setup()`).
///
/// Não tem proteção entre tarefas: o autoteste roda no `setup()`, antes de as tarefas existirem.
class BarramentoI2cEsp32 : public IBarramentoI2c {
 public:
  explicit BarramentoI2cEsp32(TwoWire& barramento) : barramento_(barramento) {}

  /// @copydoc IBarramentoI2c::responde
  bool responde(uint8_t endereco) override;

  /// @copydoc IBarramentoI2c::lerRegistrador
  bool lerRegistrador(uint8_t endereco, uint8_t registrador, uint8_t& valor) override;

 private:
  TwoWire& barramento_;
};

}  // namespace micromouse
