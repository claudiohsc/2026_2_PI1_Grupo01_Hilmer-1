/// @file test_integracao_colisao.cpp
/// @brief Testes de integração da detecção de colisão e da recuperação (#192): o robô simulado anda
/// num labirinto de paredes sólidas, bate de verdade e o firmware só vê o que veria na placa.
///
/// A cadeia é a de `SimulacaoCorrida.h`, com as paredes sólidas (`paredesSolidas`): o chassi que
/// encostaria numa parede fica parado, as rodas patinam e o encontrão chega ao `ImuSimulado` como
/// um pico de aceleração. O `DetectorColisao` lê o acelerômetro, a última potência comandada e o
/// ToF frontal; o `CicloNavegacao` recebe o resultado, recua e volta a perceber e decidir.
///
/// Para o robô bater é preciso que ele não veja a parede da frente: os testes cegam o ToF frontal
/// (`cegarTofFrontal`), como numa falha do sensor ou de leitura no vazio.
/// Roda no computador: `pio test -e native -f test_integracao_colisao`.

#include <unity.h>

#include <cstdio>

#include "../test_integracao_execucao/SimulacaoCorrida.h"
#include "../test_labirinto_simulado/LabirintosExemplo.h"

using namespace micromouse;
using namespace micromouse::simulacao_corrida;

void setUp() {}
void tearDown() {}

namespace {

OpcoesSimulacao comColisao() {
  OpcoesSimulacao o;
  o.robo.paredesSolidas = true;
  o.detectarColisao = true;
  return o;
}

void registrar(const char* texto) { TEST_MESSAGE(texto); }

/// Quantas paredes o mapa já conhece. Com `semParedeFalsa`, toda parede gravada tem que existir no
/// labirinto: o ToF cego grava "livre" onde há parede (o sensor mente), mas a recuperação nunca pode
/// gravar uma parede que não existe, porque isso fecharia um caminho livre.
uint32_t paredesConhecidas(const Mapa& mapa, const LabirintoSimulado& labirinto, bool semParedeFalsa) {
  uint32_t conhecidas = 0;
  for (uint8_t l = 0; l < labirinto.obterLinhas(); l++) {
    for (uint8_t c = 0; c < labirinto.obterColunas(); c++) {
      for (uint8_t d = 0; d < 4; d++) {
        const Direcao direcao = static_cast<Direcao>(d);
        const EstadoParede estado = mapa.obterParede({l, c}, direcao);
        if (estado == EstadoParede::Desconhecido) {
          continue;
        }
        conhecidas++;
        if (semParedeFalsa && estado == EstadoParede::Parede) {
          char msg[96];
          std::snprintf(msg, sizeof(msg), "parede falsa em (%u,%u) lado %u", l, c, d);
          TEST_ASSERT_TRUE_MESSAGE(labirinto.temParede({l, c}, direcao), msg);
        }
      }
    }
  }
  return conhecidas;
}

void conferirChegada(Simulacao& sim, const char* nome) {
  char msg[220];
  const PosicaoCelula real = sim.robo().celula();
  std::snprintf(msg, sizeof(msg),
                "%s: estado %s em %.1f s, %u celulas, %u colisoes (%u recuperadas, %u paredes por colisao), %u "
                "encontroes; real (%u,%u)",
                nome, nomeDoEstadoCiclo(sim.ciclo().estado()), sim.agoraMs() / 1000.0,
                static_cast<unsigned>(sim.ciclo().celulasAvancadas()), static_cast<unsigned>(sim.ciclo().colisoes()),
                static_cast<unsigned>(sim.ciclo().recuperacoes()), static_cast<unsigned>(sim.ciclo().paredesPorColisao()),
                static_cast<unsigned>(sim.robo().encontroes()), real.linha, real.coluna);
  registrar(msg);
  TEST_ASSERT_EQUAL_MESSAGE(EstadoCiclo::ObjetivoAlcancado, sim.ciclo().estado(), msg);
  const PosicaoCelula objetivo = sim.ciclo().navegacao().obterObjetivo();
  TEST_ASSERT_EQUAL_UINT8(objetivo.linha, real.linha);
  TEST_ASSERT_EQUAL_UINT8(objetivo.coluna, real.coluna);
  TEST_ASSERT_EQUAL_UINT8(objetivo.linha, sim.odometria().pose().celula.linha);
  TEST_ASSERT_EQUAL_UINT8(objetivo.coluna, sim.odometria().pose().celula.coluna);
  TEST_ASSERT_LESS_THAN_UINT32(DEZ_MINUTOS_MS, sim.agoraMs());
}

}  // namespace

// ---- Sem batida, sem colisão

/// O robô que enxerga as paredes nunca bate, e a detecção nunca dispara: nem o acelerômetro (partidas
/// e frenagens normais) nem o ToF frontal (corredor mais longo que o alcance do sensor, no 12×4).
void test_corrida_sem_batidas_nao_detecta_colisao() {
  const char* nomes[] = {"4x4", "8x4", "12x4"};
  const char* labirintos[] = {exemplos::LABIRINTO_4X4, exemplos::LABIRINTO_8X4, exemplos::LABIRINTO_12X4};
  for (int i = 0; i < 3; i++) {
    Simulacao sim(labirintos[i], comColisao());
    sim.correr();
    conferirChegada(sim, nomes[i]);
    TEST_ASSERT_EQUAL_UINT32(0, sim.ciclo().colisoes());
    TEST_ASSERT_EQUAL_UINT32(0, sim.detector().colisoes());
    TEST_ASSERT_EQUAL_UINT32(0, sim.robo().encontroes());
    TEST_ASSERT_FALSE(sim.robo().colidiu());
  }
}

// ---- A batida e a recuperação

/// Critérios 1 a 3: sem enxergar a parede da frente, o robô bate; o acelerômetro percebe, ele recua,
/// percebe de novo e o mapa e a célula da navegação continuam como estavam.
void test_batida_e_detectada_pelo_acelerometro_e_o_robo_recua() {
  Simulacao sim(exemplos::LABIRINTO_4X4, comColisao());
  sim.cegarTofFrontal(true);

  // Corre até a primeira colisão ser tratada.
  while (sim.ciclo().colisoes() == 0 && sim.agoraMs() < DEZ_MINUTOS_MS) {
    sim.passoCiclo();
  }
  TEST_ASSERT_EQUAL_UINT32(1, sim.ciclo().colisoes());
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, sim.ciclo().ultimaColisao());
  TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, sim.ciclo().estado());
  TEST_ASSERT_TRUE(sim.robo().encontroes() >= 1);
  const PosicaoCelula celula = sim.ciclo().navegacao().obterPosicao();
  const Direcao direcao = sim.ciclo().direcao();
  const uint32_t conhecidas = paredesConhecidas(sim.mapa(), sim.labirinto(), false);

  // Recua e volta a perceber, na mesma célula e direção, sem apagar nada do mapa.
  while (sim.ciclo().estado() == EstadoCiclo::Recuperando && sim.agoraMs() < DEZ_MINUTOS_MS) {
    sim.passoCiclo();
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, sim.ciclo().estado());
  TEST_ASSERT_EQUAL_UINT32(1, sim.ciclo().recuperacoes());
  TEST_ASSERT_EQUAL_UINT8(celula.linha, sim.ciclo().navegacao().obterPosicao().linha);
  TEST_ASSERT_EQUAL_UINT8(celula.coluna, sim.ciclo().navegacao().obterPosicao().coluna);
  TEST_ASSERT_EQUAL(direcao, sim.ciclo().direcao());
  TEST_ASSERT_EQUAL_UINT32(conhecidas, paredesConhecidas(sim.mapa(), sim.labirinto(), false));
  TEST_ASSERT_FALSE_MESSAGE(sim.robo().preso(), "o robô recuou e saiu da parede");
  // O robô voltou para dentro da célula, sem passar do centro para trás.
  char msg[120];
  std::snprintf(msg, sizeof(msg), "depois do recuo: real (%u,%u), erro ao longo %.1f mm, lateral %.1f mm",
                sim.robo().celula().linha, sim.robo().celula().coluna, sim.robo().erroAoLongoMm(),
                sim.robo().erroLateralMm());
  registrar(msg);
  TEST_ASSERT_TRUE_MESSAGE(sim.robo().erroAoLongoMm() < 5.0 && sim.robo().erroAoLongoMm() > -30.0, msg);
}

/// Critério 2 e 3, na corrida inteira: com o ToF frontal cego, o robô bate nas paredes que não vê,
/// recupera-se de cada batida, grava a parede só quando a batida se repete e chega ao objetivo. Nenhuma
/// parede gravada é falsa: a recuperação não corrompe o mapa.
void test_corrida_com_tof_frontal_cego_recupera_de_cada_batida_e_chega_ao_objetivo() {
  Simulacao sim(exemplos::LABIRINTO_4X4, comColisao());
  sim.cegarTofFrontal(true);
  sim.correr();
  conferirChegada(sim, "4x4 com ToF frontal cego");
  TEST_ASSERT_GREATER_THAN_UINT32(0, sim.ciclo().colisoes());
  TEST_ASSERT_EQUAL_UINT32(sim.ciclo().colisoes(), sim.detector().colisoes());
  TEST_ASSERT_EQUAL_UINT32(sim.ciclo().colisoes(), sim.ciclo().recuperacoes());
  TEST_ASSERT_GREATER_THAN_UINT32(0, sim.ciclo().paredesPorColisao());
  TEST_ASSERT_GREATER_THAN_UINT32(0, paredesConhecidas(sim.mapa(), sim.labirinto(), true));
}

/// O mesmo no 8×4, que tem mais paredes no caminho.
void test_corrida_com_tof_frontal_cego_no_8x4() {
  Simulacao sim(exemplos::LABIRINTO_8X4, comColisao());
  sim.cegarTofFrontal(true);
  sim.correr();
  conferirChegada(sim, "8x4 com ToF frontal cego");
  TEST_ASSERT_GREATER_THAN_UINT32(0, sim.ciclo().colisoes());
  TEST_ASSERT_EQUAL_UINT32(sim.ciclo().colisoes(), sim.ciclo().recuperacoes());
  TEST_ASSERT_GREATER_THAN_UINT32(0, paredesConhecidas(sim.mapa(), sim.labirinto(), true));
}

/// Sem o detector ligado, o mesmo robô cego fica empurrando a parede: a odometria, que soma o que os
/// motores mandaram, acha que ele andou, e ele "chega" ao objetivo na estimativa, mas parado em
/// outra célula. O teste mostra o que a detecção resolve.
void test_sem_deteccao_o_robo_cego_acha_que_andou_mas_fica_preso_na_parede() {
  OpcoesSimulacao o;
  o.robo.paredesSolidas = true;
  Simulacao sim(exemplos::LABIRINTO_4X4, o);
  sim.cegarTofFrontal(true);
  sim.correr(60000);
  const PosicaoCelula objetivo = sim.ciclo().navegacao().obterObjetivo();
  const PosicaoCelula real = sim.robo().celula();
  TEST_ASSERT_EQUAL_UINT32(0, sim.ciclo().colisoes());
  TEST_ASSERT_TRUE_MESSAGE(sim.robo().encontroes() >= 1, "o robô bateu na parede");
  TEST_ASSERT_TRUE_MESSAGE(real.linha != objetivo.linha || real.coluna != objetivo.coluna,
                           "sem a detecção, o robô não está no objetivo");
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_corrida_sem_batidas_nao_detecta_colisao);
  RUN_TEST(test_batida_e_detectada_pelo_acelerometro_e_o_robo_recua);
  RUN_TEST(test_corrida_com_tof_frontal_cego_recupera_de_cada_batida_e_chega_ao_objetivo);
  RUN_TEST(test_corrida_com_tof_frontal_cego_no_8x4);
  RUN_TEST(test_sem_deteccao_o_robo_cego_acha_que_andou_mas_fica_preso_na_parede);
  return UNITY_END();
}
