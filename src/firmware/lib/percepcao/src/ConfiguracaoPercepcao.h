#pragma once

/// @file ConfiguracaoPercepcao.h
/// @brief Geometria dos sensores ToF (VL53L0X), limiares de classificação parede/livre e filtros (#379).
///
/// Substituição do LiDAR por 3 sensores VL53L0X (Rev 2.0 de 21/10/2026).
///
/// ==============================================================================
/// 1. GEOMETRIA DO LABIRINTO E DO CHASSI
/// ==============================================================================
/// - Tamanho da célula da competição: 180 mm (`TAMANHO_CELULA_MM`, `Configuracao.h`).
/// - Espessura das paredes do labirinto: 12 mm (`ESPESSURA_PAREDE_MM`, `ConfiguracaoControle.h`).
/// - Largura livre do corredor: 180 - 12 = 168 mm.
/// - Distância do centro do robô a cada parede com o robô centralizado:
///     d_centro_parede = (180 - 12) / 2 = 84 mm (`DISTANCIA_CENTRO_A_PAREDE_MM`).
/// - Meia largura do chassi: 60 mm (`MEIA_LARGURA_CHASSI_MM`).
/// - Meio comprimento do chassi: 50 mm (`MEIO_COMPRIMENTO_CHASSI_MM`).
///
/// ==============================================================================
/// 2. CARACTERÍSTICAS OPERACIONAIS DO SENSOR VL53L0X
/// ==============================================================================
/// - Distância mínima confiável: ~30 mm (`DISTANCIA_MINIMA_TOF_MM`).
///   Abaixo disso o sensor perde precisão ou satura.
/// - Distância máxima confiável em ambiente interno: ~1200 mm (`DISTANCIA_MAXIMA_TOF_MM`).
/// - Fora de alcance: quando não há obstáculo refletindo sinal na faixa operacional,
///   o sensor ou o driver retorna códigos como 8190/8191 mm ou valores acima do alcance máximo.
///
/// ==============================================================================
/// 3. CÁLCULO DOS LIMIARES DE DECISÃO (PAREDE vs LIVRE)
/// ==============================================================================
/// A classificação decide entre:
///   (a) Parede imediata na célula atual;
///   (b) Passagem livre (parede apenas na célula seguinte, ou corredor aberto).
///
/// Para um sensor com offset d_offset em relação ao centro do robô em direção à parede:
///   - Leitura nominal com parede imediata (robô centralizado):
///       d_parede = d_centro_parede - d_offset = 84 mm - d_offset
///   - Leitura nominal com passagem livre (parede na próxima célula):
///       d_livre = d_parede + TAMANHO_CELULA_MM = d_parede + 180 mm
///   - Limiar ideal de decisão (ponto médio):
///       limiar = (d_parede + d_livre) / 2 = d_parede + 90 mm
///
/// Sensores Laterais (Esquerdo e Direito):
///   - Se os sensores estivessem na borda extrema (d_offset = 60 mm), a leitura seria:
///       84 mm - 60 mm = 24 mm.
///     Como 24 mm é menor que o mínimo confiável do VL53L0X (30 mm), a montagem mecânica
///     necessariamente posiciona os sensores recuados em relação à borda extrema.
///   - Posição provisória adotada: offset lateral de 30 mm (recuado 30 mm para o interior).
///       d_parede_lateral = 84 - 30 = 54 mm (bem acima dos 30 mm mínimos).
///       d_livre_lateral = 54 + 180 = 234 mm (ou fora de alcance se corredor aberto).
///       limiar_lateral_calculado = 54 + 90 = 144 mm (adotado provisoriamente: 140 mm).
///
/// Sensor Frontal:
///   - Com chassi de 100 mm (meio comprimento = 50 mm), com offset frontal provisório de 30 mm:
///       d_parede_frente = 84 - 30 = 54 mm.
///       d_livre_frente = 54 + 180 = 234 mm.
///       limiar_frente_calculado = 54 + 90 = 144 mm (adotado provisoriamente: 140 mm).
///   - No ambiente simulado ideal (`LabirintoSimulado`), a parede imediata fica a 90 mm e a
///     livre a >= 270 mm. Com o limiar em 140 mm:
///       90 mm <= 140 mm -> Parede (correto!)
///       270 mm > 140 mm -> Livre (correto!)
///
/// ==============================================================================
/// 4. TRATAMENTO DE BORDAS E RUÍDOS
/// ==============================================================================
/// - Fora de alcance (> 1200 mm ou >= 8190 mm): interpretado como passagem livre,
///   sendo limitado superiormente a `DISTANCIA_MAXIMA_TOF_MM` para processamento.
/// - Abaixo do mínimo (< 30 mm): interpretado como parede muito próxima (ou robô encostado),
///   sendo limitado inferiormente a `DISTANCIA_MINIMA_TOF_MM` para processamento.
/// - Filtro: descarta leituras espúrias usando mediana de pequenas amostras (padrão)
///   ou média aritmética.

#include <stdint.h>
#include <stddef.h>
#include "Configuracao.h"

namespace micromouse {

/// Menor distância confiável do VL53L0X, em mm (datasheet ST DocID029104).
constexpr uint16_t DISTANCIA_MINIMA_TOF_MM = 30;

/// Alcance máximo confiável do VL53L0X em ambiente interno, em mm.
constexpr uint16_t DISTANCIA_MAXIMA_TOF_MM = 1200;

/// Código retornado pelo VL53L0X quando não há obstáculo no alcance (fora de alcance).
constexpr uint16_t DISTANCIA_OUT_OF_RANGE_TOF_MM = 8190;

/// Limiar provisório frontal de detecção de parede, em mm.
/// Calculado para offset frontal de ~30 mm: ponto de decisão entre 54 mm (parede) e 234 mm (livre).
constexpr uint16_t LIMIAR_PADRAO_FRENTE_MM = 140;

/// Limiar provisório esquerdo de detecção de parede, em mm.
/// Calculado para offset lateral de ~30 mm: ponto de decisão entre 54 mm (parede) e 234 mm (livre).
constexpr uint16_t LIMIAR_PADRAO_ESQUERDA_MM = 140;

/// Limiar provisório direito de detecção de parede, em mm.
/// Calculado para offset lateral de ~30 mm: ponto de decisão entre 54 mm (parede) e 234 mm (livre).
constexpr uint16_t LIMIAR_PADRAO_DIREITA_MM = 140;

/// Capacidade máxima do buffer estático de filtragem de amostras, sem alocação dinâmica.
constexpr size_t MAX_AMOSTRAS_FILTRO_PERCEPCAO = 16;

/// Tipo de filtro digital aplicado às amostras de distância.
enum class TipoFiltroPercepcao : uint8_t {
  Mediana,  ///< Filtro de mediana: imune a outliers pontuais (leituras espúrias).
  Media     ///< Filtro de média aritmética simples.
};

/// Estrutura de configuração da camada de percepção.
struct ConfiguracaoPercepcao {
  uint16_t limiarFrenteMm = LIMIAR_PADRAO_FRENTE_MM;
  uint16_t limiarEsquerdaMm = LIMIAR_PADRAO_ESQUERDA_MM;
  uint16_t limiarDireitaMm = LIMIAR_PADRAO_DIREITA_MM;
  uint16_t distanciaMinimaMm = DISTANCIA_MINIMA_TOF_MM;
  uint16_t distanciaMaximaMm = DISTANCIA_MAXIMA_TOF_MM;
  TipoFiltroPercepcao tipoFiltro = TipoFiltroPercepcao::Mediana;
};

}  // namespace micromouse

