/// @file test_labirinto_simulado.cpp
/// @brief Testes unitários do `LabirintoSimulado` e do modo labirinto do `LidarSimulado`.
/// Roda no computador: `pio test -e native -f test_labirinto_simulado`.
///
/// Esqueleto inicial: cada teste lista um critério de pronto da tarefa e fica ignorado
/// (`TEST_IGNORE_MESSAGE`) até ser implementado. Teste ignorado não reprova o CI.

#include <unity.h>

#include "LabirintoSimulado.h"
#include "LabirintosExemplo.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

// ---- Carga do texto e validação

void test_carrega_os_exemplos_dos_tres_tamanhos() {
  TEST_IGNORE_MESSAGE("a implementar: LABIRINTO_4X4, _8X4 e _12X4 devolvem Carregado, com linhas e colunas certas");
}

void test_recusa_tamanho_fora_da_competicao() {
  TEST_IGNORE_MESSAGE("a implementar: 3x3, 4x5 e linhas de larguras diferentes devolvem TamanhoInvalido");
}

void test_recusa_caractere_invalido() {
  TEST_IGNORE_MESSAGE("a implementar: um 'x' no lugar de '|' devolve CaractereInvalido");
}

void test_recusa_parede_incompleta() {
  TEST_IGNORE_MESSAGE("a implementar: '- -' ou '-  ' no lugar de '---' devolve ParedeIncompleta");
}

void test_recusa_perimetro_aberto() {
  TEST_IGNORE_MESSAGE("a implementar: espaço na borda externa devolve PerimetroAberto");
}

void test_texto_invalido_mantem_o_labirinto_anterior() {
  TEST_IGNORE_MESSAGE("a implementar: carregar texto inválido depois de um válido não altera o labirinto");
}

// ---- Consulta de paredes

void test_tem_parede_concorda_entre_celulas_vizinhas() {
  TEST_IGNORE_MESSAGE("a implementar: nos três exemplos, temParede(c, d) == temParede(vizinha, oposta(d))");
}

void test_tem_parede_fora_do_labirinto_e_verdadeiro() {
  TEST_IGNORE_MESSAGE("a implementar: célula fora do labirinto conta como parede");
}

// ---- Distâncias (robô no centro da célula: 90 mm + 180 mm por célula livre)

void test_corredor() {
  TEST_IGNORE_MESSAGE("a implementar: corredor reto com paredes dos lados");
}

void test_curva() {
  TEST_IGNORE_MESSAGE("a implementar: curva, com parede à frente e passagem para um dos lados");
}

void test_beco_sem_saida() {
  TEST_IGNORE_MESSAGE("a implementar: beco sem saída, 90 mm nos três lados");
}

void test_labirinto_4x4_completo() {
  TEST_IGNORE_MESSAGE("a implementar: leituras esperadas da tabela em LabirintosExemplo.h");
}

void test_distancias_fora_do_labirinto_ou_sem_carga_falham() {
  TEST_IGNORE_MESSAGE("a implementar: distancias() devolve false e não altera a saída");
}

// ---- LidarSimulado no modo labirinto

void test_lidar_posicionado_le_o_que_o_labirinto_mostra() {
  TEST_IGNORE_MESSAGE("a implementar: usarLabirinto + posicionar(pose) e lerDistanciasLaterais igual a distancias(pose)");
}

void test_lidar_no_labirinto_respeita_falha_e_fila() {
  TEST_IGNORE_MESSAGE("a implementar: falha e fila continuam com prioridade sobre o labirinto");
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_carrega_os_exemplos_dos_tres_tamanhos);
  RUN_TEST(test_recusa_tamanho_fora_da_competicao);
  RUN_TEST(test_recusa_caractere_invalido);
  RUN_TEST(test_recusa_parede_incompleta);
  RUN_TEST(test_recusa_perimetro_aberto);
  RUN_TEST(test_texto_invalido_mantem_o_labirinto_anterior);
  RUN_TEST(test_tem_parede_concorda_entre_celulas_vizinhas);
  RUN_TEST(test_tem_parede_fora_do_labirinto_e_verdadeiro);
  RUN_TEST(test_corredor);
  RUN_TEST(test_curva);
  RUN_TEST(test_beco_sem_saida);
  RUN_TEST(test_labirinto_4x4_completo);
  RUN_TEST(test_distancias_fora_do_labirinto_ou_sem_carga_falham);
  RUN_TEST(test_lidar_posicionado_le_o_que_o_labirinto_mostra);
  RUN_TEST(test_lidar_no_labirinto_respeita_falha_e_fila);
  return UNITY_END();
}
