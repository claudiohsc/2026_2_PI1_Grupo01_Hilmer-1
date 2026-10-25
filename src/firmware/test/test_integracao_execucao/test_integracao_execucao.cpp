/// @file test_integracao_execucao.cpp
/// @brief Testes de integração da execução dos movimentos (#184): o robô simulado anda de verdade
/// no labirinto e o firmware só vê o que veria na placa.
///
/// A cadeia é a mesma da tarefa de navegação: `RoboSimulado` (rodas com velocidade diferente da
/// tabela, giro pela diferença das rodas e colisão) → `ImuSimulado` + `AcompanhadorGiro` (viés,
/// ruído e calibração) e 3 raios de ToF → `CombinadorTof` (a cada 33 ms) → `DeslocamentoEstimado`
/// + `Odometria` (modo sem encoder, com a correção pelos ToF) → `CicloNavegacao` / `ExecutorMovimentos`
/// → motores, pelo `MotoresComEstimativa`.
///
/// Os erros de parada e de giro aparecem no log do teste (`TEST_MESSAGE`): são o registro do erro
/// na simulação. O erro real é medido na bancada (`scripts/bancada_deslocamento.py`).
/// Roda no computador: `pio test -e native -f test_integracao_execucao`.

#include <unity.h>

#include <cmath>
#include <cstdio>

#include "../test_labirinto_simulado/LabirintosExemplo.h"
#include "AcompanhadorGiro.h"
#include "ArmazenamentoMapa.h"
#include "CicloNavegacao.h"
#include "CombinadorTof.h"
#include "ConfiguracaoControle.h"
#include "CorretorDeslocamentoTof.h"
#include "DeslocamentoEstimado.h"
#include "Direcao.h"
#include "ExecutorMovimentos.h"
#include "ImuSimulado.h"
#include "LabirintoSimulado.h"
#include "Odometria.h"
#include "RoboSimulado.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint32_t CICLO_MS = 10;
constexpr uint32_t PERIODO_TOF_MS = 33;
constexpr uint32_t DEZ_MINUTOS_MS = 10u * 60u * 1000u;

/// O robô, os sensores e o firmware, ligados como no `main.cpp`.
class Simulacao {
 public:
  Simulacao(const char* texto, const ParametrosRoboSimulado& parametros = ParametrosRoboSimulado{},
            Pose inicial = Pose{{0, 0}, Direcao::Norte}, double desvioLateralMm = 0.0, double desvioRumoGraus = 0.0)
      : robo_(labirinto_, parametros), deslocamento_(robo_), ciclo_(armazenamento_.mapa()) {
    TEST_ASSERT_EQUAL(ResultadoCarga::Carregado, labirinto_.carregar(texto));
    armazenamento_.iniciarCorrida(labirinto_.obterDimensao());
    robo_.posicionar(inicial, desvioLateralMm, desvioRumoGraus);
    deslocamento_.reiniciar(odometria_, inicial);
    imu_.definirVies(12);           // 1,2 °/s parado
    imu_.definirRuidoAlternado(4);  // ±0,4 °/s
    acompanhador_.iniciarCalibracao();
    for (int i = 0; i < 400 && acompanhador_.ultima().calibracao != EstadoCalibracao::Calibrado; i++) {
      acompanhador_.ciclo(CICLO_MS);
    }
    TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, acompanhador_.ultima().calibracao);
    ciclo_.reiniciar(inicial, deslocamento_.motores());
  }

  /// Um ciclo de 10 ms do mundo e dos sensores; devolve a leitura nova do ToF, se houver.
  const DistanciasLaterais* sentir(bool sincronizarDirecao, Direcao direcao) {
    agoraMs_ += CICLO_MS;
    robo_.avancar(CICLO_MS);
    imu_.definirTaxa(robo_.taxaGiroDecimosDps());
    giro_ = acompanhador_.ciclo(CICLO_MS);

    bool nova = false;
    if (agoraMs_ - ultimaTofMs_ >= PERIODO_TOF_MS) {
      ultimaTofMs_ = agoraMs_;
      const DistanciasLaterais bruta = robo_.medirTof();
      tof_.registrar(Lado::Frente, bruta.frenteMm, false, agoraMs_);
      tof_.registrar(Lado::Esquerda, bruta.esquerdaMm, false, agoraMs_);
      tof_.registrar(Lado::Direita, bruta.direitaMm, false, agoraMs_);
      nova = tof_.distancias(agoraMs_, leitura_);
    }
    // Como no `main.cpp`: fora dos giros, a direção da odometria é a do giroscópio.
    if (sincronizarDirecao && odometria_.pose().direcao != direcao) {
      odometria_.definirDirecao(direcao);
    }
    deslocamento_.ciclo(CICLO_MS, odometria_, nova ? &leitura_ : nullptr);
    return nova ? &leitura_ : nullptr;
  }

  /// Um ciclo da tarefa de navegação com o `CicloNavegacao`.
  EstadoCiclo passoCiclo() {
    const DistanciasLaterais* nova = sentir(ciclo_.direcaoDefinida(), ciclo_.direcao());
    EntradaCiclo e;
    e.liberado = true;
    e.pose = odometria_.pose();
    e.poseValida = odometria_.posicaoValida();
    e.posicaoAvancoUm = posicaoAoLongoDoAvancoUm(odometria_);
    e.leituraNova = nova;
    e.giro = giro_;
    e.dtMs = CICLO_MS;
    return ciclo_.atualizar(e, deslocamento_.motores());
  }

  /// Corre até o ciclo parar (objetivo, sem caminho ou falha) ou o tempo acabar.
  EstadoCiclo correr(uint32_t limiteMs = DEZ_MINUTOS_MS) {
    EstadoCiclo estado = ciclo_.estado();
    while (agoraMs_ < limiteMs) {
      estado = passoCiclo();
      if (estado == EstadoCiclo::ObjetivoAlcancado || estado == EstadoCiclo::SemCaminho ||
          estado == EstadoCiclo::Falha) {
        break;
      }
    }
    return estado;
  }

  /// Executa um movimento só, com o `ExecutorMovimentos`, até ele terminar.
  EstadoExecucao executar(ExecutorMovimentos& executor, Movimento movimento, Direcao& direcao) {
    executor.iniciar(movimento);
    EstadoExecucao estado = executor.estado();
    const uint32_t limite = agoraMs_ + 10000;
    while (executor.ocupado() && agoraMs_ < limite) {
      const DistanciasLaterais* nova = sentir(!ehGiro(movimento), direcao);
      EntradaExecucao e;
      e.leituraNova = nova;
      e.giro = giro_;
      e.posicaoAvancoUm = posicaoAoLongoDoAvancoUm(odometria_);
      e.dtMs = CICLO_MS;
      estado = executor.atualizar(e, deslocamento_.motores());
    }
    if (estado == EstadoExecucao::Concluido) {
      direcao = direcaoDepois(direcao, movimento);
      odometria_.definirDirecao(direcao);
    }
    // Deixa o robô assentar um ciclo, como na percepção.
    sentir(true, direcao);
    return estado;
  }

  const RoboSimulado& robo() const { return robo_; }
  const Odometria& odometria() const { return odometria_; }
  const CicloNavegacao& ciclo() const { return ciclo_; }
  const LabirintoSimulado& labirinto() const { return labirinto_; }
  const Mapa& mapa() const { return armazenamento_.mapa(); }
  uint32_t agoraMs() const { return agoraMs_; }
  const LeituraGiro& giro() const { return giro_; }

 private:
  LabirintoSimulado labirinto_;
  RoboSimulado robo_;
  DeslocamentoEstimado deslocamento_;
  Odometria odometria_;
  ImuSimulado imu_;
  AcompanhadorGiro acompanhador_{imu_};
  CombinadorTof tof_;
  ArmazenamentoMapa armazenamento_;
  CicloNavegacao ciclo_;
  DistanciasLaterais leitura_ = {0, 0, 0};
  LeituraGiro giro_;
  uint32_t agoraMs_ = 0;
  uint32_t ultimaTofMs_ = 0;
};

void registrar(const char* texto) { TEST_MESSAGE(texto); }

/// Toda parede que o robô gravou no mapa confere com o labirinto de verdade.
void conferirMapaComOLabirinto(const Mapa& mapa, const LabirintoSimulado& labirinto, uint32_t& conhecidas) {
  conhecidas = 0;
  for (uint8_t l = 0; l < labirinto.obterLinhas(); l++) {
    for (uint8_t c = 0; c < labirinto.obterColunas(); c++) {
      for (uint8_t d = 0; d < 4; d++) {
        const Direcao direcao = static_cast<Direcao>(d);
        const EstadoParede estado = mapa.obterParede({l, c}, direcao);
        if (estado == EstadoParede::Desconhecido) {
          continue;
        }
        conhecidas++;
        const bool parede = labirinto.temParede({l, c}, direcao);
        char msg[96];
        std::snprintf(msg, sizeof(msg), "parede (%u,%u) lado %u", l, c, d);
        TEST_ASSERT_EQUAL_MESSAGE(parede ? EstadoParede::Parede : EstadoParede::Livre, estado, msg);
      }
    }
  }
}

/// Confere a corrida completa: chegou ao objetivo dentro dos 10 minutos, a célula real e a da
/// odometria são o objetivo e todas as paredes gravadas conferem com o labirinto.
/// @param exigirSemContato Com o robô ideal, nenhum contato com as paredes. Com o robô imperfeito o
///        contato só é registrado: a folga do giro no lugar é de 6 mm (veja
///        `test_folga_do_giro_no_lugar`), menor que o erro de centralização da sintonia provisória.
void conferirChegada(Simulacao& sim, const char* nome, bool exigirSemContato = true) {
  const PosicaoCelula objetivo = sim.ciclo().navegacao().obterObjetivo();
  const PosicaoCelula real = sim.robo().celula();
  char msg[200];
  std::snprintf(msg, sizeof(msg),
                "%s: estado %s em %.1f s, %u celulas, %u giros; real (%u,%u), erro ao longo %.1f mm, lateral %.1f "
                "mm, rumo %.2f graus",
                nome, nomeDoEstadoCiclo(sim.ciclo().estado()), sim.agoraMs() / 1000.0,
                static_cast<unsigned>(sim.ciclo().celulasAvancadas()), static_cast<unsigned>(sim.ciclo().girosFeitos()),
                real.linha, real.coluna, sim.robo().erroAoLongoMm(), sim.robo().erroLateralMm(),
                sim.robo().erroRumoGraus());
  registrar(msg);
  TEST_ASSERT_EQUAL_MESSAGE(EstadoCiclo::ObjetivoAlcancado, sim.ciclo().estado(), msg);
  if (sim.robo().colidiu()) {
    std::snprintf(msg, sizeof(msg), "%s: o chassi encostou numa parede em (%.1f, %.1f) mm, rumo %.2f graus, %u ms em contato",
                  nome, sim.robo().xColisaoMm(), sim.robo().yColisaoMm(), sim.robo().rumoColisaoGraus(),
                  static_cast<unsigned>(sim.robo().msEmContato()));
    if (exigirSemContato) {
      TEST_FAIL_MESSAGE(msg);
    }
    registrar(msg);
  }
  TEST_ASSERT_EQUAL_UINT8(objetivo.linha, real.linha);
  TEST_ASSERT_EQUAL_UINT8(objetivo.coluna, real.coluna);
  TEST_ASSERT_EQUAL_UINT8(objetivo.linha, sim.odometria().pose().celula.linha);
  TEST_ASSERT_EQUAL_UINT8(objetivo.coluna, sim.odometria().pose().celula.coluna);
  TEST_ASSERT_LESS_THAN_UINT32(DEZ_MINUTOS_MS, sim.agoraMs());
  uint32_t conhecidas = 0;
  conferirMapaComOLabirinto(sim.mapa(), sim.labirinto(), conhecidas);
  TEST_ASSERT_GREATER_THAN_UINT32(0, conhecidas);
}

/// Robô mais lento que a tabela, com as rodas desiguais: o caso que o modo estimado erra.
ParametrosRoboSimulado roboImperfeito() {
  ParametrosRoboSimulado p;
  p.fatorEsquerda = 0.92;
  p.fatorDireita = 0.97;
  return p;
}

}  // namespace

// ---- Movimentos isolados (o erro de cada comando)

/// Critério 1 e 3 (simulação): 3 células em linha reta pelo corredor da coluna 0 do 4×4, com o
/// robô 8% mais lento que a tabela. Registra o erro de parada em cada célula.
void test_avancar_uma_celula_tres_vezes_no_corredor() {
  Simulacao sim(exemplos::LABIRINTO_4X4, roboImperfeito());
  ExecutorMovimentos executor;
  Direcao direcao = Direcao::Norte;
  double piorErro = 0.0;
  for (uint8_t i = 1; i <= 3; i++) {
    TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, sim.executar(executor, Movimento::AvancarCelula, direcao));
    const double erro = sim.robo().erroAoLongoMm();
    char msg[160];
    std::snprintf(msg, sizeof(msg), "avanco %u: real (%u,%u), erro ao longo %.1f mm, lateral %.1f mm, %u ms", i,
                  sim.robo().celula().linha, sim.robo().celula().coluna, erro, sim.robo().erroLateralMm(),
                  static_cast<unsigned>(executor.tempoMs()));
    registrar(msg);
    TEST_ASSERT_EQUAL_UINT8(i, sim.robo().celula().linha);
    TEST_ASSERT_EQUAL_UINT8(0, sim.robo().celula().coluna);
    TEST_ASSERT_EQUAL_UINT8(i, sim.odometria().pose().celula.linha);
    piorErro = std::fabs(erro) > piorErro ? std::fabs(erro) : piorErro;
  }
  TEST_ASSERT_TRUE_MESSAGE(piorErro <= 25.0, "erro de parada acima de 25 mm");
  TEST_ASSERT_FALSE(sim.robo().colidiu());
}

/// Critério 1 (simulação): giros de 90° para os dois lados e de 180°, com o viés do giroscópio
/// calibrado e as rodas desiguais. Registra o erro de rumo de cada giro.
void test_girar_90_e_180_no_lugar() {
  Simulacao sim(exemplos::LABIRINTO_4X4, roboImperfeito());
  ExecutorMovimentos executor;
  Direcao direcao = Direcao::Norte;
  const Movimento giros[] = {Movimento::Girar90Direita, Movimento::Girar90Esquerda, Movimento::Girar180,
                             Movimento::Girar90Esquerda, Movimento::Girar180, Movimento::Girar90Direita};
  for (Movimento giro : giros) {
    TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, sim.executar(executor, giro, direcao));
    char msg[120];
    std::snprintf(msg, sizeof(msg), "%s: rumo real %.2f graus (erro %.2f), %u ms", nomeDoMovimento(giro),
                  sim.robo().rumoGraus(), sim.robo().erroRumoGraus(), static_cast<unsigned>(executor.tempoMs()));
    registrar(msg);
    TEST_ASSERT_TRUE_MESSAGE(std::fabs(sim.robo().erroRumoGraus()) <= 3.0, msg);
    TEST_ASSERT_EQUAL(direcao, sim.odometria().pose().direcao);
  }
  // Os giros não tiram o robô da célula nem o encostam.
  TEST_ASSERT_EQUAL_UINT8(0, sim.robo().celula().linha);
  TEST_ASSERT_EQUAL_UINT8(0, sim.robo().celula().coluna);
  TEST_ASSERT_FALSE(sim.robo().colidiu());
  // Seis giros que somam 0°: o rumo real volta ao Norte sem acumular erro.
  TEST_ASSERT_EQUAL(Direcao::Norte, direcao);
}

/// Giro seguido de avanço: a odometria adota a direção do giroscópio, e o avanço vai para o lado
/// certo mesmo com a diferença das rodas mostrando outro ângulo.
void test_girar_e_avancar_pela_direcao_do_giroscopio() {
  Simulacao sim(exemplos::LABIRINTO_4X4, roboImperfeito(), Pose{{1, 0}, Direcao::Norte});
  ExecutorMovimentos executor;
  Direcao direcao = Direcao::Norte;
  // (1, 0) tem a direita (Leste) livre no 4×4 de exemplo.
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, sim.executar(executor, Movimento::Girar90Direita, direcao));
  TEST_ASSERT_EQUAL(Direcao::Leste, sim.odometria().pose().direcao);
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, sim.executar(executor, Movimento::AvancarCelula, direcao));
  TEST_ASSERT_EQUAL_UINT8(1, sim.robo().celula().linha);
  TEST_ASSERT_EQUAL_UINT8(1, sim.robo().celula().coluna);
  TEST_ASSERT_EQUAL_UINT8(1, sim.odometria().pose().celula.coluna);
  TEST_ASSERT_FALSE(sim.robo().colidiu());
}

/// Uma parede na frente do destino: a parada pela parede vale e o robô fica no centro.
void test_avanco_ate_a_parede_para_no_centro() {
  // (3, 0) virado para o Leste no 4×4: uma célula livre e depois parede (frente = 270 do centro).
  Simulacao sim(exemplos::LABIRINTO_4X4, roboImperfeito(), Pose{{3, 0}, Direcao::Leste});
  ExecutorMovimentos executor;
  Direcao direcao = Direcao::Leste;
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, sim.executar(executor, Movimento::AvancarCelula, direcao));
  char msg[100];
  std::snprintf(msg, sizeof(msg), "ate a parede: erro ao longo %.1f mm", sim.robo().erroAoLongoMm());
  registrar(msg);
  TEST_ASSERT_EQUAL_UINT8(1, sim.robo().celula().coluna);
  TEST_ASSERT_TRUE_MESSAGE(std::fabs(sim.robo().erroAoLongoMm()) <= 15.0, msg);
  TEST_ASSERT_FALSE(sim.robo().colidiu());
}

// ---- Corrida completa: integração com o algoritmo de resolução (critério 2)

void test_corrida_no_4x4_chega_ao_objetivo() {
  Simulacao sim(exemplos::LABIRINTO_4X4);
  sim.correr();
  conferirChegada(sim, "4x4");
}

void test_corrida_no_8x4_chega_ao_objetivo() {
  Simulacao sim(exemplos::LABIRINTO_8X4);
  sim.correr();
  conferirChegada(sim, "8x4");
}

void test_corrida_no_12x4_chega_ao_objetivo() {
  Simulacao sim(exemplos::LABIRINTO_12X4);
  sim.correr();
  conferirChegada(sim, "12x4");
}

/// Robô imperfeito, largando torto e fora do centro: chega ao objetivo, com o mapa e a pose certos.
void test_corrida_com_robo_imperfeito_e_largada_torta() {
  Simulacao sim(exemplos::LABIRINTO_4X4, roboImperfeito(), Pose{{0, 0}, Direcao::Norte}, 8.0, 3.0);
  sim.correr();
  conferirChegada(sim, "4x4 imperfeito e torto", false);
}

void test_corrida_com_robo_imperfeito_no_12x4() {
  Simulacao sim(exemplos::LABIRINTO_12X4, roboImperfeito());
  sim.correr();
  conferirChegada(sim, "12x4 imperfeito", false);
}

/// No 8×4 de exemplo, o caminho otimista do começo (sem paredes conhecidas) esbarra em paredes que
/// a percepção grava no caminho, e o Flood Fill replaneja até chegar, sem ficar rodando.
void test_corrida_replaneja_e_nao_repete_celulas_sem_fim() {
  Simulacao sim(exemplos::LABIRINTO_8X4, roboImperfeito());
  sim.correr();
  conferirChegada(sim, "8x4 imperfeito", false);
  // 32 células: mesmo explorando, não passa de 3 visitas por célula em média.
  TEST_ASSERT_LESS_THAN_UINT32(96, sim.ciclo().celulasAvancadas());
}

// ---- Folga do giro no lugar

/// O chassi de 120 × 100 mm gira no lugar num corredor de 168 mm: a quina passa a 78,1 mm do
/// centro, e a parede fica a 84 mm. Sobram 5,9 mm, e qualquer erro lateral (ou de parada) maior
/// que isso encosta a quina na parede durante o giro. É uma restrição da mecânica, registrada aqui
/// para a bancada e para a sintonia da centralização (#191).
void test_folga_do_giro_no_lugar() {
  const ParametrosRoboSimulado p;
  const double raioDaQuina = std::sqrt(p.meiaLarguraMm * p.meiaLarguraMm + p.meioComprimentoMm * p.meioComprimentoMm);
  const double folga = DISTANCIA_CENTRO_A_PAREDE_MM - raioDaQuina;
  char msg[100];
  std::snprintf(msg, sizeof(msg), "folga do giro no lugar: %.1f mm (quina a %.1f mm do centro)", folga, raioDaQuina);
  registrar(msg);
  TEST_ASSERT_TRUE(folga > 5.5 && folga < 6.5);

  // Dentro da folga: o giro de 90° não encosta.
  {
    Simulacao sim(exemplos::LABIRINTO_4X4, ParametrosRoboSimulado{}, Pose{{0, 0}, Direcao::Norte}, 4.0);
    ExecutorMovimentos executor;
    Direcao direcao = Direcao::Norte;
    TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, sim.executar(executor, Movimento::Girar90Direita, direcao));
    TEST_ASSERT_FALSE(sim.robo().colidiu());
  }
  // Fora dela: a quina encosta na parede lateral.
  {
    Simulacao sim(exemplos::LABIRINTO_4X4, ParametrosRoboSimulado{}, Pose{{0, 0}, Direcao::Norte}, 8.0);
    ExecutorMovimentos executor;
    Direcao direcao = Direcao::Norte;
    sim.executar(executor, Movimento::Girar90Direita, direcao);
    TEST_ASSERT_TRUE(sim.robo().colidiu());
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_avancar_uma_celula_tres_vezes_no_corredor);
  RUN_TEST(test_girar_90_e_180_no_lugar);
  RUN_TEST(test_girar_e_avancar_pela_direcao_do_giroscopio);
  RUN_TEST(test_avanco_ate_a_parede_para_no_centro);
  RUN_TEST(test_corrida_no_4x4_chega_ao_objetivo);
  RUN_TEST(test_corrida_no_8x4_chega_ao_objetivo);
  RUN_TEST(test_corrida_no_12x4_chega_ao_objetivo);
  RUN_TEST(test_corrida_com_robo_imperfeito_e_largada_torta);
  RUN_TEST(test_corrida_com_robo_imperfeito_no_12x4);
  RUN_TEST(test_corrida_replaneja_e_nao_repete_celulas_sem_fim);
  RUN_TEST(test_folga_do_giro_no_lugar);
  return UNITY_END();
}
