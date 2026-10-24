#pragma once

/// @file ConfiguracaoTof.h
/// @brief Parâmetros dos 3 sensores VL53L0X: tempo de medição, faixa válida e idade da leitura.
///
/// ATENÇÃO: os valores abaixo são PROVISÓRIOS. Vêm do datasheet do VL53L0X (ST, DocID029104) e
/// dos padrões da biblioteca `pololu/VL53L0X`; nada foi medido com os sensores montados no robô.
/// Valide na bancada (issue #175) antes de confiar. Para mudar um valor, altere só este arquivo.
///
/// As distâncias são medidas a partir da face de cada sensor, não do centro do robô. A conversão
/// para a geometria do robô e os limiares de parede ficam na percepção (issue #379).

#include <stdint.h>

namespace micromouse {

/// Tempo de medição de cada sensor (*timing budget*), em µs. 33 ms é o padrão do VL53L0X: cerca de
/// 30 leituras por segundo, com precisão razoável até 1,2 m. Valores menores (mínimo de 20 ms)
/// leem mais rápido e com mais ruído; maiores leem mais longe e mais devagar.
constexpr uint32_t TEMPO_MEDICAO_TOF_US = 33000;

/// Intervalo entre medições no modo contínuo, em ms. 0 = uma medição emendada na outra
/// (*back-to-back*), no ritmo do `TEMPO_MEDICAO_TOF_US`.
constexpr uint32_t PERIODO_MEDICAO_TOF_MS = 0;

/// Menor distância, em mm, considerada medida válida. Abaixo disso o VL53L0X perde a precisão
/// (o datasheet garante a medida a partir de cerca de 30 mm), e a leitura sai limitada a este valor.
constexpr uint16_t DISTANCIA_MINIMA_VALIDA_TOF_MM = 30;

/// Maior distância, em mm, considerada medida válida: o alcance do modo padrão em ambiente interno.
/// Sem alvo no alcance, o sensor devolve 8190 ou 8191 mm, que caem acima deste limite. Acima dele,
/// a leitura sai limitada a este valor (lado livre).
constexpr uint16_t DISTANCIA_MAXIMA_VALIDA_TOF_MM = 1200;

/// Idade máxima, em ms, da amostra mais antiga dos 3 sensores. Passando disso, a leitura falha:
/// o sensor parou de entregar medidas. Equivale a cerca de 3 medições perdidas.
constexpr uint32_t IDADE_MAXIMA_LEITURA_TOF_MS = 100;

/// Tempo máximo, em ms, de espera por uma resposta do sensor no barramento I²C.
constexpr uint16_t TEMPO_LIMITE_I2C_TOF_MS = 50;

/// Tempo, em ms, de espera depois de soltar o XSHUT de um sensor, antes de falar com ele.
/// O datasheet dá no máximo 1,2 ms de partida; a folga cobre o regulador do módulo.
constexpr uint16_t ESPERA_PARTIDA_TOF_MS = 10;

/// Parâmetros em uso pelo combinador e pelo driver. Os valores padrão vêm das constantes acima;
/// os testes alteram os campos para cobrir outros cenários.
struct ConfiguracaoTof {
  uint32_t tempoMedicaoUs = TEMPO_MEDICAO_TOF_US;
  uint32_t periodoMedicaoMs = PERIODO_MEDICAO_TOF_MS;
  uint16_t distanciaMinimaMm = DISTANCIA_MINIMA_VALIDA_TOF_MM;
  uint16_t distanciaMaximaMm = DISTANCIA_MAXIMA_VALIDA_TOF_MM;
  uint32_t idadeMaximaLeituraMs = IDADE_MAXIMA_LEITURA_TOF_MS;
  uint16_t tempoLimiteI2cMs = TEMPO_LIMITE_I2C_TOF_MS;
  uint16_t esperaPartidaMs = ESPERA_PARTIDA_TOF_MS;
};

}  // namespace micromouse
