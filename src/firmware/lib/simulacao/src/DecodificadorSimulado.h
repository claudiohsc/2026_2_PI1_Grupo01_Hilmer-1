#pragma once

/// @file DecodificadorSimulado.h
/// @brief Decodificador de um protocolo INVENTADO, só para testar o driver sem o sensor.

#include <stdint.h>

#include "IDecodificadorLidar.h"

namespace micromouse {

/// Decodificador de teste. O protocolo abaixo NÃO é o do ST-L50B2: serve apenas para exercitar
/// o `LidarUart` (bytes em pacotes, pacote corrompido, ressincronização) enquanto o decodificador
/// do sensor real não existe.
///
/// Pacote de 6 bytes, um ponto por pacote:
/// `[0xAA][angulo_lo][angulo_hi][distancia_lo][distancia_hi][xor dos 4 bytes anteriores]`
/// com ângulo em centésimos de grau e distância em mm, ambos little-endian.
/// Um pacote com erro de verificação é descartado inteiro.
class DecodificadorSimulado : public IDecodificadorLidar {
 public:
  static constexpr uint8_t CABECALHO = 0xAA;
  static constexpr uint8_t TAMANHO_PACOTE = 6;

  /// Monta os bytes de um pacote válido para o ponto indicado.
  static void montarPacote(uint16_t anguloCentiGraus, uint16_t distanciaMm,
                           uint8_t (&saida)[TAMANHO_PACOTE]) {
    saida[0] = CABECALHO;
    saida[1] = static_cast<uint8_t>(anguloCentiGraus & 0xFF);
    saida[2] = static_cast<uint8_t>(anguloCentiGraus >> 8);
    saida[3] = static_cast<uint8_t>(distanciaMm & 0xFF);
    saida[4] = static_cast<uint8_t>(distanciaMm >> 8);
    saida[5] = static_cast<uint8_t>(saida[1] ^ saida[2] ^ saida[3] ^ saida[4]);
  }

  /// @copydoc IDecodificadorLidar::alimentar
  bool alimentar(uint8_t byte) override {
    if (recebidos_ == 0 && byte != CABECALHO) {
      return false;  // fora de um pacote: espera o cabeçalho
    }
    buffer_[recebidos_++] = byte;
    if (recebidos_ < TAMANHO_PACOTE) {
      return false;
    }
    recebidos_ = 0;
    if ((buffer_[1] ^ buffer_[2] ^ buffer_[3] ^ buffer_[4]) != buffer_[5]) {
      return false;  // erro de verificação: descarta o pacote
    }
    ponto_.anguloCentiGraus = static_cast<uint16_t>(buffer_[1] | (buffer_[2] << 8));
    ponto_.distanciaMm = static_cast<uint16_t>(buffer_[3] | (buffer_[4] << 8));
    return true;
  }

  /// @copydoc IDecodificadorLidar::quantidadePontos
  uint8_t quantidadePontos() const override { return 1; }

  /// @copydoc IDecodificadorLidar::ponto
  PontoLidar ponto(uint8_t) const override { return ponto_; }

 private:
  uint8_t buffer_[TAMANHO_PACOTE] = {};
  uint8_t recebidos_ = 0;
  PontoLidar ponto_ = {0, 0};
};

}  // namespace micromouse
