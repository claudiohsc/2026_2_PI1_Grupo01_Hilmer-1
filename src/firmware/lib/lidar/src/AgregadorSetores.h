#pragma once

/// @file AgregadorSetores.h
/// @brief Transforma os pontos de uma volta do LiDAR em distâncias frente/esquerda/direita.

#include <stdint.h>

#include "ConfiguracaoLidar.h"
#include "IDecodificadorLidar.h"
#include "Tipos.h"

namespace micromouse {

/// Agrupa os pontos de cada volta do LiDAR em três setores e calcula a mediana de cada um.
///
/// Funcionamento:
/// 1. Cada ponto é convertido para o ângulo em relação à frente do robô, usando
///    `deslocamentoFrenteCentiGraus` e `anguloCrescenteHorario` da configuração.
/// 2. Pontos fora de `[distanciaMinimaMm, distanciaMaximaMm]` são descartados.
/// 3. Os demais entram no setor da frente (0°), da direita (90°) ou da esquerda (270°),
///    se estiverem a até `meiaLarguraSetorCentiGraus` do centro do setor.
/// 4. Uma volta termina quando o ângulo dá o salto de ~360° para ~0° (ou o inverso).
///    Nesse momento calcula-se a mediana de cada setor.
///
/// A primeira volta após a criação (ou `reiniciar()`) é descartada, pois começa no meio.
/// Sem alocação dinâmica: cada setor guarda até `CAPACIDADE_SETOR` pontos por volta. Pontos
/// além disso são ignorados.
class AgregadorSetores {
 public:
  /// Máximo de pontos válidos guardados por setor em uma volta.
  static constexpr uint16_t CAPACIDADE_SETOR = 256;

  /// @param config Parâmetros de calibração. A meia largura é limitada a 44,99°, para os
  ///        setores não se sobreporem.
  explicit AgregadorSetores(const ConfiguracaoLidar& config = ConfiguracaoLidar{});

  /// Entrega um ponto, na ordem em que o sensor o mediu.
  /// @return `true` se este ponto fechou uma volta completa (o resultado dela está em
  ///         `voltaValida()` e `distancias()`); `false` caso contrário.
  bool adicionarPonto(const PontoLidar& ponto);

  /// `true` se a última volta fechada tinha pontos válidos suficientes nos três setores.
  bool voltaValida() const { return voltaValida_; }

  /// Distâncias da última volta válida, em mm. Só tem significado quando `voltaValida()` é `true`.
  const DistanciasLaterais& distancias() const { return distancias_; }

  /// Esquece tudo o que foi acumulado e volta a descartar a próxima volta.
  void reiniciar();

 private:
  static constexpr uint8_t NUM_SETORES = 3;

  int8_t setorDoAngulo(uint16_t anguloCentiGraus) const;
  void acumular(uint16_t anguloCentiGraus, uint16_t distanciaMm);
  bool fecharVolta();
  void limparAcumuladores();

  ConfiguracaoLidar config_;
  uint16_t valores_[NUM_SETORES][CAPACIDADE_SETOR];
  uint16_t contagem_[NUM_SETORES];
  uint16_t ultimoAngulo_ = 0;
  bool temUltimoAngulo_ = false;
  bool descartarProximaVolta_ = true;
  bool voltaValida_ = false;
  DistanciasLaterais distancias_ = {0, 0, 0};
};

}  // namespace micromouse
