#pragma once

/// @file IFonteBytes.h
/// @brief Interface de uma fonte de bytes lida sem bloquear (camada `hal`).
///
/// O driver do LiDAR lê os bytes por esta interface, sem saber se eles vêm da UART
/// do ESP32 (`FonteBytesSerial`, em `drivers_esp32`) ou de uma fila montada por um
/// teste (`FonteBytesSimulada`, em `simulacao`).

#include <stdint.h>

namespace micromouse {

/// Fonte de bytes de leitura não bloqueante.
class IFonteBytes {
 public:
  virtual ~IFonteBytes() = default;

  /// Lê um byte, se houver algum disponível. Nunca espera por dados.
  /// @param[out] saida Recebe o byte lido quando o retorno é `true`.
  /// @return `true` se um byte foi lido; `false` se não há bytes disponíveis agora.
  virtual bool lerByte(uint8_t& saida) = 0;
};

}  // namespace micromouse
