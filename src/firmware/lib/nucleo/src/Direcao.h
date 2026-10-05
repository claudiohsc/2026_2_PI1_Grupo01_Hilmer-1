#pragma once

/// @file Direcao.h
/// @brief Operações sobre direções: girar o robô e converter lados relativos em direções absolutas.
///
/// Todas as funções são `constexpr`: podem ser avaliadas em tempo de compilação
/// e não têm efeitos colaterais. Dependem da ordem horária de `Direcao`
/// (Norte, Leste, Sul, Oeste), usando soma módulo 4 (`& 3`).

#include "Tipos.h"

namespace micromouse {

/// Direção resultante de um giro de 90° para a direita (sentido horário).
/// @param d Direção atual.
/// @return Próxima direção no sentido horário (ex.: Norte → Leste).
constexpr Direcao girarDireita(Direcao d) {
  return static_cast<Direcao>((static_cast<uint8_t>(d) + 1) & 3);
}

/// Direção resultante de um giro de 90° para a esquerda (sentido anti-horário).
/// @param d Direção atual.
/// @return Próxima direção no sentido anti-horário (ex.: Norte → Oeste).
constexpr Direcao girarEsquerda(Direcao d) {
  return static_cast<Direcao>((static_cast<uint8_t>(d) + 3) & 3);
}

/// Direção oposta, equivalente a um giro de 180°.
/// @param d Direção atual.
/// @return Direção oposta (ex.: Norte → Sul).
constexpr Direcao oposta(Direcao d) {
  return static_cast<Direcao>((static_cast<uint8_t>(d) + 2) & 3);
}

/// Converte um lado relativo ao robô (frente, esquerda, direita) na direção absoluta do labirinto.
///
/// Usada para gravar no mapa as paredes lidas pelo LiDAR: com o robô virado para
/// Leste, a parede à esquerda está ao Norte da célula.
/// @param direcaoDoRobo Direção para a qual o robô está virado.
/// @param lado Lado relativo ao robô.
/// @return Direção absoluta correspondente.
constexpr Direcao direcaoAbsoluta(Direcao direcaoDoRobo, Lado lado) {
  return lado == Lado::Frente     ? direcaoDoRobo
         : lado == Lado::Esquerda ? girarEsquerda(direcaoDoRobo)
                                  : girarDireita(direcaoDoRobo);
}

}  // namespace micromouse
