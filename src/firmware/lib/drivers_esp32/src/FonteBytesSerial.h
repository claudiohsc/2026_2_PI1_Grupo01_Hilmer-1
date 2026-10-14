#pragma once

/// @file FonteBytesSerial.h
/// @brief Adaptador de uma UART do ESP32 (`HardwareSerial`) para `IFonteBytes`.
///
/// Único arquivo do driver do LiDAR que depende do Arduino. Por isso não compila no ambiente
/// `native` e não é coberto por testes no computador.

#include <Arduino.h>

#include "IFonteBytes.h"

namespace micromouse {

/// Lê bytes de uma UART do ESP32, sem bloquear.
///
/// ATENÇÃO aos pinos: `Pinos.h` prevê o LiDAR em GPIO20/GPIO21, que são os pinos da UART0
/// usada pelo `Serial` (log e gravação). Se o `Serial` estiver em uso nesses pinos, o log e os
/// dados do sensor se misturam. Por isso os pinos são parâmetros de `iniciar()`.
class FonteBytesSerial : public IFonteBytes {
 public:
  /// @param serial UART a usar, por exemplo `Serial1`.
  explicit FonteBytesSerial(HardwareSerial& serial) : serial_(serial) {}

  /// Abre a UART. Chame uma vez, em `setup()`.
  /// @param baud Velocidade do sensor (conferir no manual do ST-L50B2).
  /// @param pinoRx GPIO ligado ao TX do sensor.
  /// @param pinoTx GPIO ligado ao RX do sensor.
  /// @param bufferRxBytes Tamanho do buffer de recepção. O LD06 envia cerca de 17,6 KB/s
  ///        (375 pacotes de 47 bytes por segundo): o buffer padrão de 256 bytes encheria em uns
  ///        15 ms e perderia dados entre duas leituras. Com 2048 bytes sobram ~115 ms.
  ///        É definido antes de abrir a UART.
  void iniciar(uint32_t baud, int8_t pinoRx, int8_t pinoTx, size_t bufferRxBytes = 2048) {
    serial_.setRxBufferSize(bufferRxBytes);
    serial_.begin(baud, SERIAL_8N1, pinoRx, pinoTx);
  }

  /// @copydoc IFonteBytes::lerByte
  bool lerByte(uint8_t& saida) override {
    if (serial_.available() <= 0) {
      return false;
    }
    const int valor = serial_.read();
    if (valor < 0) {
      return false;
    }
    saida = static_cast<uint8_t>(valor);
    return true;
  }

 private:
  HardwareSerial& serial_;
};

}  // namespace micromouse
