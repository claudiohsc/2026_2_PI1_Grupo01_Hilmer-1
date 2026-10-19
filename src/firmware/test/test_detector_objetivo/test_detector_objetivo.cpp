/// @file test_detector_objetivo.cpp
/// @brief Testes da detecção de chegada à célula objetivo e parada imediata dos motores (#189, HU-07).
/// Roda no computador: `pio test -e native -f test_detector_objetivo`.

#include <stdint.h>
#include <unity.h>

#include "ArmazenamentoMapa.h"
#include "DetectorObjetivo.h"
#include "MotoresSimulados.h"
#include "Tipos.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

// ---- 1. Cálculo da célula objetivo

void test_calcular_objetivo_por_dimensao() {
  const PosicaoCelula obj4x4 = DetectorObjetivo::calcularObjetivo(DimensaoMapa::Labirinto4x4);
  TEST_ASSERT_EQUAL_UINT8(3, obj4x4.linha);
  TEST_ASSERT_EQUAL_UINT8(3, obj4x4.coluna);

  const PosicaoCelula obj8x4 = DetectorObjetivo::calcularObjetivo(DimensaoMapa::Labirinto8x4);
  TEST_ASSERT_EQUAL_UINT8(3, obj8x4.linha);
  TEST_ASSERT_EQUAL_UINT8(7, obj8x4.coluna);

  const PosicaoCelula obj12x4 = DetectorObjetivo::calcularObjetivo(DimensaoMapa::Labirinto12x4);
  TEST_ASSERT_EQUAL_UINT8(3, obj12x4.linha);
  TEST_ASSERT_EQUAL_UINT8(11, obj12x4.coluna);
}

void test_calcular_objetivo_por_linhas_e_colunas() {
  const PosicaoCelula obj4x4 = DetectorObjetivo::calcularObjetivo(4, 4);
  TEST_ASSERT_EQUAL_UINT8(3, obj4x4.linha);
  TEST_ASSERT_EQUAL_UINT8(3, obj4x4.coluna);

  const PosicaoCelula obj8x4 = DetectorObjetivo::calcularObjetivo(4, 8);
  TEST_ASSERT_EQUAL_UINT8(3, obj8x4.linha);
  TEST_ASSERT_EQUAL_UINT8(7, obj8x4.coluna);

  const PosicaoCelula obj1x1 = DetectorObjetivo::calcularObjetivo(1, 1);
  TEST_ASSERT_EQUAL_UINT8(0, obj1x1.linha);
  TEST_ASSERT_EQUAL_UINT8(0, obj1x1.coluna);

  // Casos de borda: dimensões zero retornam (0, 0) com segurança
  const PosicaoCelula obj0x0 = DetectorObjetivo::calcularObjetivo(0, 0);
  TEST_ASSERT_EQUAL_UINT8(0, obj0x0.linha);
  TEST_ASSERT_EQUAL_UINT8(0, obj0x0.coluna);

  const PosicaoCelula obj0x5 = DetectorObjetivo::calcularObjetivo(0, 5);
  TEST_ASSERT_EQUAL_UINT8(0, obj0x5.linha);
  TEST_ASSERT_EQUAL_UINT8(0, obj0x5.coluna);

  const PosicaoCelula obj5x0 = DetectorObjetivo::calcularObjetivo(5, 0);
  TEST_ASSERT_EQUAL_UINT8(0, obj5x0.linha);
  TEST_ASSERT_EQUAL_UINT8(0, obj5x0.coluna);
}

// ---- 2. Detecção estática

void test_alcancou_objetivo_estatico() {
  const PosicaoCelula objetivo = {3, 3};
  TEST_ASSERT_TRUE(DetectorObjetivo::alcancouObjetivo({3, 3}, objetivo));
  TEST_ASSERT_FALSE(DetectorObjetivo::alcancouObjetivo({0, 0}, objetivo));
  TEST_ASSERT_FALSE(DetectorObjetivo::alcancouObjetivo({3, 2}, objetivo));
  TEST_ASSERT_FALSE(DetectorObjetivo::alcancouObjetivo({2, 3}, objetivo));
}

// ---- 3. Construtores e atualização de objetivo

void test_construtores_e_definicao_objetivo() {
  // Construtor padrão (4x4)
  DetectorObjetivo detPadrao;
  TEST_ASSERT_EQUAL_UINT8(3, detPadrao.celulaObjetivo().linha);
  TEST_ASSERT_EQUAL_UINT8(3, detPadrao.celulaObjetivo().coluna);

  // Construtor com DimensaoMapa
  DetectorObjetivo det12x4(DimensaoMapa::Labirinto12x4);
  TEST_ASSERT_EQUAL_UINT8(3, det12x4.celulaObjetivo().linha);
  TEST_ASSERT_EQUAL_UINT8(11, det12x4.celulaObjetivo().coluna);

  // Construtor explícito por PosicaoCelula
  DetectorObjetivo detExplicito(PosicaoCelula{2, 5});
  TEST_ASSERT_EQUAL_UINT8(2, detExplicito.celulaObjetivo().linha);
  TEST_ASSERT_EQUAL_UINT8(5, detExplicito.celulaObjetivo().coluna);

  // Construtor por linhas e colunas
  DetectorObjetivo detDimensoes(4, 8);
  TEST_ASSERT_EQUAL_UINT8(3, detDimensoes.celulaObjetivo().linha);
  TEST_ASSERT_EQUAL_UINT8(7, detDimensoes.celulaObjetivo().coluna);

  // Atualização por definirObjetivo
  detPadrao.definirObjetivo(PosicaoCelula{1, 2});
  TEST_ASSERT_EQUAL_UINT8(1, detPadrao.celulaObjetivo().linha);
  TEST_ASSERT_EQUAL_UINT8(2, detPadrao.celulaObjetivo().coluna);

  detPadrao.definirObjetivo(DimensaoMapa::Labirinto8x4);
  TEST_ASSERT_EQUAL_UINT8(3, detPadrao.celulaObjetivo().linha);
  TEST_ASSERT_EQUAL_UINT8(7, detPadrao.celulaObjetivo().coluna);

  detPadrao.definirObjetivo(4, 12);
  TEST_ASSERT_EQUAL_UINT8(3, detPadrao.celulaObjetivo().linha);
  TEST_ASSERT_EQUAL_UINT8(11, detPadrao.celulaObjetivo().coluna);
}

// ---- 4. Detecção por célula e pose em diferentes orientações

void test_alcancou_objetivo_por_pose_em_qualquer_orientacao() {
  DetectorObjetivo detector(DimensaoMapa::Labirinto4x4);  // Objetivo: (3, 3)

  // Em qualquer orientação na célula (3, 3), alcançou o objetivo
  TEST_ASSERT_TRUE(detector.alcancouObjetivo(Pose{{3, 3}, Direcao::Norte}));
  TEST_ASSERT_TRUE(detector.alcancouObjetivo(Pose{{3, 3}, Direcao::Leste}));
  TEST_ASSERT_TRUE(detector.alcancouObjetivo(Pose{{3, 3}, Direcao::Sul}));
  TEST_ASSERT_TRUE(detector.alcancouObjetivo(Pose{{3, 3}, Direcao::Oeste}));

  // Posição diferente não alcança
  TEST_ASSERT_FALSE(detector.alcancouObjetivo(Pose{{0, 0}, Direcao::Norte}));
  TEST_ASSERT_FALSE(detector.alcancouObjetivo(Pose{{3, 2}, Direcao::Leste}));
  TEST_ASSERT_FALSE(detector.alcancouObjetivo(Pose{{2, 3}, Direcao::Sul}));
}

// ---- 5. Parada imediata de motores e congelamento de mapa

void test_processar_chegada_para_motores_imediatamente_e_congela_mapa() {
  DetectorObjetivo detector(DimensaoMapa::Labirinto4x4);  // Objetivo: (3, 3)
  MotoresSimulados motores;
  ArmazenamentoMapa armazenamento;
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto4x4);

  // Robô em movimento (potência aplicada)
  motores.definirPotencia({400, 400});
  TEST_ASSERT_FALSE(motores.freados());
  TEST_ASSERT_FALSE(armazenamento.mapaCongelado());
  TEST_ASSERT_FALSE(detector.objetivoAlcancado());

  // 1. Pose ainda distante: não para motores nem congela mapa
  const bool chegouOrigem = detector.processarChegada(Pose{{0, 0}, Direcao::Norte}, motores, &armazenamento);
  TEST_ASSERT_FALSE(chegouOrigem);
  TEST_ASSERT_FALSE(motores.freados());
  TEST_ASSERT_EQUAL_INT16(400, motores.ultimaPotencia().esquerda);
  TEST_ASSERT_FALSE(armazenamento.mapaCongelado());
  TEST_ASSERT_FALSE(detector.objetivoAlcancado());

  // 2. Pose intermediária (2, 3): não para motores
  const bool chegouMeio = detector.processarChegada(Pose{{2, 3}, Direcao::Norte}, motores, &armazenamento);
  TEST_ASSERT_FALSE(chegouMeio);
  TEST_ASSERT_FALSE(motores.freados());
  TEST_ASSERT_FALSE(armazenamento.mapaCongelado());
  TEST_ASSERT_FALSE(detector.objetivoAlcancado());

  // 3. Chegada à célula objetivo (3, 3): para os motores imediatamente e congela o mapa!
  const bool alcancou = detector.processarChegada(Pose{{3, 3}, Direcao::Leste}, motores, &armazenamento);
  TEST_ASSERT_TRUE(alcancou);
  TEST_ASSERT_TRUE(detector.objetivoAlcancado());
  TEST_ASSERT_TRUE(motores.freados());
  TEST_ASSERT_EQUAL_INT16(0, motores.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(0, motores.ultimaPotencia().direita);
  TEST_ASSERT_TRUE(armazenamento.mapaCongelado());
  TEST_ASSERT_NOT_NULL(armazenamento.mapaFinal());
}

void test_processar_chegada_idempotente_mantem_parado() {
  DetectorObjetivo detector(DimensaoMapa::Labirinto4x4);
  MotoresSimulados motores;
  ArmazenamentoMapa armazenamento;
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto4x4);

  // Chega ao objetivo
  detector.processarChegada(Pose{{3, 3}, Direcao::Norte}, motores, &armazenamento);
  TEST_ASSERT_TRUE(detector.objetivoAlcancado());
  TEST_ASSERT_TRUE(motores.freados());

  // Próxima chamada (ciclos subsequentes de 10 ms): mantém motores parados
  const uint32_t comandosAntes = motores.comandos();
  const bool repetido = detector.processarChegada(Pose{{3, 3}, Direcao::Norte}, motores, &armazenamento);
  TEST_ASSERT_TRUE(repetido);
  TEST_ASSERT_TRUE(motores.freados());
  TEST_ASSERT_EQUAL_UINT32(comandosAntes + 1, motores.comandos());
}

void test_resetar_detector_objetivo() {
  DetectorObjetivo detector(DimensaoMapa::Labirinto4x4);
  MotoresSimulados motores;
  detector.processarChegada(Pose{{3, 3}, Direcao::Norte}, motores);
  TEST_ASSERT_TRUE(detector.objetivoAlcancado());

  detector.resetar();
  TEST_ASSERT_FALSE(detector.objetivoAlcancado());
}

void test_processar_chegada_sem_armazenamento_opcional() {
  DetectorObjetivo detector(DimensaoMapa::Labirinto4x4);
  MotoresSimulados motores;
  motores.definirPotencia({300, 300});

  // Funciona com armazenamento = nullptr sem falhas
  const bool alcancou = detector.processarChegada(Pose{{3, 3}, Direcao::Norte}, motores, nullptr);
  TEST_ASSERT_TRUE(alcancou);
  TEST_ASSERT_TRUE(motores.freados());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_calcular_objetivo_por_dimensao);
  RUN_TEST(test_calcular_objetivo_por_linhas_e_colunas);
  RUN_TEST(test_alcancou_objetivo_estatico);
  RUN_TEST(test_construtores_e_definicao_objetivo);
  RUN_TEST(test_alcancou_objetivo_por_pose_em_qualquer_orientacao);
  RUN_TEST(test_processar_chegada_para_motores_imediatamente_e_congela_mapa);
  RUN_TEST(test_processar_chegada_idempotente_mantem_parado);
  RUN_TEST(test_resetar_detector_objetivo);
  RUN_TEST(test_processar_chegada_sem_armazenamento_opcional);
  return UNITY_END();
}
