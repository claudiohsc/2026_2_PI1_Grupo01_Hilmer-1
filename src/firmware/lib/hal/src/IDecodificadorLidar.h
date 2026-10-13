#pragma once

/// @file IDecodificadorLidar.h
/// @brief Interface do decodificador do protocolo serial do LiDAR (camada `hal`).
///
/// Só o decodificador conhece o formato dos pacotes do sensor (cabeçalho, tamanho,
/// verificação de erro). O resto do driver trabalha com `PontoLidar`.

#include <stdint.h>

namespace micromouse {

/// Uma medida do LiDAR: um ângulo e a distância medida nele.
struct PontoLidar {
  uint16_t anguloCentiGraus;  ///< Ângulo na convenção do sensor, em centésimos de grau (0 a 35999).
  uint16_t distanciaMm;       ///< Distância em mm; 0 significa que não houve retorno.
};

/// Transforma os bytes recebidos do sensor em pontos.
class IDecodificadorLidar {
 public:
  virtual ~IDecodificadorLidar() = default;

  /// Entrega um byte ao decodificador, na ordem em que chegou do sensor.
  /// Pacotes com erro (cabeçalho ou verificação inválidos) são descartados em silêncio.
  /// @return `true` se este byte completou um pacote válido. Os pontos dele ficam
  ///         disponíveis em `ponto()` até o próximo `alimentar()`.
  virtual bool alimentar(uint8_t byte) = 0;

  /// Quantidade de pontos do último pacote completo.
  virtual uint8_t quantidadePontos() const = 0;

  /// Ponto de índice `indice` (de 0 a `quantidadePontos() - 1`) do último pacote completo.
  virtual PontoLidar ponto(uint8_t indice) const = 0;
};

}  // namespace micromouse
