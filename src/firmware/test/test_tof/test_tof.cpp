/// @file test_tof.cpp
/// @brief Testes unitários do `CombinadorTof` (#344): faixa válida, medidas fora da faixa limitadas,
/// falha de sensor, sensor sem amostra, idade da leitura e amostras brutas expostas por sensor. O driver `TofVl53l0x` só
/// repassa as medidas a esta classe, então a regra de validade é toda testada aqui.
/// Roda no computador: `pio test -e native -f test_tof`.

#include <unity.h>

#include "CombinadorTof.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

/// Registra uma medida válida nos 3 sensores, no mesmo instante.
void registrarTres(CombinadorTof& combinador, uint16_t frente, uint16_t esquerda, uint16_t direita,
                   uint32_t agoraMs) {
  combinador.registrar(Lado::Frente, frente, false, agoraMs);
  combinador.registrar(Lado::Esquerda, esquerda, false, agoraMs);
  combinador.registrar(Lado::Direita, direita, false, agoraMs);
}

}  // namespace

void test_sem_amostras_a_leitura_falha() {
  CombinadorTof combinador;
  DistanciasLaterais saida;
  TEST_ASSERT_FALSE(combinador.distancias(1000, saida));
  TEST_ASSERT_EQUAL_UINT32(CombinadorTof::IDADE_DESCONHECIDA, combinador.idadeMs(1000));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::SemAmostra, combinador.amostra(Lado::Frente).estado);
}

void test_tres_amostras_validas_montam_a_leitura_por_lado() {
  CombinadorTof combinador;
  registrarTres(combinador, 250, 40, 45, 1000);
  DistanciasLaterais saida;
  TEST_ASSERT_TRUE(combinador.distancias(1000, saida));
  TEST_ASSERT_EQUAL_UINT16(250, saida.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(40, saida.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(45, saida.direitaMm);
}

void test_classificacao_nos_limites_da_faixa() {
  CombinadorTof combinador;  // faixa padrão: 30 a 1200 mm
  TEST_ASSERT_EQUAL(EstadoAmostraTof::AbaixoDoMinimo, combinador.classificar(0, false));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::AbaixoDoMinimo, combinador.classificar(29, false));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::Valida, combinador.classificar(30, false));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::Valida, combinador.classificar(1200, false));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::ForaDeAlcance, combinador.classificar(1201, false));
  // Valores que o VL53L0X devolve sem alvo no alcance.
  TEST_ASSERT_EQUAL(EstadoAmostraTof::ForaDeAlcance, combinador.classificar(8190, false));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::ForaDeAlcance, combinador.classificar(8191, false));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::ForaDeAlcance, combinador.classificar(65535, false));
}

void test_falha_tem_prioridade_sobre_a_distancia() {
  CombinadorTof combinador;
  TEST_ASSERT_EQUAL(EstadoAmostraTof::Falha, combinador.classificar(100, true));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::Falha, combinador.classificar(65535, true));
}

void test_sensor_com_falha_invalida_a_leitura() {
  CombinadorTof combinador;
  registrarTres(combinador, 250, 40, 45, 1000);
  combinador.registrar(Lado::Esquerda, 0, true, 1010);
  DistanciasLaterais saida;
  TEST_ASSERT_FALSE(combinador.distancias(1010, saida));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::Falha, combinador.amostra(Lado::Esquerda).estado);
}

void test_fora_de_alcance_vale_limitado_ao_maximo_e_fica_exposto_por_sensor() {
  CombinadorTof combinador;
  registrarTres(combinador, 8190, 40, 45, 1000);
  DistanciasLaterais saida;
  // "Nada à frente" é uma leitura válida (lado livre, comum no 12x4): sai limitada a 1200 mm,
  // acima do limiar de parede da percepção (#379).
  TEST_ASSERT_TRUE(combinador.distancias(1000, saida));
  TEST_ASSERT_EQUAL_UINT16(1200, saida.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(40, saida.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(45, saida.direitaMm);

  // A amostra bruta continua disponível para quem precisa do caso exato.
  const AmostraTof& frente = combinador.amostra(Lado::Frente);
  TEST_ASSERT_EQUAL(EstadoAmostraTof::ForaDeAlcance, frente.estado);
  TEST_ASSERT_EQUAL_UINT16(8190, frente.distanciaMm);
  TEST_ASSERT_EQUAL_UINT32(1000, frente.instanteMs);
}

void test_abaixo_do_minimo_vale_limitado_ao_minimo() {
  CombinadorTof combinador;
  registrarTres(combinador, 250, 12, 45, 1000);
  DistanciasLaterais saida;
  // O sensor lateral perto da parede pode ler menos de 30 mm: vale como parede muito perto.
  TEST_ASSERT_TRUE(combinador.distancias(1000, saida));
  TEST_ASSERT_EQUAL_UINT16(30, saida.esquerdaMm);
  TEST_ASSERT_EQUAL(EstadoAmostraTof::AbaixoDoMinimo, combinador.amostra(Lado::Esquerda).estado);
  TEST_ASSERT_EQUAL_UINT16(12, combinador.amostra(Lado::Esquerda).distanciaMm);
}

void test_os_tres_lados_fora_da_faixa_ainda_montam_a_leitura() {
  CombinadorTof combinador;
  registrarTres(combinador, 8191, 0, 65535, 1000);
  DistanciasLaterais saida;
  TEST_ASSERT_TRUE(combinador.distancias(1000, saida));
  TEST_ASSERT_EQUAL_UINT16(1200, saida.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(30, saida.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(1200, saida.direitaMm);
}

void test_falha_derruba_a_leitura_mesmo_com_os_outros_fora_da_faixa() {
  CombinadorTof combinador;
  registrarTres(combinador, 8190, 12, 45, 1000);
  combinador.registrar(Lado::Direita, 0, true, 1000);
  DistanciasLaterais saida;
  TEST_ASSERT_FALSE(combinador.distancias(1000, saida));
}

void test_leitura_fora_da_faixa_velha_tambem_falha() {
  CombinadorTof combinador;  // idade máxima padrão: 100 ms
  registrarTres(combinador, 8190, 40, 45, 1000);
  DistanciasLaterais saida;
  TEST_ASSERT_TRUE(combinador.distancias(1100, saida));
  TEST_ASSERT_FALSE(combinador.distancias(1101, saida));
}

void test_sensor_que_nunca_mediu_invalida_a_leitura() {
  CombinadorTof combinador;
  combinador.registrar(Lado::Frente, 250, false, 1000);
  combinador.registrar(Lado::Esquerda, 40, false, 1000);
  DistanciasLaterais saida;
  TEST_ASSERT_FALSE(combinador.distancias(1000, saida));
  TEST_ASSERT_EQUAL_UINT32(CombinadorTof::IDADE_DESCONHECIDA, combinador.idadeMs(1000));
}

void test_a_idade_e_a_da_amostra_mais_antiga() {
  CombinadorTof combinador;
  combinador.registrar(Lado::Frente, 250, false, 1000);
  combinador.registrar(Lado::Esquerda, 40, false, 1030);
  combinador.registrar(Lado::Direita, 45, false, 1020);
  TEST_ASSERT_EQUAL_UINT32(50, combinador.idadeMs(1050));
}

void test_leitura_velha_falha_e_volta_quando_chega_amostra_nova() {
  CombinadorTof combinador;  // idade máxima padrão: 100 ms
  registrarTres(combinador, 250, 40, 45, 1000);
  DistanciasLaterais saida;
  TEST_ASSERT_TRUE(combinador.distancias(1100, saida));  // exatamente no limite
  TEST_ASSERT_FALSE(combinador.distancias(1101, saida));

  // Só a frente parou de medir: a leitura continua velha até ela voltar.
  combinador.registrar(Lado::Esquerda, 41, false, 1150);
  combinador.registrar(Lado::Direita, 44, false, 1150);
  TEST_ASSERT_FALSE(combinador.distancias(1150, saida));
  combinador.registrar(Lado::Frente, 240, false, 1160);
  TEST_ASSERT_TRUE(combinador.distancias(1160, saida));
  TEST_ASSERT_EQUAL_UINT16(240, saida.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(41, saida.esquerdaMm);
}

void test_leitura_volta_depois_de_uma_falha() {
  CombinadorTof combinador;
  registrarTres(combinador, 250, 40, 45, 1000);
  combinador.registrar(Lado::Direita, 0, true, 1033);
  DistanciasLaterais saida;
  TEST_ASSERT_FALSE(combinador.distancias(1033, saida));
  combinador.registrar(Lado::Direita, 46, false, 1066);
  TEST_ASSERT_TRUE(combinador.distancias(1066, saida));
  TEST_ASSERT_EQUAL_UINT16(46, saida.direitaMm);
}

void test_idade_correta_quando_o_relogio_da_a_volta() {
  CombinadorTof combinador;
  const uint32_t pertoDoFim = UINT32_MAX - 9;  // 10 ms antes de o contador voltar a 0
  registrarTres(combinador, 250, 40, 45, pertoDoFim);
  TEST_ASSERT_EQUAL_UINT32(30, combinador.idadeMs(20));
  DistanciasLaterais saida;
  TEST_ASSERT_TRUE(combinador.distancias(20, saida));
}

void test_reiniciar_esquece_as_amostras() {
  CombinadorTof combinador;
  registrarTres(combinador, 250, 40, 45, 1000);
  combinador.reiniciar();
  DistanciasLaterais saida;
  TEST_ASSERT_FALSE(combinador.distancias(1000, saida));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::SemAmostra, combinador.amostra(Lado::Direita).estado);
}

void test_configuracao_altera_faixa_e_idade() {
  ConfiguracaoTof config;
  config.distanciaMinimaMm = 50;
  config.distanciaMaximaMm = 500;
  config.idadeMaximaLeituraMs = 20;
  CombinadorTof combinador(config);
  TEST_ASSERT_EQUAL(EstadoAmostraTof::AbaixoDoMinimo, combinador.classificar(40, false));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::ForaDeAlcance, combinador.classificar(600, false));

  registrarTres(combinador, 300, 60, 60, 1000);
  DistanciasLaterais saida;
  TEST_ASSERT_TRUE(combinador.distancias(1020, saida));
  TEST_ASSERT_FALSE(combinador.distancias(1021, saida));

  // A limitação segue a faixa configurada, não a padrão.
  registrarTres(combinador, 600, 40, 60, 2000);
  TEST_ASSERT_TRUE(combinador.distancias(2000, saida));
  TEST_ASSERT_EQUAL_UINT16(500, saida.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(50, saida.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(60, saida.direitaMm);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_sem_amostras_a_leitura_falha);
  RUN_TEST(test_tres_amostras_validas_montam_a_leitura_por_lado);
  RUN_TEST(test_classificacao_nos_limites_da_faixa);
  RUN_TEST(test_falha_tem_prioridade_sobre_a_distancia);
  RUN_TEST(test_sensor_com_falha_invalida_a_leitura);
  RUN_TEST(test_fora_de_alcance_vale_limitado_ao_maximo_e_fica_exposto_por_sensor);
  RUN_TEST(test_abaixo_do_minimo_vale_limitado_ao_minimo);
  RUN_TEST(test_os_tres_lados_fora_da_faixa_ainda_montam_a_leitura);
  RUN_TEST(test_falha_derruba_a_leitura_mesmo_com_os_outros_fora_da_faixa);
  RUN_TEST(test_leitura_fora_da_faixa_velha_tambem_falha);
  RUN_TEST(test_sensor_que_nunca_mediu_invalida_a_leitura);
  RUN_TEST(test_a_idade_e_a_da_amostra_mais_antiga);
  RUN_TEST(test_leitura_velha_falha_e_volta_quando_chega_amostra_nova);
  RUN_TEST(test_leitura_volta_depois_de_uma_falha);
  RUN_TEST(test_idade_correta_quando_o_relogio_da_a_volta);
  RUN_TEST(test_reiniciar_esquece_as_amostras);
  RUN_TEST(test_configuracao_altera_faixa_e_idade);
  return UNITY_END();
}
