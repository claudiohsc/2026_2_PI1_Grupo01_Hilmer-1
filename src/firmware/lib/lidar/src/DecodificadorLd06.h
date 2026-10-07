#pragma once

/// @file DecodificadorLd06.h
/// @brief Decodificador do protocolo serial do LD06 (LDROBOT), que o anúncio do ST-L50B2 diz ser igual.

#include <stdint.h>

#include "IDecodificadorLidar.h"

namespace micromouse {

/// Decodifica os pacotes de 47 bytes do LD06 em 12 pontos cada.
///
/// Formato do pacote (little-endian), conforme o manual do LD06:
/// | bytes | campo                                                      |
/// |-------|------------------------------------------------------------|
/// | 0     | cabeçalho, sempre `0x54`                                   |
/// | 1     | tipo e quantidade de pontos, sempre `0x2C` (12 pontos)     |
/// | 2-3   | velocidade de rotação, em graus por segundo                |
/// | 4-5   | ângulo do primeiro ponto, em centésimos de grau            |
/// | 6-41  | 12 pontos de 3 bytes: distância em mm (2 bytes) + intensidade |
/// | 42-43 | ângulo do último ponto, em centésimos de grau              |
/// | 44-45 | instante da medida, em ms (volta a zero em 30000)          |
/// | 46    | CRC-8 dos bytes 0 a 45 (polinômio 0x4D, valor inicial 0)   |
///
/// Os ângulos dos pontos intermediários são interpolados linearmente entre o primeiro e o último.
/// A intensidade é ignorada por enquanto.
///
/// Pacotes com cabeçalho inválido, CRC errado ou ângulos fora de 0 a 359,99° são descartados e o
/// decodificador se ressincroniza sozinho no próximo cabeçalho `0x54`.
///
/// ATENÇÃO: este formato foi confirmado só para o LD06. Que o ST-L50B2 (etiquetado STL-50B2) usa o
/// mesmo protocolo é uma informação do anúncio do vendedor, ainda sem confirmação no sensor.
class DecodificadorLd06 : public IDecodificadorLidar {
 public:
  /// Velocidade da UART do LD06 (8 bits, sem paridade, 1 stop bit, sem controle de fluxo).
  static constexpr uint32_t BAUD_PADRAO = 230400;
  static constexpr uint8_t TAMANHO_PACOTE = 47;
  static constexpr uint8_t PONTOS_POR_PACOTE = 12;
  static constexpr uint8_t CABECALHO = 0x54;
  static constexpr uint8_t TIPO_E_QUANTIDADE = 0x2C;

  /// CRC-8 usado pelo LD06 (polinômio 0x4D, valor inicial 0, sem inversão de bits).
  /// @param dados Bytes sobre os quais calcular o CRC.
  /// @param tamanho Quantidade de bytes.
  static uint8_t calcularCrc8(const uint8_t* dados, uint8_t tamanho);

  /// @copydoc IDecodificadorLidar::alimentar
  bool alimentar(uint8_t byte) override;

  /// @copydoc IDecodificadorLidar::quantidadePontos
  uint8_t quantidadePontos() const override { return PONTOS_POR_PACOTE; }

  /// @copydoc IDecodificadorLidar::ponto
  PontoLidar ponto(uint8_t indice) const override;

 private:
  void ressincronizar();
  bool extrairPontos();

  uint8_t buffer_[TAMANHO_PACOTE] = {};
  uint8_t recebidos_ = 0;
  PontoLidar pontos_[PONTOS_POR_PACOTE] = {};
};

}  // namespace micromouse
