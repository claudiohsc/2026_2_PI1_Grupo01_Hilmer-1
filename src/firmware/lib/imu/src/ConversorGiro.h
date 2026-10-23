#pragma once

/// @file ConversorGiro.h
/// @brief Conversão da leitura bruta do giroscópio do MPU6050 para décimos de °/s.
///
/// Fica fora do driver para ser testada no computador: o driver só lê os dois bytes do eixo Z
/// e chama `converterBrutoParaDecimosDps`.

#include <stdint.h>

#include "ConfiguracaoImu.h"

namespace micromouse {

/// Sensibilidade do giroscópio multiplicada por 10, em LSB por °/s (131 → 1310), para a conta
/// ficar em inteiros.
constexpr uint16_t sensibilidadeGiroDecimaLsb(FaixaGiro faixa) {
  return faixa == FaixaGiro::Dps250    ? 1310
         : faixa == FaixaGiro::Dps500  ? 655
         : faixa == FaixaGiro::Dps1000 ? 328
                                       : 164;
}

/// Converte a leitura bruta (complemento de dois, 16 bits) em décimos de °/s, arredondando para
/// o mais próximo. Exemplo: na faixa de ±250 °/s, 131 LSB = 1 °/s = 10 décimos.
constexpr int16_t converterBrutoParaDecimosDps(int16_t bruto, FaixaGiro faixa) {
  // décimos = bruto × 10 / (LSB por °/s) = bruto × 100 / (sensibilidade × 10)
  const int32_t numerador = static_cast<int32_t>(bruto) * 100;
  const int32_t divisor = sensibilidadeGiroDecimaLsb(faixa);
  const int32_t metade = divisor / 2;
  return static_cast<int16_t>(numerador >= 0 ? (numerador + metade) / divisor : (numerador - metade) / divisor);
}

}  // namespace micromouse
