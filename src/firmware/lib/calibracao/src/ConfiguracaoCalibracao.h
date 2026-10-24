#pragma once

/// @file ConfiguracaoCalibracao.h
/// @brief Parâmetros da rotina de calibração dos VL53L0X e do giroscópio (#240, HU-13 / RF-13).
///
/// Orçamento total: **1 s** (RNF-08). A metade deste arquivo cobre os ToF; a parte do giroscópio e
/// a orquestração que mede a duração ficam para a outra metade da issue (Cláudio).
///
/// Valores **provisórios**: as distâncias de referência dependem da montagem e da célula de
/// partida. Confirme na bancada (#175) antes de confiar. Para mudar um valor, altere só este
/// arquivo.

#include <stdint.h>

#include "Tipos.h"

namespace micromouse {

/// Orçamento total da autocalibração, em ms (RNF-08).
constexpr uint32_t ORCAMENTO_CALIBRACAO_MS = 1000;

/// Fatia prevista para os ToF dentro do orçamento (deixando margem para o giroscópio).
constexpr uint32_t ORCAMENTO_CALIBRACAO_TOF_MS = 400;

/// Amostras da calibração de cada ToF. Com período de medição de ~33 ms (#344), 10 amostras cabem
/// em cerca de 330 ms, dentro de `ORCAMENTO_CALIBRACAO_TOF_MS`.
constexpr uint16_t AMOSTRAS_CALIBRACAO_TOF = 10;

/// Maior diferença entre a maior e a menor amostra de um lado, em mm. Acima dela, o robô
/// provavelmente se mexeu (ou o sensor oscilou demais) e a calibração daquele lado é recusada.
constexpr uint16_t VARIACAO_MAXIMA_CALIBRACAO_TOF_MM = 8;

/// Distância nominal esperada na face de cada sensor com o robô parado e centralizado na célula
/// de partida, em mm: `DISTANCIA_CENTRO_A_PAREDE_MM` (84) − offset provisório de montagem (30) =
/// 54 mm, o mesmo da percepção (#379). Ajuste se a referência da bancada for outra.
constexpr uint16_t DISTANCIA_REFERENCIA_TOF_MM = 54;

/// Distâncias de referência usadas na calibração (frente, esquerda, direita), em mm.
struct ReferenciasTof {
  uint16_t frenteMm = DISTANCIA_REFERENCIA_TOF_MM;
  uint16_t esquerdaMm = DISTANCIA_REFERENCIA_TOF_MM;
  uint16_t direitaMm = DISTANCIA_REFERENCIA_TOF_MM;
};

/// Parâmetros do `CalibradorTof`. Os testes alteram os campos para cobrir outros cenários.
struct ConfiguracaoCalibracaoTof {
  uint16_t amostras = AMOSTRAS_CALIBRACAO_TOF;
  uint16_t variacaoMaximaMm = VARIACAO_MAXIMA_CALIBRACAO_TOF_MM;
  ReferenciasTof referencias = {};
};

// ---------------------------------------------------------------------------
// Metade do Cláudio (#240): giroscópio + rotina unificada
// ---------------------------------------------------------------------------
// - Ajustar a calibração do viés do giro (média de amostras; abortar se o robô se mexer) ao
//   orçamento de 1 s. Em #378 a calibração usa 200 × 10 ms = 2 s — precisa caber em
//   `ORCAMENTO_CALIBRACAO_MS` junto com os ToF.
// - Orquestrar ToF (`CalibradorTof`) + giro (`IntegradorGiro` / `AcompanhadorGiro` da #378).
// - Medir a duração e falhar se passar de `ORCAMENTO_CALIBRACAO_MS`.
// - Testes com `ImuSimulado` + leituras simuladas dos ToF.

}  // namespace micromouse
