#pragma once

/// @file ConfiguracaoExecucao.h
/// @brief Parâmetros da execução dos movimentos entre células (#184): avanço de uma célula, giros,
/// percepção no centro da célula e gatilho da largada.
///
/// ATENÇÃO: todos os valores são PROVISÓRIOS. Foram ajustados na simulação dos testes
/// (`test_execucao_movimentos` e `test_integracao_execucao`), com a tabela de velocidade e a
/// geometria provisórias dos outros módulos. O erro de deslocamento real é medido na bancada
/// (`scripts/bancada_deslocamento.py`, comandos de célula e de giro). Para mudar um valor, altere
/// só este arquivo.

#include <stdint.h>

#include "ConfiguracaoDeslocamento.h"

namespace micromouse {

// ---------------------------------------------------------------------------------------------
// 1. Geometria dos sensores para o controle do avanço
// ---------------------------------------------------------------------------------------------

/// Distância do centro do robô até a face dos sensores laterais, em mm. O `ControleMovimento`
/// trabalha com distâncias a partir do centro do robô (84 mm até a parede com ele centralizado),
/// e o ToF mede a partir da face do sensor. Mesmo recuo da percepção (`ConfiguracaoPercepcao.h`:
/// 84 − 30 = 54 mm lidos com parede). A frente usa o recuo do deslocamento estimado
/// (`DISTANCIA_SENSOR_FRONTAL_AO_CENTRO_MM`), o mesmo da correção pela parede da frente.
constexpr uint16_t DISTANCIA_SENSOR_LATERAL_AO_CENTRO_MM = 30;

// ---------------------------------------------------------------------------------------------
// 2. Avanço de uma célula
// ---------------------------------------------------------------------------------------------

/// Distância até o centro da célula de destino em que o avanço termina, em mm. O robô para
/// quando faltar isto ou menos (ou quando passar do centro).
constexpr uint16_t TOLERANCIA_CHEGADA_MM = 3;

/// Distância antes do centro de destino em que a rampa de desaceleração começa, em mm. Na rampa, a
/// potência base cai de `potenciaCruzeiro` até `potenciaMinima` (`ConfiguracaoControle`).
constexpr uint16_t DISTANCIA_DESACELERACAO_MM = 60;

/// Se a parede da frente fizer o robô parar a até esta distância do centro de destino, em mm, o
/// avanço conta como concluído: a parede é a melhor referência do centro. Mais longe, o avanço
/// termina como `ParedeNaFrente` (uma parede que o mapa ainda não tinha).
constexpr uint16_t FOLGA_PARADA_NA_PAREDE_MM = 40;

/// Tempo máximo de um avanço de uma célula, em ms. A 45 mm/s (potência mínima), 180 mm levam 4 s.
constexpr uint32_t TEMPO_MAXIMO_AVANCO_MS = 6000;

// ---------------------------------------------------------------------------------------------
// 3. Percepção no centro da célula
// ---------------------------------------------------------------------------------------------

/// Leituras novas do ToF, com o robô parado no centro, que entram na classificação das paredes
/// (mediana do `ClassificadorParede`). A 33 ms por leitura, 3 leituras levam cerca de 0,1 s.
constexpr uint8_t AMOSTRAS_PERCEPCAO = 3;

// ---------------------------------------------------------------------------------------------
// 4. Gatilho da largada: a mão na frente do ToF frontal
// ---------------------------------------------------------------------------------------------

/// Leitura frontal, em mm, a partir da qual a frente conta como livre (a mão saiu). Fica acima da
/// parede imediata (cerca de 34 mm com o sensor a 50 mm do centro): uma parede parada na frente
/// nunca arma o gatilho, e a largada fica só pelo comando serial.
constexpr uint16_t LIMIAR_FRENTE_LIVRE_GATILHO_MM = 100;

/// Leitura frontal, em mm, até a qual a frente conta como coberta pela mão.
constexpr uint16_t LIMIAR_FRENTE_COBERTA_GATILHO_MM = 50;

/// Tempo mínimo com a frente coberta, em ms, para valer como gesto (um vulto rápido não larga).
constexpr uint32_t TEMPO_MINIMO_COBERTO_GATILHO_MS = 300;

/// Espera depois de a mão sair, em ms, antes de largar: dá tempo de tirar a mão do caminho.
constexpr uint32_t ESPERA_APOS_GESTO_GATILHO_MS = 1000;

// ---------------------------------------------------------------------------------------------
// Parâmetros em uso (os testes alteram os campos para cobrir outros cenários)
// ---------------------------------------------------------------------------------------------

/// Parâmetros do `ExecutorMovimentos` e do `CicloNavegacao`.
struct ConfiguracaoExecucao {
  uint16_t distanciaSensorFrontalAoCentroMm = DISTANCIA_SENSOR_FRONTAL_AO_CENTRO_MM;
  uint16_t distanciaSensorLateralAoCentroMm = DISTANCIA_SENSOR_LATERAL_AO_CENTRO_MM;
  uint16_t toleranciaChegadaMm = TOLERANCIA_CHEGADA_MM;
  uint16_t distanciaDesaceleracaoMm = DISTANCIA_DESACELERACAO_MM;
  uint16_t folgaParadaNaParedeMm = FOLGA_PARADA_NA_PAREDE_MM;
  uint32_t tempoMaximoAvancoMs = TEMPO_MAXIMO_AVANCO_MS;
  uint8_t amostrasPercepcao = AMOSTRAS_PERCEPCAO;
};

/// Parâmetros do `GatilhoLargada`.
struct ConfiguracaoGatilhoLargada {
  uint16_t limiarFrenteLivreMm = LIMIAR_FRENTE_LIVRE_GATILHO_MM;
  uint16_t limiarFrenteCobertaMm = LIMIAR_FRENTE_COBERTA_GATILHO_MM;
  uint32_t tempoMinimoCobertoMs = TEMPO_MINIMO_COBERTO_GATILHO_MS;
  uint32_t esperaAposGestoMs = ESPERA_APOS_GESTO_GATILHO_MS;
};

}  // namespace micromouse
