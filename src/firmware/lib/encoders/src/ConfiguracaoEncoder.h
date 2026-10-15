#pragma once

/// @file ConfiguracaoEncoder.h
/// @brief Parâmetros de calibração dos encoders de roda.
///
/// ATENÇÃO: o valor abaixo é PROVISÓRIO. Nada foi medido com o robô montado: o diâmetro final da
/// roda, a redução da caixa e a resolução do encoder ainda não estão fechados nos documentos.
/// A calibração na bancada (issue #186, critério 2) define o valor real: ande uma distância
/// conhecida, leia os pulsos e use `ConversorDeslocamento::calcularMicrometrosPorPulso`.
/// Para mudar o valor, altere só este arquivo.

#include <stdint.h>

namespace micromouse {

/// Distância percorrida pela roda por pulso contado (decodificação x4), em micrômetros.
/// Valor ilustrativo: motor N20 com redução 1:100, encoder de 7 pulsos por volta do eixo do
/// motor e roda de 30 mm de diâmetro dão cerca de 34 µm por pulso. Em inteiros, para não depender
/// de ponto flutuante no ESP32-C3, que não tem unidade de ponto flutuante.
constexpr uint32_t MICROMETROS_POR_PULSO_PROVISORIO = 34;

}  // namespace micromouse
