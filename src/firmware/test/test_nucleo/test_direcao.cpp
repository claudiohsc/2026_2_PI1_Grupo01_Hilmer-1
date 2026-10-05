/// @file test_direcao.cpp
/// @brief Testes unitários de `Direcao.h`: giros de 90° e 180° e conversão de lado relativo
/// em direção absoluta. Roda no computador: `pio test -e native -f test_nucleo`.
///
/// Também serve de modelo para novos testes (veja "Padrão de testes" no README).

#include <unity.h>

#include "Direcao.h"

using namespace micromouse;

/// Compara duas direções pelo valor numérico, já que o Unity não conhece o enum `Direcao`.
#define ASSERT_DIRECAO(esperada, obtida) \
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(esperada), static_cast<uint8_t>(obtida))

void setUp() {}
void tearDown() {}

void test_girar_direita_percorre_as_quatro_direcoes() {
  ASSERT_DIRECAO(Direcao::Leste, girarDireita(Direcao::Norte));
  ASSERT_DIRECAO(Direcao::Sul, girarDireita(Direcao::Leste));
  ASSERT_DIRECAO(Direcao::Oeste, girarDireita(Direcao::Sul));
  ASSERT_DIRECAO(Direcao::Norte, girarDireita(Direcao::Oeste));
}

void test_girar_esquerda_desfaz_girar_direita() {
  ASSERT_DIRECAO(Direcao::Oeste, girarEsquerda(Direcao::Norte));
  ASSERT_DIRECAO(Direcao::Norte, girarEsquerda(girarDireita(Direcao::Norte)));
}

void test_oposta_inverte_a_direcao() {
  ASSERT_DIRECAO(Direcao::Sul, oposta(Direcao::Norte));
  ASSERT_DIRECAO(Direcao::Leste, oposta(Direcao::Oeste));
}

void test_direcao_absoluta_converte_lado_do_robo_em_direcao_do_labirinto() {
  ASSERT_DIRECAO(Direcao::Leste, direcaoAbsoluta(Direcao::Leste, Lado::Frente));
  ASSERT_DIRECAO(Direcao::Norte, direcaoAbsoluta(Direcao::Leste, Lado::Esquerda));
  ASSERT_DIRECAO(Direcao::Sul, direcaoAbsoluta(Direcao::Leste, Lado::Direita));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_girar_direita_percorre_as_quatro_direcoes);
  RUN_TEST(test_girar_esquerda_desfaz_girar_direita);
  RUN_TEST(test_oposta_inverte_a_direcao);
  RUN_TEST(test_direcao_absoluta_converte_lado_do_robo_em_direcao_do_labirinto);
  return UNITY_END();
}
