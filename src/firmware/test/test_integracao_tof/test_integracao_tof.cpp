/// @file test_integracao_tof.cpp
/// @brief Integração ToF → mapa (#344): o robô simulado explora o labirinto lendo os 3 VL53L0X
/// pelas amostras brutas, e o mapa final deve ser idêntico ao labirinto.
/// Roda no computador: `pio test -e native -f test_integracao_tof`.
///
/// Cadeia testada, sem hardware: `LabirintoSimulado` → `TofSimulado` (amostras brutas da face de
/// cada sensor, 8190 mm sem alvo) → `CombinadorTof` → `ClassificadorParede` → `AtualizadorMapa`
/// → `Mapa`. Diferente do `test_integracao_mapa`, a leitura passa pela regra de validade do driver.

#include <unity.h>

#include "AtualizadorMapa.h"
#include "ClassificadorParede.h"
#include "Direcao.h"
#include "LabirintoSimulado.h"
#include "Mapa.h"
#include "TofSimulado.h"
#include "../test_labirinto_simulado/LabirintosExemplo.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

/// 12×4 em serpentina: cada linha é um corredor inteiro, então, olhando ao longo dela, a parede
/// fica a 2070 mm do centro e o sensor devolve 8190 mm (sem alvo). Os exemplos de
/// `LabirintosExemplo.h` não passam de 1140 mm brutos e nunca chegam a esse caso.
constexpr const char* LABIRINTO_12X4_CORREDORES = R"(+---+---+---+---+---+---+---+---+---+---+---+---+
|                                               |
+   +---+---+---+---+---+---+---+---+---+---+---+
|                                               |
+---+---+---+---+---+---+---+---+---+---+---+   +
|                                               |
+   +---+---+---+---+---+---+---+---+---+---+---+
|                                               |
+---+---+---+---+---+---+---+---+---+---+---+---+)";

/// Robô simulado que explora o labirinto em profundidade usando só o que os ToF mostram: a cada
/// célula nova ele lê os sensores na pose atual, atualiza o mapa e segue pelas direções que o
/// próprio mapa diz estarem livres.
class ExploradorTof {
 public:
  ExploradorTof(const LabirintoSimulado& labirinto, Mapa& mapa) : mapa_(mapa), tof_(labirinto) {}

  /// Explora a partir da partida (0, 0), virado para o Norte.
  void explorar() { visitar({0, 0}, Direcao::Norte); }

  uint8_t celulasVisitadas() const { return visitadas_; }
  uint8_t registrosComConflito() const { return conflitos_; }
  uint8_t leiturasQueFalharam() const { return falhas_; }
  /// Quantas amostras brutas vieram sem alvo (8190 mm) durante a exploração.
  uint16_t amostrasSemAlvo() const { return semAlvo_; }

 private:
  void visitar(PosicaoCelula celula, Direcao direcao) {
    visitada_[celula.linha][celula.coluna] = true;
    visitadas_++;

    const Pose pose = {celula, direcao};
    tof_.posicionar(pose);
    DistanciasLaterais leitura = {};
    if (!tof_.lerDistanciasLaterais(leitura)) {
      falhas_++;
      return;
    }
    const Lado lados[] = {Lado::Frente, Lado::Esquerda, Lado::Direita};
    for (Lado lado : lados) {
      semAlvo_ += tof_.amostra(lado).estado == EstadoAmostraTof::ForaDeAlcance ? 1 : 0;
    }
    const ResultadoAtualizacao r = AtualizadorMapa::atualizar(mapa_, pose, classificador_, &leitura, 1);
    conflitos_ += r.temConflito ? 1 : 0;

    const Direcao direcoes[] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};
    for (Direcao d : direcoes) {
      PosicaoCelula proxima;
      if (mapa_.obterParede(celula, d) == EstadoParede::Livre && vizinha(celula, d, proxima) &&
          mapa_.posValida(proxima) && !visitada_[proxima.linha][proxima.coluna]) {
        visitar(proxima, d);
      }
    }
  }

  Mapa& mapa_;
  TofSimulado tof_;
  ClassificadorParede classificador_;
  bool visitada_[MAX_LINHAS_LABIRINTO][MAX_COLUNAS_LABIRINTO] = {};
  uint8_t visitadas_ = 0;
  uint8_t conflitos_ = 0;
  uint8_t falhas_ = 0;
  uint16_t semAlvo_ = 0;
};

/// Número de paredes em que o mapa discorda do labirinto (parede ou livre).
uint16_t paredesDiferentes(const Mapa& mapa, const LabirintoSimulado& labirinto) {
  const Direcao direcoes[] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};
  uint16_t diferentes = 0;
  for (uint8_t linha = 0; linha < labirinto.obterLinhas(); linha++) {
    for (uint8_t coluna = 0; coluna < labirinto.obterColunas(); coluna++) {
      for (Direcao d : direcoes) {
        const EstadoParede esperado =
            labirinto.temParede({linha, coluna}, d) ? EstadoParede::Parede : EstadoParede::Livre;
        if (mapa.obterParede({linha, coluna}, d) != esperado) {
          diferentes++;
        }
      }
    }
  }
  return diferentes;
}

/// Carrega o texto, explora pelos ToF e confere que o mapa final é idêntico ao labirinto.
/// @return Quantas amostras vieram sem alvo durante a exploração.
uint16_t explorarEConferir(const char* texto, DimensaoMapa dimensao) {
  LabirintoSimulado labirinto;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoCarga::Carregado),
                          static_cast<uint8_t>(labirinto.carregar(texto)));
  Mapa mapa(dimensao);
  ExploradorTof explorador(labirinto, mapa);

  explorador.explorar();

  TEST_ASSERT_EQUAL_UINT8(labirinto.obterLinhas() * labirinto.obterColunas(), explorador.celulasVisitadas());
  TEST_ASSERT_EQUAL_UINT8(0, explorador.leiturasQueFalharam());
  TEST_ASSERT_EQUAL_UINT8(0, explorador.registrosComConflito());
  TEST_ASSERT_EQUAL_UINT16(0, paredesDiferentes(mapa, labirinto));
  return explorador.amostrasSemAlvo();
}

void verificarEstado(EstadoParede esperado, EstadoParede obtido) {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(esperado), static_cast<uint8_t>(obtido));
}

}  // namespace

void test_mapa_final_identico_ao_labirinto_4x4() {
  explorarEConferir(exemplos::LABIRINTO_4X4, DimensaoMapa::Labirinto4x4);
}

void test_mapa_final_identico_ao_labirinto_8x4() {
  explorarEConferir(exemplos::LABIRINTO_8X4, DimensaoMapa::Labirinto8x4);
}

void test_mapa_final_identico_ao_labirinto_12x4() {
  explorarEConferir(exemplos::LABIRINTO_12X4, DimensaoMapa::Labirinto12x4);
}

void test_corredores_sem_alvo_viram_passagem_livre_e_o_mapa_fecha() {
  // Sem a limitação à faixa, cada 8190 mm derrubaria a leitura e o robô pararia na partida.
  const uint16_t semAlvo = explorarEConferir(LABIRINTO_12X4_CORREDORES, DimensaoMapa::Labirinto12x4);
  TEST_ASSERT_GREATER_THAN_UINT16(0, semAlvo);
}

void test_amostra_bruta_e_a_distancia_da_face_do_sensor() {
  LabirintoSimulado labirinto;
  labirinto.carregar(exemplos::LABIRINTO_4X4);
  TofSimulado tof(labirinto);
  // Em (0, 0) Norte, do centro: {630, 90, 90}; da face de cada sensor, 30 mm a menos.
  tof.posicionar({{0, 0}, Direcao::Norte});
  DistanciasLaterais leitura = {};
  TEST_ASSERT_TRUE(tof.lerDistanciasLaterais(leitura));
  TEST_ASSERT_EQUAL_UINT16(600, leitura.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(60, leitura.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(60, leitura.direitaMm);
  TEST_ASSERT_EQUAL_UINT32(0, tof.idadeUltimaLeituraMs());
}

void test_sensor_sem_alvo_entrega_o_alcance_maximo() {
  LabirintoSimulado labirinto;
  labirinto.carregar(LABIRINTO_12X4_CORREDORES);
  TofSimulado tof(labirinto);
  // Em (0, 0) Leste, a parede da frente está a 2070 mm do centro: o sensor não a alcança.
  tof.posicionar({{0, 0}, Direcao::Leste});
  DistanciasLaterais leitura = {};
  TEST_ASSERT_TRUE(tof.lerDistanciasLaterais(leitura));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::ForaDeAlcance, tof.amostra(Lado::Frente).estado);
  TEST_ASSERT_EQUAL_UINT16(SEM_ALVO_TOF_SIMULADO_MM, tof.amostra(Lado::Frente).distanciaMm);
  TEST_ASSERT_EQUAL_UINT16(DISTANCIA_MAXIMA_VALIDA_TOF_MM, leitura.frenteMm);
}

void test_sensor_encostado_registra_parede() {
  LabirintoSimulado labirinto;
  labirinto.carregar(exemplos::LABIRINTO_4X4);
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  TofSimulado tof(labirinto);
  ClassificadorParede classificador;

  // Em (1, 1) Norte há parede a Oeste (esquerda). O robô desviou e o sensor esquerdo lê 12 mm,
  // abaixo do mínimo de 30 mm: a leitura vale, limitada a 30 mm, e a parede é registrada.
  const Pose pose = {{1, 1}, Direcao::Norte};
  tof.posicionar(pose);
  tof.forcarAmostra(Lado::Esquerda, 12);
  DistanciasLaterais leitura = {};
  TEST_ASSERT_TRUE(tof.lerDistanciasLaterais(leitura));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::AbaixoDoMinimo, tof.amostra(Lado::Esquerda).estado);
  TEST_ASSERT_EQUAL_UINT16(DISTANCIA_MINIMA_VALIDA_TOF_MM, leitura.esquerdaMm);

  const ResultadoAtualizacao r = AtualizadorMapa::atualizar(mapa, pose, classificador, &leitura, 1);
  TEST_ASSERT_FALSE(r.temConflito);
  verificarEstado(EstadoParede::Parede, mapa.obterParede({1, 1}, Direcao::Oeste));
}

void test_sensor_sem_resposta_nao_altera_o_mapa() {
  LabirintoSimulado labirinto;
  labirinto.carregar(exemplos::LABIRINTO_4X4);
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  const Mapa antes = mapa;
  TofSimulado tof(labirinto);

  tof.posicionar({{1, 1}, Direcao::Norte});
  tof.simularFalha(Lado::Direita, true);
  DistanciasLaterais leitura = {};
  TEST_ASSERT_FALSE(tof.lerDistanciasLaterais(leitura));
  TEST_ASSERT_EQUAL(EstadoAmostraTof::Falha, tof.amostra(Lado::Direita).estado);
  // Quem lê não atualiza o mapa com leitura recusada: o mapa continua igual.
  TEST_ASSERT_TRUE(mapa == antes);

  tof.simularFalha(Lado::Direita, false);
  TEST_ASSERT_TRUE(tof.lerDistanciasLaterais(leitura));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_mapa_final_identico_ao_labirinto_4x4);
  RUN_TEST(test_mapa_final_identico_ao_labirinto_8x4);
  RUN_TEST(test_mapa_final_identico_ao_labirinto_12x4);
  RUN_TEST(test_corredores_sem_alvo_viram_passagem_livre_e_o_mapa_fecha);
  RUN_TEST(test_amostra_bruta_e_a_distancia_da_face_do_sensor);
  RUN_TEST(test_sensor_sem_alvo_entrega_o_alcance_maximo);
  RUN_TEST(test_sensor_encostado_registra_parede);
  RUN_TEST(test_sensor_sem_resposta_nao_altera_o_mapa);
  return UNITY_END();
}
