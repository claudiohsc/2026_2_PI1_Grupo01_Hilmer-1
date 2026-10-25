/// @file test_integracao_falhas_i2c.cpp
/// @brief Testes de integração da falha do I²C (#241) com o ciclo de navegação (#184): o robô
/// simulado corre o labirinto, um dispositivo do I²C para de responder no meio da corrida, e o
/// firmware para, reinicia o dispositivo e retoma a corrida.
///
/// A cadeia é a do `main.cpp`: `RoboSimulado` → `ImuSimulado` + `AcompanhadorGiro` e 3 raios de ToF
/// → `CombinadorTof` (a cada 33 ms, ou uma amostra em `Falha` a cada ciclo para o sensor sem
/// resposta, como o `TofVl53l0x`) → `VigiaTof` + `SupervisorI2c` → `TravaFalhaI2c` na
/// `MotoresComTrava` e `EntradaCiclo::liberado` → `CicloNavegacao` → motores, pelo
/// `MotoresComEstimativa`.
///
/// O sensor sem resposta segue o driver: um reinício que falha deixa o sensor em reset (o
/// `TofVl53l0x` não o lê mais), e só um reinício depois de o contato voltar o traz de volta.
/// Roda no computador: `pio test -e native -f test_integracao_falhas_i2c`.

#include <unity.h>

#include <cmath>
#include <cstdio>

#include "../test_labirinto_simulado/LabirintosExemplo.h"
#include "AcompanhadorGiro.h"
#include "ArmazenamentoMapa.h"
#include "CicloNavegacao.h"
#include "CombinadorTof.h"
#include "CorrecaoPelasParedes.h"
#include "DeslocamentoEstimado.h"
#include "ImuSimulado.h"
#include "LabirintoSimulado.h"
#include "MotoresComTrava.h"
#include "Odometria.h"
#include "RoboSimulado.h"
#include "SupervisorI2c.h"
#include "TravaFalhaI2c.h"
#include "VigiaTof.h"

using namespace micromouse;
using exemplos::LABIRINTO_4X4;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint32_t CICLO_MS = 10;
constexpr uint32_t PERIODO_TOF_MS = 33;
constexpr uint32_t DEZ_MINUTOS_MS = 10u * 60u * 1000u;
constexpr Lado LADOS[3] = {Lado::Frente, Lado::Esquerda, Lado::Direita};

uint8_t indice(Lado lado) { return static_cast<uint8_t>(lado); }

/// O robô, os sensores, o supervisor do I²C e o ciclo de navegação, ligados como no `main.cpp`.
class Corrida {
 public:
  explicit Corrida(const char* texto)
      : robo_(labirinto_), deslocamento_(robo_), trava_(deslocamento_.motores()), ciclo_(armazenamento_.mapa()) {
    TEST_ASSERT_EQUAL(ResultadoCarga::Carregado, labirinto_.carregar(texto));
    armazenamento_.iniciarCorrida(labirinto_.obterDimensao());
    const Pose inicial{{0, 0}, Direcao::Norte};
    robo_.posicionar(inicial);
    deslocamento_.reiniciar(odometria_, inicial);
    imu_.definirVies(12);
    imu_.definirRuidoAlternado(4);
    acompanhador_.iniciarCalibracao();
    for (int i = 0; i < 400 && acompanhador_.ultima().calibracao != EstadoCalibracao::Calibrado; i++) {
      acompanhador_.ciclo(CICLO_MS);
    }
    TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, acompanhador_.ultima().calibracao);
    trava_.liberar();  // boot aprovado
    ciclo_.reiniciar(inicial, trava_);
  }

  /// O ToF do `lado` para de responder a partir de agora, por `duracaoMs`.
  void tofSemResposta(Lado lado, uint32_t duracaoMs) { tofSemRespostaAteMs_[indice(lado)] = agoraMs_ + duracaoMs; }

  /// O MPU6050 para de responder a partir de agora, por `duracaoMs` (um mau contato que volta sozinho).
  void imuSemResposta(uint32_t duracaoMs) { imuSemRespostaAteMs_ = agoraMs_ + duracaoMs; }

  /// Um ciclo de 10 ms: o mundo, a tarefa da IMU e a tarefa de navegação.
  EstadoCiclo passo() {
    agoraMs_ += CICLO_MS;
    robo_.avancar(CICLO_MS);

    // Tarefa da IMU: o giroscópio e o acelerômetro, e o supervisor.
    imu_.simularFalha(agoraMs_ < imuSemRespostaAteMs_);
    imu_.definirTaxa(robo_.taxaGiroDecimosDps());
    const uint32_t falhasAntes = acompanhador_.ultima().falhasLeitura;
    giro_ = acompanhador_.ciclo(CICLO_MS);
    int16_t ax = 0;
    int16_t ay = 0;
    const bool aceleracaoOk = imu_.lerAceleracaoPlano(ax, ay);
    const LeituraI2c leituraImu = leituraDaImu(giro_.falhasLeitura != falhasAntes || !aceleracaoOk, false);
    if (supervisor_.registrarLeitura(Dispositivo::Mpu6050, leituraImu, agoraMs_).acao == AcaoI2c::Reiniciar) {
      supervisor_.registrarReinicio(Dispositivo::Mpu6050, agoraMs_ >= imuSemRespostaAteMs_);
    }

    // Tarefa de navegação: a leitura dos ToF, como o `TofVl53l0x` a registra no combinador.
    const bool medicao = agoraMs_ - ultimaTofMs_ >= PERIODO_TOF_MS;
    const DistanciasLaterais bruta = robo_.medirTof();
    const uint16_t medidas[3] = {bruta.frenteMm, bruta.esquerdaMm, bruta.direitaMm};
    for (Lado lado : LADOS) {
      const uint8_t i = indice(lado);
      if (emReset_[i] || agoraMs_ < tofSemRespostaAteMs_[i]) {
        tof_.registrar(lado, 0, true, agoraMs_);  // sem resposta, ou em reset depois de um reinício que falhou
      } else if (medicao) {
        tof_.registrar(lado, medidas[i], false, agoraMs_);
      }
    }
    bool nova = false;
    if (medicao) {
      ultimaTofMs_ = agoraMs_;
      nova = tof_.distancias(agoraMs_, leitura_);
    }

    // `supervisionarI2c()`: registra os 3 ToF, aplica a trava e só então reinicia.
    bool reiniciar[3] = {false, false, false};
    for (Lado lado : LADOS) {
      const LeituraI2c leitura = vigia_.avaliar(lado, tof_.amostra(lado), agoraMs_);
      reiniciar[indice(lado)] =
          supervisor_.registrarLeitura(dispositivoDoTof(lado), leitura, agoraMs_).acao == AcaoI2c::Reiniciar;
    }
    const bool falhaI2c = supervisor_.motoresDevemParar();
    travaFalha_.aplicar(falhaI2c, trava_);
    for (Lado lado : LADOS) {
      const uint8_t i = indice(lado);
      if (!reiniciar[i]) {
        continue;
      }
      andouNoReinicio_ = andouNoReinicio_ || robo_.potencia().esquerda != 0 || robo_.potencia().direita != 0;
      const bool voltou = agoraMs_ >= tofSemRespostaAteMs_[i];
      emReset_[i] = !voltou;
      ++reiniciosTof_[i];
      vigia_.sensorReiniciado(lado, agoraMs_);
      supervisor_.registrarReinicio(dispositivoDoTof(lado), voltou);
      ultimaTofMs_ = agoraMs_;  // o sensor reiniciado leva uma medição inteira para a primeira medida
    }

    // A odometria e a correção pelas paredes, como no `main.cpp`.
    const MovimentoRobo movimento = ciclo_.movimentoRobo();
    if (ciclo_.direcaoDefinida()) {
      odometria_.definirDirecao(ciclo_.direcao());
    }
    if (movimento == MovimentoRobo::Manobrando) {
      deslocamento_.esquecerLaterais();
    }
    const bool referencia = nova && movimento != MovimentoRobo::Manobrando;
    deslocamento_.ciclo(CICLO_MS, odometria_, referencia ? &leitura_ : nullptr);
    if (nova) {
      correcao_.aplicar(odometria_, leitura_, movimento, false);
    }

    EntradaCiclo e;
    e.liberado = !falhaI2c;
    e.pose = odometria_.pose();
    e.poseValida = odometria_.posicaoValida();
    e.posicaoAvancoUm = posicaoAoLongoDoAvancoUm(odometria_);
    e.leituraNova = nova ? &leitura_ : nullptr;
    e.giro = giro_;
    e.dtMs = CICLO_MS;
    return ciclo_.atualizar(e, trava_);
  }

  /// Corre até o ciclo parar (objetivo, sem caminho ou falha) ou o tempo acabar.
  EstadoCiclo correr(uint32_t limiteMs = DEZ_MINUTOS_MS) {
    EstadoCiclo estado = ciclo_.estado();
    while (agoraMs_ < limiteMs) {
      estado = passo();
      if (estado == EstadoCiclo::ObjetivoAlcancado || estado == EstadoCiclo::SemCaminho ||
          estado == EstadoCiclo::Falha) {
        break;
      }
    }
    return estado;
  }

  /// Corre até o robô estar avançando depois de `celulas` células.
  void correrAteAvancar(uint32_t celulas) {
    while (agoraMs_ < DEZ_MINUTOS_MS &&
           !(ciclo_.celulasAvancadas() >= celulas && ciclo_.estado() == EstadoCiclo::Avancando)) {
      passo();
    }
    TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, ciclo_.estado());
  }

  const RoboSimulado& robo() const { return robo_; }
  const CicloNavegacao& ciclo() const { return ciclo_; }
  const SupervisorI2c& supervisor() const { return supervisor_; }
  const MotoresComTrava& trava() const { return trava_; }
  const Mapa& mapa() const { return armazenamento_.mapa(); }
  const LabirintoSimulado& labirinto() const { return labirinto_; }
  uint32_t agoraMs() const { return agoraMs_; }
  uint32_t reiniciosTof(Lado lado) const { return reiniciosTof_[indice(lado)]; }
  bool andouNoReinicio() const { return andouNoReinicio_; }

 private:
  LabirintoSimulado labirinto_;
  RoboSimulado robo_;
  DeslocamentoEstimado deslocamento_;
  MotoresComTrava trava_;
  Odometria odometria_;
  ImuSimulado imu_;
  AcompanhadorGiro acompanhador_{imu_};
  LeituraGiro giro_;
  CombinadorTof tof_;
  DistanciasLaterais leitura_ = {0, 0, 0};
  ArmazenamentoMapa armazenamento_;
  CorrecaoPelasParedes correcao_;
  CicloNavegacao ciclo_;
  VigiaTof vigia_;
  SupervisorI2c supervisor_;
  TravaFalhaI2c travaFalha_;
  uint32_t agoraMs_ = 0;
  uint32_t ultimaTofMs_ = 0;
  uint32_t tofSemRespostaAteMs_[3] = {0, 0, 0};
  uint32_t imuSemRespostaAteMs_ = 0;
  bool emReset_[3] = {false, false, false};
  uint32_t reiniciosTof_[3] = {0, 0, 0};
  bool andouNoReinicio_ = false;
};

/// Toda parede que o robô gravou no mapa confere com o labirinto de verdade.
void conferirMapa(const Corrida& corrida) {
  const LabirintoSimulado& labirinto = corrida.labirinto();
  for (uint8_t l = 0; l < labirinto.obterLinhas(); l++) {
    for (uint8_t c = 0; c < labirinto.obterColunas(); c++) {
      for (uint8_t d = 0; d < 4; d++) {
        const Direcao direcao = static_cast<Direcao>(d);
        const EstadoParede estado = corrida.mapa().obterParede({l, c}, direcao);
        if (estado == EstadoParede::Desconhecido) {
          continue;
        }
        char msg[64];
        std::snprintf(msg, sizeof(msg), "parede (%u,%u) lado %u", l, c, d);
        TEST_ASSERT_EQUAL_MESSAGE(labirinto.temParede({l, c}, direcao) ? EstadoParede::Parede : EstadoParede::Livre,
                                  estado, msg);
      }
    }
  }
}

/// Chegou ao objetivo, sem encostar nas paredes, e o mapa confere com o labirinto.
void conferirChegada(const Corrida& corrida, const char* nome) {
  char msg[160];
  std::snprintf(msg, sizeof(msg), "%s: estado %s em %.1f s, %lu celulas", nome,
                nomeDoEstadoCiclo(corrida.ciclo().estado()), corrida.agoraMs() / 1000.0,
                static_cast<unsigned long>(corrida.ciclo().celulasAvancadas()));
  TEST_MESSAGE(msg);
  TEST_ASSERT_EQUAL_MESSAGE(EstadoCiclo::ObjetivoAlcancado, corrida.ciclo().estado(), msg);
  TEST_ASSERT_FALSE_MESSAGE(corrida.robo().colidiu(), msg);
  conferirMapa(corrida);
}

}  // namespace

void test_sem_falha_a_corrida_chega_ao_objetivo() {
  Corrida corrida(LABIRINTO_4X4);
  corrida.correr();
  conferirChegada(corrida, "sem falha");
  TEST_ASSERT_EQUAL_UINT32(0, corrida.supervisor().reiniciosTotais());
  TEST_ASSERT_FALSE(corrida.trava().travado());
}

/// O ToF frontal para de responder no meio de um avanço, por 300 ms. Os motores param, o ciclo
/// espera, o primeiro reinício falha (o contato ainda está solto) e deixa o sensor em reset, o
/// segundo, 500 ms depois, o traz de volta, e a corrida segue até o objetivo.
void test_tof_sem_resposta_no_meio_do_avanco_para_reinicia_e_retoma_a_corrida() {
  Corrida corrida(LABIRINTO_4X4);
  corrida.correrAteAvancar(2);
  corrida.tofSemResposta(Lado::Frente, 300);
  const uint32_t inicio = corrida.agoraMs();

  while (corrida.supervisor().situacao(Dispositivo::TofFrontal) != SituacaoI2c::EmFalha) {
    TEST_ASSERT_FALSE(corrida.trava().travado());  // nas tentativas, segue
    corrida.passo();
  }
  TEST_ASSERT_TRUE(corrida.agoraMs() - inicio <= 6 * CICLO_MS);
  TEST_ASSERT_TRUE(corrida.trava().travado());
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, corrida.ciclo().estado());  // o ciclo espera
  TEST_ASSERT_FALSE(corrida.andouNoReinicio());

  // Parado durante toda a falha.
  const double x = corrida.robo().xMm();
  const double y = corrida.robo().yMm();
  while (corrida.supervisor().situacao(Dispositivo::TofFrontal) != SituacaoI2c::Ok) {
    corrida.passo();
    TEST_ASSERT_TRUE(corrida.agoraMs() - inicio < 2000);
  }
  TEST_ASSERT_TRUE(std::fabs(corrida.robo().xMm() - x) <= 1.0);
  TEST_ASSERT_TRUE(std::fabs(corrida.robo().yMm() - y) <= 1.0);
  TEST_ASSERT_EQUAL_UINT32(2, corrida.reiniciosTof(Lado::Frente));
  TEST_ASSERT_EQUAL_UINT32(1, corrida.supervisor().reiniciosSemSucesso(Dispositivo::TofFrontal));
  TEST_ASSERT_FALSE(corrida.trava().travado());

  corrida.correr();
  conferirChegada(corrida, "ToF frontal sem resposta por 300 ms");
}

/// O ToF esquerdo cai de vez: 3 reinícios, inoperante, e o robô fica parado até o fim do tempo.
void test_tof_que_nao_volta_deixa_o_robo_parado() {
  Corrida corrida(LABIRINTO_4X4);
  corrida.correrAteAvancar(1);
  corrida.tofSemResposta(Lado::Esquerda, DEZ_MINUTOS_MS);
  corrida.correr(corrida.agoraMs() + 5000);
  TEST_ASSERT_TRUE(corrida.supervisor().situacao(Dispositivo::TofEsquerdo) == SituacaoI2c::Inoperante);
  TEST_ASSERT_EQUAL_UINT32(REINICIOS_MAXIMOS_I2C, corrida.reiniciosTof(Lado::Esquerda));
  TEST_ASSERT_TRUE(corrida.trava().travado());
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, corrida.ciclo().estado());
  TEST_ASSERT_FALSE(corrida.robo().colidiu());
  TEST_ASSERT_FALSE(corrida.andouNoReinicio());
}

/// O MPU6050 para de responder no meio de um avanço, por 200 ms, e volta sozinho: os motores param,
/// a IMU é reiniciada e a corrida segue até o objetivo.
void test_imu_sem_resposta_no_meio_do_avanco_para_e_retoma_a_corrida() {
  Corrida corrida(LABIRINTO_4X4);
  corrida.correrAteAvancar(2);
  corrida.imuSemResposta(200);
  const uint32_t inicio = corrida.agoraMs();
  while (corrida.supervisor().situacao(Dispositivo::Mpu6050) != SituacaoI2c::EmFalha) {
    corrida.passo();
    TEST_ASSERT_TRUE(corrida.agoraMs() - inicio <= 6 * CICLO_MS);
  }
  TEST_ASSERT_TRUE(corrida.trava().travado());
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, corrida.ciclo().estado());
  while (corrida.supervisor().situacao(Dispositivo::Mpu6050) != SituacaoI2c::Ok) {
    corrida.passo();
    TEST_ASSERT_TRUE(corrida.agoraMs() - inicio < 2000);
  }
  TEST_ASSERT_TRUE(corrida.supervisor().reinicios(Dispositivo::Mpu6050) >= 1);
  corrida.correr();
  conferirChegada(corrida, "IMU sem resposta por 200 ms");
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_sem_falha_a_corrida_chega_ao_objetivo);
  RUN_TEST(test_tof_sem_resposta_no_meio_do_avanco_para_reinicia_e_retoma_a_corrida);
  RUN_TEST(test_tof_que_nao_volta_deixa_o_robo_parado);
  RUN_TEST(test_imu_sem_resposta_no_meio_do_avanco_para_e_retoma_a_corrida);
  return UNITY_END();
}
