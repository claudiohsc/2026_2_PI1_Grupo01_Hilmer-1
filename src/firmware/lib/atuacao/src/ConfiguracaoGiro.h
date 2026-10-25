#pragma once

/// @file ConfiguracaoGiro.h
/// @brief Parâmetros de controle para giros de 90° e 180° em malha fechada com o giroscópio (#190).
///
/// Parâmetros PROVISÓRIOS: os ganhos, a tolerância e o tempo limite foram ajustados
/// em simulação e devem ser confirmados na bancada com o robô físico montado no labirinto.

#include <stdint.h>

namespace micromouse {

/// Parâmetros de controle do giro em malha fechada (`ControleGiro`).
struct ConfiguracaoGiro {
  /// Tolerância angular para considerar o giro concluído, em mili-graus.
  /// Padrão: 2 000 mili-graus (2,0°). Configurável para maior precisão ou rapidez.
  int32_t toleranciaMiligraus = 2000;

  /// Potência máxima aplicada nas rodas durante o giro.
  /// Valor conservador para limitar a aceleração angular e evitar derrapagem excessiva.
  int16_t potenciaMaxima = 350;

  /// Potência mínima aplicada nas rodas (para vencer a zona morta dos motores N20).
  /// Abaixo deste valor as rodas não vencem o atrito estático com o piso.
  int16_t potenciaMinima = 150;

  /// Ganho proporcional do controle de giro, em décimos.
  /// Potência adicional por grau de erro restante: P = (ganhoProporcionalDecimos * erroGraus) / 10.
  /// Com 40 (4,0 de potência por grau): um erro de 10° gera 40 de potência adicional sobre a mínima.
  int16_t ganhoProporcionalDecimos = 40;

  /// Tempo limite máximo para completar o giro, em milissegundos.
  /// Caso o robô fique preso ou os motores travem, o controle entra em timeout e freia.
  uint32_t tempoMaximoGiroMs = 2500;

  /// Número de ciclos consecutivos dentro da faixa de tolerância antes de declarar conclusão.
  /// A 10 ms por ciclo, 3 ciclos = 30 ms estável na meta, evitando falsos positivos por overshoot.
  uint8_t ciclosEstabilizacao = 3;
};

}  // namespace micromouse

