#pragma once

/// @file IBarramentoI2c.h
/// @brief Interface mínima do barramento I²C para o autoteste do boot (camada `hal`, #50).
///
/// O `AutotesteBoot` (`lib/inicializacao`) só precisa saber quem responde no barramento e ler um
/// registrador de identificação. Com esta interface, a lógica do boot é testada no computador com o
/// `BarramentoI2cSimulado`; na placa, o `BarramentoI2cEsp32` (`drivers_esp32`) usa o `Wire`.

#include <stdint.h>

namespace micromouse {

/// Barramento I²C visto pelo autoteste do boot.
///
/// Implementações:
/// - `BarramentoI2cSimulado` (`lib/simulacao`): o teste escolhe quem responde e o que cada
///   registrador devolve.
/// - `BarramentoI2cEsp32` (`lib/drivers_esp32`): `Wire` do Arduino.
class IBarramentoI2c {
 public:
  virtual ~IBarramentoI2c() = default;

  /// `true` se algum dispositivo confirma (ACK) o endereço de 7 bits `endereco`.
  virtual bool responde(uint8_t endereco) = 0;

  /// Lê um byte do `registrador` do dispositivo em `endereco`.
  /// @return `false` se o dispositivo não respondeu; `valor` fica como estava.
  virtual bool lerRegistrador(uint8_t endereco, uint8_t registrador, uint8_t& valor) = 0;
};

}  // namespace micromouse
