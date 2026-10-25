#pragma once

/// @file ConversorAceleracao.h
/// @brief Conversão da leitura bruta do acelerômetro do MPU6050 para milésimos de g.
///
/// Fica fora do driver para ser testada no computador, como o `ConversorGiro.h`: o driver só lê os
/// bytes dos eixos X e Y e chama `juntarBytesAceleracao` e `converterBrutoParaMg`.

#include <stdint.h>

#include "ConfiguracaoImu.h"

namespace micromouse {

/// Sensibilidade do acelerômetro, em LSB por g.
constexpr uint16_t sensibilidadeAcelerometroLsbPorG(FaixaAcelerometro faixa) {
  return faixa == FaixaAcelerometro::G2   ? 16384
         : faixa == FaixaAcelerometro::G4 ? 8192
         : faixa == FaixaAcelerometro::G8 ? 4096
                                          : 2048;
}

/// Converte a leitura bruta (complemento de dois, 16 bits) em milésimos de g, arredondando para o
/// mais próximo. Exemplo: na faixa de ±4 g, 8192 LSB = 1 g = 1000 mg.
constexpr int16_t converterBrutoParaMg(int16_t bruto, FaixaAcelerometro faixa) {
  const int32_t numerador = static_cast<int32_t>(bruto) * 1000;
  const int32_t divisor = sensibilidadeAcelerometroLsbPorG(faixa);
  const int32_t metade = divisor / 2;
  return static_cast<int16_t>(numerador >= 0 ? (numerador + metade) / divisor : (numerador - metade) / divisor);
}

/// Valor do registrador ACCEL_CONFIG do MPU6050 para a faixa: o campo AFS_SEL fica nos bits 4 e 3
/// (0x00, 0x08, 0x10 e 0x18), e os bits de autoteste ficam em zero.
constexpr uint8_t valorAccelConfig(FaixaAcelerometro faixa) { return static_cast<uint8_t>(static_cast<uint8_t>(faixa) << 3); }

/// Junta os dois bytes lidos do MPU6050 (o mais significativo vem primeiro) num inteiro de 16 bits
/// com sinal, em complemento de dois.
constexpr int16_t juntarBytesAceleracao(uint8_t alto, uint8_t baixo) {
  return static_cast<int16_t>(static_cast<uint16_t>((static_cast<uint16_t>(alto) << 8) | baixo));
}

}  // namespace micromouse
