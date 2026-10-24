/// @file test_sensor_distancia_simulado.cpp
/// @brief Testes unitários do `SensorDistanciaSimulado`: leitura fixa, fila em ordem, falha simulada,
/// fila cheia e uso pela interface `ISensorDistancia`. Roda no computador:
/// `pio test -e native -f test_simulacao`.

#include <unity.h>

#include "SensorDistanciaSimulado.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

/// Monta uma leitura com as distâncias, em mm, à frente, à esquerda e à direita.
static DistanciasLaterais leitura(uint16_t frente, uint16_t esquerda, uint16_t direita) {
  return DistanciasLaterais{frente, esquerda, direita};
}

void test_sem_leitura_configurada_a_leitura_falha() {
  SensorDistanciaSimulado lidar;
  DistanciasLaterais saida = {};
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));
}

void test_leitura_fixa_se_repete_a_cada_chamada() {
  SensorDistanciaSimulado lidar;
  lidar.definirLeituraFixa(leitura(90, 90, 270));
  DistanciasLaterais saida = {};
  for (int i = 0; i < 3; i++) {
    TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
    TEST_ASSERT_EQUAL_UINT16(90, saida.frenteMm);
    TEST_ASSERT_EQUAL_UINT16(90, saida.esquerdaMm);
    TEST_ASSERT_EQUAL_UINT16(270, saida.direitaMm);
  }
}

void test_fila_entrega_as_leituras_em_ordem_e_depois_volta_para_a_fixa() {
  SensorDistanciaSimulado lidar;
  lidar.definirLeituraFixa(leitura(500, 500, 500));
  TEST_ASSERT_TRUE(lidar.enfileirarLeitura(leitura(100, 0, 0)));
  TEST_ASSERT_TRUE(lidar.enfileirarLeitura(leitura(200, 0, 0)));

  DistanciasLaterais saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(100, saida.frenteMm);
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(200, saida.frenteMm);
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(500, saida.frenteMm);
}

void test_falha_simulada_retorna_false_e_nao_consome_a_fila() {
  SensorDistanciaSimulado lidar;
  lidar.enfileirarLeitura(leitura(100, 100, 100));
  DistanciasLaterais saida = {};

  lidar.simularFalha(true);
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));

  lidar.simularFalha(false);
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(100, saida.frenteMm);
}

void test_fila_cheia_recusa_novas_leituras() {
  SensorDistanciaSimulado lidar;
  for (uint8_t i = 0; i < SensorDistanciaSimulado::CAPACIDADE_FILA; i++) {
    TEST_ASSERT_TRUE(lidar.enfileirarLeitura(leitura(i, 0, 0)));
  }
  TEST_ASSERT_FALSE(lidar.enfileirarLeitura(leitura(999, 0, 0)));
}

void test_pode_ser_usado_pela_interface_ISensorDistancia() {
  SensorDistanciaSimulado simulado;
  simulado.definirLeituraFixa(leitura(180, 180, 180));
  ISensorDistancia& lidar = simulado;
  DistanciasLaterais saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(180, saida.frenteMm);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_sem_leitura_configurada_a_leitura_falha);
  RUN_TEST(test_leitura_fixa_se_repete_a_cada_chamada);
  RUN_TEST(test_fila_entrega_as_leituras_em_ordem_e_depois_volta_para_a_fixa);
  RUN_TEST(test_falha_simulada_retorna_false_e_nao_consome_a_fila);
  RUN_TEST(test_fila_cheia_recusa_novas_leituras);
  RUN_TEST(test_pode_ser_usado_pela_interface_ISensorDistancia);
  return UNITY_END();
}
