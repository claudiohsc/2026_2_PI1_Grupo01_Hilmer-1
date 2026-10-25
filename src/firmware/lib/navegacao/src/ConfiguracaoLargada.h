#pragma once

/// @file ConfiguracaoLargada.h
/// @brief Parâmetros do estado "Aguardando Largada" (#242): quando o robô largou e a que ritmo ele avisa
/// que está esperando.
///
/// Valores PROVISÓRIOS, sem medida na bancada. Para mudar um valor, altere só este arquivo.

#include <stdint.h>

#include "Configuracao.h"

namespace micromouse {

/// Deslocamento desde a partida, em mm, a partir do qual o robô conta como largado. Fica acima da
/// oscilação da estimativa parado e abaixo de meia célula (90 mm): a largada é percebida nos primeiros
/// centímetros, e não só ao trocar de célula.
constexpr uint32_t DESLOCAMENTO_MINIMO_LARGADA_MM = 20;

/// Intervalo, em ms, entre duas amostras de telemetria enquanto o robô aguarda a largada. A primeira
/// sai logo ao entrar no estado; as seguintes só mostram que o robô continua vivo, então não precisam
/// do ritmo de 1 s da corrida (uma espera de 5 min viraria 300 amostras no buffer).
constexpr uint32_t PERIODO_AMOSTRA_AGUARDANDO_MS = 5000;

/// Parâmetros da `FaseCorrida`. Os testes alteram os campos para cobrir outros cenários.
struct ConfiguracaoLargada {
  uint32_t deslocamentoMinimoMm = DESLOCAMENTO_MINIMO_LARGADA_MM;
  uint32_t periodoAmostraAguardandoMs = PERIODO_AMOSTRA_AGUARDANDO_MS;
  uint32_t limiteTempoMs = LIMITE_TEMPO_CORRIDA_MS;
};

}  // namespace micromouse
