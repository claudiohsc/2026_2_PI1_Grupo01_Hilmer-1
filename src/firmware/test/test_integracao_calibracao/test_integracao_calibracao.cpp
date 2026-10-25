/// @file test_integracao_calibracao.cpp
/// @brief Integração da calibração do boot (#240): desvio dos ToF e viés do giroscópio, medidos na
/// largada, chegam ao mapa e ao ângulo de giro.
/// Roda no computador: `pio test -e native -f test_integracao_calibracao`.
///
/// Cadeias testadas, sem hardware:
/// - `LabirintoSimulado` → `TofSimulado` (com desvio por sensor) → `RotinaCalibracao` →
///   `corrigirTof` → `ClassificadorParede` → `AtualizadorMapa` → `Mapa`;
/// - `ImuSimulado` (com viés) → `RotinaCalibracao` → `AcompanhadorGiro::definirVies` → ângulo.

#include <unity.h>

#include "AcompanhadorGiro.h"
#include "AtualizadorMapa.h"
#include "ClassificadorParede.h"
#include "Direcao.h"
#include "ImuSimulado.h"
#include "IntegradorGiro.h"
#include "LabirintoSimulado.h"
#include "Mapa.h"
#include "RotinaCalibracao.h"
#include "TofSimulado.h"
#include "../test_labirinto_simulado/LabirintosExemplo.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr int16_t DESVIO_FRENTE_MM = 12;     ///< Sem referência na largada: não é corrigido.
constexpr int16_t DESVIO_ESQUERDA_MM = 9;
constexpr int16_t DESVIO_DIREITA_MM = -7;
constexpr int16_t VIES_GIRO_DECIMOS_DPS = 14;  ///< 1,4 °/s, típico de um MPU6050 parado.

/// Distância da face de cada sensor na célula de partida do 4×4 ((0, 0), Norte): paredes dos dois
/// lados, a 90 mm do centro menos o recuo do sensor; a frente é o corredor.
constexpr uint16_t FACE_LATERAL_NA_LARGADA_MM = 90 - RECUO_SENSOR_TOF_SIMULADO_MM;

LabirintoSimulado carregar4x4() {
  LabirintoSimulado labirinto;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoCarga::Carregado),
                          static_cast<uint8_t>(labirinto.carregar(exemplos::LABIRINTO_4X4)));
  return labirinto;
}

/// Referências da largada para o labirinto simulado (a face lateral mede 60 mm, e não os 54 mm
/// provisórios da geometria real).
ConfiguracaoRotinaCalibracao configNaLargada() {
  ConfiguracaoRotinaCalibracao c;
  c.tof.referencias = {SEM_REFERENCIA_TOF_MM, FACE_LATERAL_NA_LARGADA_MM, FACE_LATERAL_NA_LARGADA_MM};
  return c;
}

void desviarSensores(TofSimulado& tof) {
  tof.definirDesvio(Lado::Frente, DESVIO_FRENTE_MM);
  tof.definirDesvio(Lado::Esquerda, DESVIO_ESQUERDA_MM);
  tof.definirDesvio(Lado::Direita, DESVIO_DIREITA_MM);
}

/// Robô simulado que explora o labirinto em profundidade lendo os ToF (com desvio) e, se recebeu uma
/// rotina, corrigindo cada leitura com ela antes de classificar. Guarda o pior erro lateral que sobrou
/// em relação a um sensor sem desvio.
class ExploradorCalibrado {
 public:
  ExploradorCalibrado(const LabirintoSimulado& labirinto, Mapa& mapa, const RotinaCalibracao* rotina)
      : mapa_(mapa), rotina_(rotina), tof_(labirinto), tofIdeal_(labirinto) {
    desviarSensores(tof_);
  }

  void explorar() { visitar({0, 0}, Direcao::Norte); }

  uint8_t celulasVisitadas() const { return visitadas_; }
  uint8_t leiturasQueFalharam() const { return falhas_; }
  uint8_t registrosComConflito() const { return conflitos_; }
  /// Maior diferença, em mm, entre a leitura lateral corrigida e a de um sensor sem desvio.
  uint16_t piorErroLateralMm() const { return piorErroLateral_; }

 private:
  static uint16_t diferenca(uint16_t a, uint16_t b) { return a > b ? a - b : b - a; }

  void visitar(PosicaoCelula celula, Direcao direcao) {
    visitada_[celula.linha][celula.coluna] = true;
    visitadas_++;

    const Pose pose = {celula, direcao};
    tof_.posicionar(pose);
    tofIdeal_.posicionar(pose);
    DistanciasLaterais leitura = {};
    DistanciasLaterais ideal = {};
    if (!tof_.lerDistanciasLaterais(leitura) || !tofIdeal_.lerDistanciasLaterais(ideal)) {
      falhas_++;
      return;
    }
    if (rotina_ != nullptr) {
      leitura = rotina_->corrigirTof(leitura);
    }
    const uint16_t erro = diferenca(leitura.esquerdaMm, ideal.esquerdaMm) > diferenca(leitura.direitaMm, ideal.direitaMm)
                              ? diferenca(leitura.esquerdaMm, ideal.esquerdaMm)
                              : diferenca(leitura.direitaMm, ideal.direitaMm);
    if (erro > piorErroLateral_) {
      piorErroLateral_ = erro;
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
  const RotinaCalibracao* rotina_;
  TofSimulado tof_;
  TofSimulado tofIdeal_;
  ClassificadorParede classificador_;
  bool visitada_[MAX_LINHAS_LABIRINTO][MAX_COLUNAS_LABIRINTO] = {};
  uint8_t visitadas_ = 0;
  uint8_t falhas_ = 0;
  uint8_t conflitos_ = 0;
  uint16_t piorErroLateral_ = 0;
};

/// Calibra na largada: o robô está em (0, 0), Norte, com os sensores desviados e a IMU com viés.
EstadoRotinaCalibracao calibrarNaLargada(RotinaCalibracao& rotina, const LabirintoSimulado& labirinto,
                                         int16_t viesGiro) {
  TofSimulado tof(labirinto);
  tof.posicionar({{0, 0}, Direcao::Norte});
  desviarSensores(tof);
  ImuSimulado imu;
  imu.definirVies(viesGiro);
  return rotina.executar(tof, imu);
}

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

}  // namespace

// ---------------------------------------------------------------- ToF: da calibração ao mapa

void test_calibracao_na_largada_mede_o_desvio_dos_dois_lados_e_deixa_a_frente_sem_correcao() {
  const LabirintoSimulado labirinto = carregar4x4();
  RotinaCalibracao rotina(configNaLargada());

  const EstadoRotinaCalibracao estado = calibrarNaLargada(rotina, labirinto, VIES_GIRO_DECIMOS_DPS);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::Sucesso), static_cast<int>(estado));
  TEST_ASSERT_EQUAL_INT16(DESVIO_ESQUERDA_MM, rotina.desviosTof().esquerdaMm);
  TEST_ASSERT_EQUAL_INT16(DESVIO_DIREITA_MM, rotina.desviosTof().direitaMm);
  TEST_ASSERT_EQUAL_INT16(0, rotina.desviosTof().frenteMm);
}

void test_calibracao_cabe_no_orcamento_de_1_s_com_a_rotina_inteira() {
  const LabirintoSimulado labirinto = carregar4x4();
  RotinaCalibracao rotina(configNaLargada());

  calibrarNaLargada(rotina, labirinto, VIES_GIRO_DECIMOS_DPS);

  TEST_ASSERT_TRUE(rotina.dentroDoOrcamento());
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(ORCAMENTO_CALIBRACAO_MS, rotina.duracaoMedidaMs());
  TEST_ASSERT_GREATER_THAN_UINT32(0, rotina.duracaoMedidaMs());
}

void test_com_a_calibracao_a_leitura_lateral_corrigida_volta_ao_valor_do_sensor_sem_desvio() {
  const LabirintoSimulado labirinto = carregar4x4();
  RotinaCalibracao rotina(configNaLargada());
  calibrarNaLargada(rotina, labirinto, VIES_GIRO_DECIMOS_DPS);

  Mapa mapa(DimensaoMapa::Labirinto4x4);
  ExploradorCalibrado explorador(labirinto, mapa, &rotina);
  explorador.explorar();

  // Em todas as células visitadas, o erro lateral que sobra é o do arredondamento (zero aqui).
  TEST_ASSERT_EQUAL_UINT16(0, explorador.piorErroLateralMm());
}

void test_sem_a_calibracao_o_desvio_aparece_inteiro_na_leitura_lateral() {
  const LabirintoSimulado labirinto = carregar4x4();
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  ExploradorCalibrado explorador(labirinto, mapa, nullptr);

  explorador.explorar();

  TEST_ASSERT_EQUAL_UINT16(static_cast<uint16_t>(DESVIO_ESQUERDA_MM), explorador.piorErroLateralMm());
}

void test_mapa_final_do_4x4_e_identico_ao_labirinto_com_a_leitura_corrigida() {
  const LabirintoSimulado labirinto = carregar4x4();
  RotinaCalibracao rotina(configNaLargada());
  calibrarNaLargada(rotina, labirinto, VIES_GIRO_DECIMOS_DPS);

  Mapa mapa(DimensaoMapa::Labirinto4x4);
  ExploradorCalibrado explorador(labirinto, mapa, &rotina);
  explorador.explorar();

  TEST_ASSERT_EQUAL_UINT8(labirinto.obterLinhas() * labirinto.obterColunas(), explorador.celulasVisitadas());
  TEST_ASSERT_EQUAL_UINT8(0, explorador.leiturasQueFalharam());
  TEST_ASSERT_EQUAL_UINT8(0, explorador.registrosComConflito());
  TEST_ASSERT_EQUAL_UINT16(0, paredesDiferentes(mapa, labirinto));
}

void test_calibracao_recusada_deixa_as_leituras_do_tof_como_vieram() {
  const LabirintoSimulado labirinto = carregar4x4();
  RotinaCalibracao rotina(configNaLargada());
  TofSimulado tof(labirinto);
  tof.posicionar({{0, 0}, Direcao::Norte});
  desviarSensores(tof);
  ImuSimulado imu;
  imu.definirRuidoAlternado(200);  // 20 °/s para os dois lados: o robô está se mexendo

  const EstadoRotinaCalibracao estado = rotina.executar(tof, imu);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::RecusadaPorMovimento), static_cast<int>(estado));
  const DistanciasLaterais leitura = {600, 80, 40};
  const DistanciasLaterais corrigida = rotina.corrigirTof(leitura);
  TEST_ASSERT_EQUAL_UINT16(600, corrigida.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(80, corrigida.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(40, corrigida.direitaMm);
}

void test_referencia_errada_na_largada_nao_estraga_as_leituras() {
  const LabirintoSimulado labirinto = carregar4x4();
  // Robô posto diante de um corredor largo: a esquerda lê 60 + 40 mm contra a referência de 60.
  ConfiguracaoRotinaCalibracao config = configNaLargada();
  RotinaCalibracao rotina(config);
  TofSimulado tof(labirinto);
  tof.posicionar({{0, 0}, Direcao::Norte});
  tof.definirDesvio(Lado::Esquerda, 40);
  ImuSimulado imu;

  const EstadoRotinaCalibracao estado = rotina.executar(tof, imu);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::DesvioForaDaFaixa), static_cast<int>(estado));
  TEST_ASSERT_EQUAL_UINT16(100, rotina.corrigirTof({600, 100, 60}).esquerdaMm);  // nada subtraído
}

// ---------------------------------------------------------------- giroscópio: do viés ao ângulo

void test_com_o_vies_da_rotina_o_robo_parado_nao_acumula_angulo() {
  const LabirintoSimulado labirinto = carregar4x4();
  RotinaCalibracao rotina(configNaLargada());
  calibrarNaLargada(rotina, labirinto, VIES_GIRO_DECIMOS_DPS);
  TEST_ASSERT_EQUAL_INT16(VIES_GIRO_DECIMOS_DPS, rotina.viesGiroDecimosDps());

  ImuSimulado imu;
  imu.definirVies(VIES_GIRO_DECIMOS_DPS);
  AcompanhadorGiro giro(imu);
  giro.definirVies(rotina.viesGiroDecimosDps());

  for (int i = 0; i < 500; ++i) {  // 5 s parado
    giro.ciclo(PERIODO_LEITURA_IMU_MS);
  }

  TEST_ASSERT_TRUE(giro.ultima().valida);
  TEST_ASSERT_EQUAL_INT32(0, giro.ultima().anguloMiligraus);
}

void test_sem_a_calibracao_o_mesmo_vies_vira_deriva_de_graus() {
  ImuSimulado imu;
  imu.definirVies(VIES_GIRO_DECIMOS_DPS);
  ConfiguracaoIntegradorGiro semZonaMorta;
  semZonaMorta.zonaMortaDecimosDps = 0;
  IntegradorGiro semViesIntegrador(semZonaMorta);

  int32_t angulo = 0;
  for (int i = 0; i < 500; ++i) {  // 5 s parado
    int16_t taxa = 0;
    imu.lerTaxaGiroZ(taxa);
    angulo = semViesIntegrador.atualizar(taxa, PERIODO_LEITURA_IMU_MS);
  }

  // 1,4 °/s por 5 s = 7° de deriva: o robô "achava" que tinha girado com as rodas paradas.
  TEST_ASSERT_INT32_WITHIN(100, 7000, angulo);
}

void test_giro_de_90_graus_com_o_vies_da_rotina_termina_perto_de_90() {
  const LabirintoSimulado labirinto = carregar4x4();
  RotinaCalibracao rotina(configNaLargada());
  calibrarNaLargada(rotina, labirinto, VIES_GIRO_DECIMOS_DPS);

  ImuSimulado imu;
  imu.definirVies(VIES_GIRO_DECIMOS_DPS);
  AcompanhadorGiro giro(imu);
  giro.definirVies(rotina.viesGiroDecimosDps());

  imu.definirTaxa(900);  // 90 °/s para a esquerda
  for (int i = 0; i < 100; ++i) {  // 1 s
    giro.ciclo(PERIODO_LEITURA_IMU_MS);
  }

  TEST_ASSERT_INT32_WITHIN(500, 90000, giro.ultima().anguloMiligraus);
}

void test_falhou_a_rotina_a_tarefa_da_imu_calibra_sozinha() {
  // Mesma decisão da `tarefaImu` do main.cpp: sem o viés da rotina, calibra por conta própria.
  ImuSimulado imu;
  imu.definirVies(VIES_GIRO_DECIMOS_DPS);
  AcompanhadorGiro giro(imu);

  if (giro.estadoCalibracao() != EstadoCalibracao::Calibrado) {
    giro.iniciarCalibracao();
  }
  for (uint16_t i = 0; i < AMOSTRAS_CALIBRACAO_GIRO; ++i) {
    giro.ciclo(PERIODO_LEITURA_IMU_MS);
  }

  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, giro.estadoCalibracao());
  TEST_ASSERT_TRUE(giro.ultima().valida);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_calibracao_na_largada_mede_o_desvio_dos_dois_lados_e_deixa_a_frente_sem_correcao);
  RUN_TEST(test_calibracao_cabe_no_orcamento_de_1_s_com_a_rotina_inteira);
  RUN_TEST(test_com_a_calibracao_a_leitura_lateral_corrigida_volta_ao_valor_do_sensor_sem_desvio);
  RUN_TEST(test_sem_a_calibracao_o_desvio_aparece_inteiro_na_leitura_lateral);
  RUN_TEST(test_mapa_final_do_4x4_e_identico_ao_labirinto_com_a_leitura_corrigida);
  RUN_TEST(test_calibracao_recusada_deixa_as_leituras_do_tof_como_vieram);
  RUN_TEST(test_referencia_errada_na_largada_nao_estraga_as_leituras);
  RUN_TEST(test_com_o_vies_da_rotina_o_robo_parado_nao_acumula_angulo);
  RUN_TEST(test_sem_a_calibracao_o_mesmo_vies_vira_deriva_de_graus);
  RUN_TEST(test_giro_de_90_graus_com_o_vies_da_rotina_termina_perto_de_90);
  RUN_TEST(test_falhou_a_rotina_a_tarefa_da_imu_calibra_sozinha);
  return UNITY_END();
}
