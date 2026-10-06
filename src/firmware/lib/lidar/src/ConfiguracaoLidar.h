#pragma once

/// @file ConfiguracaoLidar.h
/// @brief Parâmetros de calibração do LiDAR: setores, filtros e tempos.
///
/// ATENÇÃO: os valores abaixo são PROVISÓRIOS. A convenção de ângulo vem do manual do LD06 (o
/// anúncio do ST-L50B2 diz que o protocolo é igual); o restante é estimativa. Nada foi medido
/// com o sensor montado. Valide na bancada (issue #93) antes de confiar. Para mudar um valor,
/// altere só este arquivo.

#include <stdint.h>

namespace micromouse {

/// Ângulo do sensor (na convenção dele) que aponta para a frente do robô.
/// No LD06, o 0° é a frente do sensor (a seta no topo). Com a seta apontando para a frente do
/// robô, o valor é 0. Confirmar como o sensor será fixado na torre e testar na bancada.
constexpr uint16_t DESLOCAMENTO_FRENTE_CENTI_GRAUS = 0;

/// `true` se o ângulo do sensor cresce no sentido horário, visto de cima (assim é no LD06).
/// Se estiver errado, esquerda e direita trocam de lugar. Confirmar na bancada.
constexpr bool ANGULO_CRESCE_NO_SENTIDO_HORARIO = true;

/// Meia largura de cada setor (frente, esquerda, direita), em centésimos de grau.
/// 1500 = ±15°. Com ±30°, o setor da frente enxergaria as paredes laterais de um corredor.
constexpr uint16_t MEIA_LARGURA_SETOR_CENTI_GRAUS = 1500;

/// Menor distância, em mm, considerada medida válida. Abaixo disso o ponto é descartado.
constexpr uint16_t DISTANCIA_MINIMA_VALIDA_MM = 30;

/// Maior distância, em mm, considerada medida válida. Acima disso o ponto é descartado.
constexpr uint16_t DISTANCIA_MAXIMA_VALIDA_MM = 4000;

/// Quantidade mínima de pontos válidos que cada setor precisa ter em uma volta.
constexpr uint8_t MINIMO_PONTOS_POR_SETOR = 3;

/// Idade máxima, em ms, da última volta completa. Passando disso, a leitura falha.
constexpr uint32_t IDADE_MAXIMA_LEITURA_MS = 300;

/// Máximo de bytes lidos da UART a cada chamada de `lerDistanciasLaterais()`.
/// Impede que um fluxo contínuo de bytes prenda a tarefa de navegação.
constexpr uint16_t MAX_BYTES_POR_CHAMADA = 1024;

/// Parâmetros em uso pelo agregador e pelo driver. Os valores padrão vêm das constantes acima;
/// os testes alteram os campos para cobrir outras montagens do sensor.
struct ConfiguracaoLidar {
  uint16_t deslocamentoFrenteCentiGraus = DESLOCAMENTO_FRENTE_CENTI_GRAUS;
  bool anguloCrescenteHorario = ANGULO_CRESCE_NO_SENTIDO_HORARIO;
  uint16_t meiaLarguraSetorCentiGraus = MEIA_LARGURA_SETOR_CENTI_GRAUS;
  uint16_t distanciaMinimaMm = DISTANCIA_MINIMA_VALIDA_MM;
  uint16_t distanciaMaximaMm = DISTANCIA_MAXIMA_VALIDA_MM;
  uint8_t minimoPontosPorSetor = MINIMO_PONTOS_POR_SETOR;
  uint32_t idadeMaximaLeituraMs = IDADE_MAXIMA_LEITURA_MS;
  uint16_t maxBytesPorChamada = MAX_BYTES_POR_CHAMADA;
};

}  // namespace micromouse
