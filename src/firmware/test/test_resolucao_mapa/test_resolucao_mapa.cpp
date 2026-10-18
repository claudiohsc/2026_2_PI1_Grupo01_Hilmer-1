/// @file test_resolucao_mapa.cpp
/// @brief Testes da Issue #183 (HU-01): Algoritmo de resolução sobre o mapa (Flood Fill).
/// Roda no computador: `pio test -e native -f test_resolucao_mapa`.

#include <unity.h>

#include "AtualizadorMapa.h"
#include "ClassificadorParede.h"
#include "Direcao.h"
#include "LabirintoSimulado.h"
#include "LidarSimulado.h"
#include "Mapa.h"
#include "Navegacao.h"
#include "../test_labirinto_simulado/LabirintosExemplo.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

PosicaoCelula pos(uint8_t linha, uint8_t coluna) {
  return PosicaoCelula{linha, coluna};
}

}  // namespace

/// @brief Verifica se as distâncias calculadas no mapa sem paredes internas correspondem à distância de Manhattan.
void test_flood_fill_mapa_vazio_4x4() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);  // Objetivo padrão: (3, 3)

  TEST_ASSERT_EQUAL_UINT8(3, navegacao.obterObjetivo().linha);
  TEST_ASSERT_EQUAL_UINT8(3, navegacao.obterObjetivo().coluna);

  // Distância do objetivo a si mesmo é 0
  TEST_ASSERT_EQUAL_UINT8(0, navegacao.obterDistancia(pos(3, 3)));

  // Em mapa sem paredes internas, a distância até (3, 3) é (3 - linha) + (3 - coluna)
  for (uint8_t l = 0; l < 4; ++l) {
    for (uint8_t c = 0; c < 4; ++c) {
      uint8_t esperado = (3 - l) + (3 - c);
      TEST_ASSERT_EQUAL_UINT8(esperado, navegacao.obterDistancia(pos(l, c)));
    }
  }

  // Da origem (0, 0) virado para o Norte, deve preferir Norte (continua em frente)
  Direcao dir;
  TEST_ASSERT_TRUE(navegacao.decidirProximaDirecao(dir));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Norte), static_cast<uint8_t>(dir));
}

/// @brief Verifica se o algoritmo contorna uma parede conhecida.
void test_contorno_parede_conhecida() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);

  // Bloqueia a passagem de (0, 0) para o Norte (1, 0)
  mapa.registrarParede(pos(0, 0), Direcao::Norte, EstadoParede::Parede);

  Navegacao navegacao(mapa);
  navegacao.moverPara(0, 0, Direcao::Norte);

  // Como o Norte está bloqueado por parede, a próxima direção viável para (3, 3) deve ser Leste
  Direcao dir;
  TEST_ASSERT_TRUE(navegacao.decidirProximaDirecao(dir));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Leste), static_cast<uint8_t>(dir));
}

/// @brief Verifica que, quando não há rota acessível até o objetivo, a decisão falha graciosamente.
void test_sem_caminho_ate_objetivo() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);

  // Enclausura a célula de partida (0, 0)
  mapa.registrarParede(pos(0, 0), Direcao::Norte, EstadoParede::Parede);
  mapa.registrarParede(pos(0, 0), Direcao::Leste, EstadoParede::Parede);

  Navegacao navegacao(mapa);
  navegacao.moverPara(0, 0, Direcao::Norte);

  TEST_ASSERT_EQUAL_UINT8(Navegacao::DISTANCIA_INFINITA, navegacao.obterDistancia(pos(0, 0)));

  Direcao dir;
  TEST_ASSERT_FALSE(navegacao.decidirProximaDirecao(dir));
}

/// @brief Verifica se o robô atinge o objetivo e encerra a navegação.
void test_atingimento_de_objetivo() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);

  // Move diretamente para o objetivo
  navegacao.moverPara(3, 3, Direcao::Norte);

  TEST_ASSERT_TRUE(navegacao.atingiuObjetivo());

  Direcao dir;
  TEST_ASSERT_FALSE(navegacao.decidirProximaDirecao(dir));
  TEST_ASSERT_FALSE(navegacao.darPasso(dir));
}

/// @brief Critério de Pronto da Issue #183:
/// Navega autonomamente no labirinto 4x4 simulado da competição (com paredes desconhecidas a priori),
/// atualizando o mapa sensorialmente a cada passo até alcançar a célula objetivo (3, 3).
void test_resolucao_labirinto_4x4_simulado() {
  LabirintoSimulado labirinto;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoCarga::Carregado),
                          static_cast<uint8_t>(labirinto.carregar(exemplos::LABIRINTO_4X4)));

  LidarSimulado lidar;
  lidar.usarLabirinto(labirinto);

  ClassificadorParede classificador;
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);

  constexpr uint16_t MAX_PASSOS = 50;
  uint16_t passos = 0;

  while (!navegacao.atingiuObjetivo() && passos < MAX_PASSOS) {
    const Pose poseAtual = navegacao.obterPose();
    lidar.posicionar(poseAtual);

    DistanciasLaterais leitura = {};
    TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(leitura));

    // Atualiza o mapa com as paredes percebidas na célula atual
    AtualizadorMapa::atualizar(mapa, poseAtual, classificador, &leitura, 1);

    // Decide a próxima direção e executa o passo no mapa
    Direcao proxDir;
    TEST_ASSERT_TRUE(navegacao.darPasso(proxDir));

    passos++;
  }

  TEST_ASSERT_TRUE(navegacao.atingiuObjetivo());
  TEST_ASSERT_EQUAL_UINT8(3, navegacao.obterPosicao().linha);
  TEST_ASSERT_EQUAL_UINT8(3, navegacao.obterPosicao().coluna);
  TEST_ASSERT_TRUE(passos < MAX_PASSOS);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_flood_fill_mapa_vazio_4x4);
  RUN_TEST(test_contorno_parede_conhecida);
  RUN_TEST(test_sem_caminho_ate_objetivo);
  RUN_TEST(test_atingimento_de_objetivo);
  RUN_TEST(test_resolucao_labirinto_4x4_simulado);
  return UNITY_END();
}
