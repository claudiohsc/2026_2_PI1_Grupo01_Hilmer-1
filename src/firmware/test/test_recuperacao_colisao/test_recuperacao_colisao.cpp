/// @file test_recuperacao_colisao.cpp
/// @brief Testes unitários da recuperação de colisão (#192): o recuo do `ExecutorMovimentos` e a
/// etapa `Recuperando` do `CicloNavegacao`, com entradas montadas à mão e motores simulados.
///
/// O robô andando num labirinto, batendo de verdade e se recuperando fica em
/// `test_integracao_colisao`. Roda no computador: `pio test -e native -f test_recuperacao_colisao`.

#include <unity.h>

#include "CicloNavegacao.h"
#include "ConfiguracaoColisao.h"
#include "ExecutorMovimentos.h"
#include "Mapa.h"
#include "MotoresSimulados.h"
#include "Movimento.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint16_t LONGE = 1000;       ///< Frente livre, medida (leitura da face do sensor).
constexpr uint16_t SEM_ALVO = 1200;    ///< ToF sem ver nada: a leitura fica no alcance máximo.
constexpr uint16_t PAREDE_LADO = 54;   ///< Parede lateral com o robô centralizado (84 − 30).
constexpr int64_t UM_MM = 1000;

LeituraGiro giroValido(int32_t anguloMiligraus = 0) {
  LeituraGiro giro;
  giro.valida = true;
  giro.calibracao = EstadoCalibracao::Calibrado;
  giro.anguloMiligraus = anguloMiligraus;
  return giro;
}

EntradaExecucao entradaExecucao(const DistanciasLaterais* leitura, int64_t posicaoUm, LeituraGiro giro = giroValido()) {
  EntradaExecucao e;
  e.leituraNova = leitura;
  e.giro = giro;
  e.posicaoAvancoUm = posicaoUm;
  e.dtMs = 10;
  return e;
}

/// Roda o recuo: a posição vai caindo `passoUm` por ciclo, como se o robô andasse de ré.
EstadoExecucao recuar(ExecutorMovimentos& executor, MotoresSimulados& motores, int64_t inicioUm, int64_t passoUm,
                      uint32_t ciclos) {
  EstadoExecucao estado = executor.estado();
  int64_t posicao = inicioUm;
  for (uint32_t i = 0; i < ciclos && executor.ocupado(); i++) {
    estado = executor.atualizar(entradaExecucao(nullptr, posicao), motores);
    posicao -= passoUm;
  }
  return estado;
}

}  // namespace

// ---- O recuo do executor

void test_o_recuo_nao_e_um_movimento_de_giro_nem_avanco() {
  TEST_ASSERT_TRUE(ehManobra(Movimento::Recuar));
  TEST_ASSERT_FALSE(ehGiro(Movimento::Recuar));
  TEST_ASSERT_EQUAL_INT32(0, variacaoRumoMiligraus(Movimento::Recuar));
  TEST_ASSERT_EQUAL(Direcao::Leste, direcaoDepois(Direcao::Leste, Movimento::Recuar));
  TEST_ASSERT_EQUAL_STRING("recuar", nomeDoMovimento(Movimento::Recuar));
}

void test_o_recuo_so_comeca_por_iniciar_recuo_e_com_distancia() {
  ExecutorMovimentos executor;
  TEST_ASSERT_FALSE(executor.iniciar(Movimento::Recuar));
  TEST_ASSERT_FALSE(executor.iniciarRecuo(0));
  TEST_ASSERT_EQUAL(EstadoExecucao::Ocioso, executor.estado());
  TEST_ASSERT_TRUE(executor.iniciarRecuo(30));
  TEST_ASSERT_EQUAL(Movimento::Recuar, executor.movimento());
  TEST_ASSERT_EQUAL(EstadoExecucao::Executando, executor.estado());
  // Ocupado: não aceita outro movimento nem outro recuo.
  TEST_ASSERT_FALSE(executor.iniciarRecuo(30));
  TEST_ASSERT_FALSE(executor.iniciar(Movimento::AvancarCelula));
}

void test_o_recuo_anda_de_re_e_para_na_distancia() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  TEST_ASSERT_TRUE(executor.iniciarRecuo(30));

  // A cada ciclo a posição cai 2 mm: depois de 14 ciclos ainda faltam 2 mm.
  EstadoExecucao estado = recuar(executor, motores, 34 * UM_MM, 2 * UM_MM, 14);
  TEST_ASSERT_EQUAL(EstadoExecucao::Executando, estado);
  TEST_ASSERT_TRUE_MESSAGE(motores.ultimaPotencia().esquerda < 0 && motores.ultimaPotencia().direita < 0,
                           "as duas rodas andam de ré");

  estado = recuar(executor, motores, 6 * UM_MM, 2 * UM_MM, 3);
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, estado);
  TEST_ASSERT_TRUE(motores.freados());
  TEST_ASSERT_FALSE(executor.ocupado());
}

void test_o_recuo_mantem_o_rumo_pelo_giroscopio() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  executor.definirRumoAlvo(0);
  TEST_ASSERT_TRUE(executor.iniciarRecuo(30));
  // Primeiro ciclo fixa o início do trecho; o robô está 5° à esquerda do rumo (ângulo positivo).
  executor.atualizar(entradaExecucao(nullptr, 0, giroValido(5000)), motores);
  executor.atualizar(entradaExecucao(nullptr, -1 * UM_MM, giroValido(5000)), motores);
  // De ré, a correção é a mesma do realinhamento: a roda esquerda recebe mais ré ou menos, conforme
  // o erro, e as duas rodas seguem andando para trás.
  const PotenciaRodas p = motores.ultimaPotencia();
  TEST_ASSERT_TRUE(p.esquerda < 0 && p.direita < 0);
  TEST_ASSERT_TRUE_MESSAGE(p.esquerda != p.direita, "o rumo errado muda a diferença entre as rodas");
}

void test_o_recuo_falha_por_tempo_se_o_robo_nao_anda() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  TEST_ASSERT_TRUE(executor.iniciarRecuo(30));
  EstadoExecucao estado = EstadoExecucao::Executando;
  // Preso: a posição não muda e o trecho passa do tempo máximo (3 s).
  for (int i = 0; i < 400 && executor.ocupado(); i++) {
    estado = executor.atualizar(entradaExecucao(nullptr, 0), motores);
  }
  TEST_ASSERT_EQUAL(EstadoExecucao::Falha, estado);
  TEST_ASSERT_EQUAL(FalhaExecucao::TempoEsgotado, executor.falha());
  TEST_ASSERT_TRUE(motores.freados());
}

void test_o_recuo_termina_no_centro_quando_a_parede_da_frente_esta_a_vista() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  TEST_ASSERT_TRUE(executor.iniciarRecuo(40));
  // Encostado na parede: a leitura da face é ~0 e o centro está a 50 mm dela. Ainda não é o centro.
  const DistanciasLaterais encostado{2, PAREDE_LADO, PAREDE_LADO};
  TEST_ASSERT_EQUAL(EstadoExecucao::Executando, executor.atualizar(entradaExecucao(&encostado, 0), motores));
  // Quase no centro (parede a 84 mm do centro = 34 mm da face): conclui sem esperar os 40 mm.
  const DistanciasLaterais noCentro{34, PAREDE_LADO, PAREDE_LADO};
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, executor.atualizar(entradaExecucao(&noCentro, -20 * UM_MM), motores));
  TEST_ASSERT_TRUE(motores.freados());
}

void test_o_recuo_ignora_parede_de_outra_celula_e_frente_sem_alvo() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  TEST_ASSERT_TRUE(executor.iniciarRecuo(40));
  // A frente em 300 mm da face é a parede da célula seguinte: não é referência do centro.
  const DistanciasLaterais longe{300, PAREDE_LADO, PAREDE_LADO};
  TEST_ASSERT_EQUAL(EstadoExecucao::Executando, executor.atualizar(entradaExecucao(&longe, 0), motores));
  const DistanciasLaterais semAlvo{8190, PAREDE_LADO, PAREDE_LADO};
  TEST_ASSERT_EQUAL(EstadoExecucao::Executando, executor.atualizar(entradaExecucao(&semAlvo, -1 * UM_MM), motores));
  // Mais perto do que o centro: o robô ainda não voltou até lá.
  const DistanciasLaterais perto{20, PAREDE_LADO, PAREDE_LADO};
  TEST_ASSERT_EQUAL(EstadoExecucao::Executando, executor.atualizar(entradaExecucao(&perto, -2 * UM_MM), motores));
}

void test_cancelar_o_recuo_freia() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  TEST_ASSERT_TRUE(executor.iniciarRecuo(40));
  executor.atualizar(entradaExecucao(nullptr, 0), motores);
  executor.cancelar(motores);
  TEST_ASSERT_EQUAL(EstadoExecucao::Ocioso, executor.estado());
  TEST_ASSERT_TRUE(motores.freados());
}

// ---- A recuperação no ciclo de navegação

namespace {

/// O ciclo num 4×4 vazio, na partida, virado para o Norte, pronto para a primeira colisão.
struct CicloDeTeste {
  Mapa mapa{DimensaoMapa::Labirinto4x4};
  MotoresSimulados motores;
  ConfiguracaoColisao config;
  CicloNavegacao ciclo;

  explicit CicloDeTeste(const ConfiguracaoColisao& c = ConfiguracaoColisao{})
      : config(c),
        ciclo(mapa, ConfiguracaoExecucao{}, ConfiguracaoControle{}, ConfiguracaoGiro{}, ConfiguracaoRealinhamento{}, c) {
    ciclo.reiniciar(Pose{{0, 0}, Direcao::Norte}, motores);
  }

  EntradaCiclo entradaBase(const DistanciasLaterais* leitura, int64_t posicaoUm = 0,
                           Pose pose = Pose{{0, 0}, Direcao::Norte}) const {
    EntradaCiclo e;
    e.liberado = true;
    e.pose = pose;
    e.poseValida = true;
    e.posicaoAvancoUm = posicaoUm;
    e.leituraNova = leitura;
    e.giro = giroValido();
    e.dtMs = 10;
    return e;
  }

  EstadoCiclo passo(const DistanciasLaterais* leitura, int64_t posicaoUm = 0, TipoColisao colisao = TipoColisao::Nenhuma,
                    Pose pose = Pose{{0, 0}, Direcao::Norte}) {
    EntradaCiclo e = entradaBase(leitura, posicaoUm, pose);
    e.colisao = colisao;
    return ciclo.atualizar(e, motores);
  }

  /// Largada, percepção com a mesma leitura e o primeiro ciclo do avanço.
  void ate_avancando(const DistanciasLaterais& leitura) {
    for (int i = 0; i < 4; i++) {
      passo(&leitura);
    }
    passo(&leitura);
    TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, ciclo.estado());
  }

  /// Bate avançando, a 34 mm do centro, com o tipo dado.
  EstadoCiclo bater(TipoColisao tipo = TipoColisao::Impacto, int64_t posicaoUm = 34 * UM_MM) {
    return passo(nullptr, posicaoUm, tipo);
  }

  /// Termina o recuo andando de ré até o centro e a percepção de novo.
  void recuarEPerceber(const DistanciasLaterais& leitura, int64_t inicioUm = 34 * UM_MM) {
    int64_t posicao = inicioUm;
    for (int i = 0; i < 300 && ciclo.estado() == EstadoCiclo::Recuperando; i++) {
      passo(nullptr, posicao);
      posicao -= 2 * UM_MM;
    }
    TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, ciclo.estado());
    for (int i = 0; i < 3; i++) {
      passo(&leitura);
    }
  }
};

}  // namespace

void test_colisao_avancando_vai_para_recuperando_e_o_robo_recua() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.ate_avancando(corredor);

  TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.bater());
  TEST_ASSERT_EQUAL_UINT32(1, t.ciclo.colisoes());
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, t.ciclo.ultimaColisao());
  TEST_ASSERT_FALSE_MESSAGE(t.ciclo.emMovimento(), "recuperando não é movimento conduzido: a detecção desliga");
  TEST_ASSERT_EQUAL(MovimentoRobo::Manobrando, t.ciclo.movimentoRobo());
  TEST_ASSERT_EQUAL(Movimento::Recuar, t.ciclo.executor().movimento());
  TEST_ASSERT_FALSE(t.ciclo.navegacao().passoPendente());

  // O recuo manda as rodas para trás.
  t.passo(nullptr, 34 * UM_MM);
  TEST_ASSERT_TRUE(t.motores.ultimaPotencia().esquerda < 0 && t.motores.ultimaPotencia().direita < 0);
}

void test_depois_do_recuo_o_ciclo_percebe_de_novo_na_mesma_celula_sem_mexer_no_mapa() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.ate_avancando(corredor);
  const EstadoParede oeste = t.mapa.obterParede({0, 0}, Direcao::Oeste);
  const EstadoParede leste = t.mapa.obterParede({0, 0}, Direcao::Leste);
  const EstadoParede norte = t.mapa.obterParede({0, 0}, Direcao::Norte);

  t.bater();
  int64_t posicao = 34 * UM_MM;
  for (int i = 0; i < 300 && t.ciclo.estado() == EstadoCiclo::Recuperando; i++) {
    t.passo(nullptr, posicao);
    posicao -= 2 * UM_MM;
  }

  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.estado());
  TEST_ASSERT_EQUAL_UINT32(1, t.ciclo.recuperacoes());
  TEST_ASSERT_TRUE(t.motores.freados());
  // A pose da navegação e o mapa ficaram como estavam.
  TEST_ASSERT_EQUAL_UINT8(0, t.ciclo.navegacao().obterPosicao().linha);
  TEST_ASSERT_EQUAL_UINT8(0, t.ciclo.navegacao().obterPosicao().coluna);
  TEST_ASSERT_EQUAL(Direcao::Norte, t.ciclo.direcao());
  TEST_ASSERT_EQUAL(oeste, t.mapa.obterParede({0, 0}, Direcao::Oeste));
  TEST_ASSERT_EQUAL(leste, t.mapa.obterParede({0, 0}, Direcao::Leste));
  TEST_ASSERT_EQUAL(norte, t.mapa.obterParede({0, 0}, Direcao::Norte));
  TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.paredesPorColisao());

  // Percebe e volta a avançar.
  for (int i = 0; i < 3; i++) {
    t.passo(&corredor);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, t.ciclo.estado());
}

void test_o_recuo_vai_ate_o_centro_pela_odometria_com_minimo_e_maximo() {
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  {  // 34 mm além do centro: recua 34 mm.
    CicloDeTeste t;
    t.ate_avancando(corredor);
    t.bater(TipoColisao::Impacto, 34 * UM_MM);
    t.passo(nullptr, 34 * UM_MM);
    t.passo(nullptr, 1 * UM_MM);  // andou 33 mm de ré: falta 1 mm
    TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.ciclo.estado());
    t.passo(nullptr, 0);
    TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.estado());
  }
  {  // No centro (uma batida no giro, por exemplo): recua o mínimo, 10 mm.
    CicloDeTeste t;
    t.ate_avancando(corredor);
    t.bater(TipoColisao::Impacto, 0);
    t.passo(nullptr, 0);
    t.passo(nullptr, -9 * UM_MM);
    TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.ciclo.estado());
    t.passo(nullptr, -11 * UM_MM);
    TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.estado());
  }
  {  // Muito além do centro (odometria errada): não recua mais que o máximo, 40 mm.
    CicloDeTeste t;
    t.ate_avancando(corredor);
    t.bater(TipoColisao::Impacto, 80 * UM_MM);
    t.passo(nullptr, 80 * UM_MM);
    t.passo(nullptr, 41 * UM_MM);
    TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.ciclo.estado());
    t.passo(nullptr, 39 * UM_MM);
    TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.estado());
  }
}

void test_sem_pose_valida_o_recuo_e_o_fixo() {
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  CicloDeTeste t;
  t.ate_avancando(corredor);
  // Sem pose a odometria não diz onde o robô está: recua a distância configurada (40 mm), e não o mínimo.
  EntradaCiclo batida = t.entradaBase(nullptr, 34 * UM_MM);
  batida.poseValida = false;
  batida.colisao = TipoColisao::Impacto;
  TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.ciclo.atualizar(batida, t.motores));
  t.passo(nullptr, 0);
  t.passo(nullptr, -39 * UM_MM);
  TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.ciclo.estado());
  t.passo(nullptr, -41 * UM_MM);
  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.estado());
}

void test_recuo_recusado_pelo_executor_para_o_ciclo_em_falha() {
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  ConfiguracaoColisao config;
  config.recuoMm = 0;  // recuo de 0 mm: o executor recusa
  config.recuoMinimoMm = 0;
  CicloDeTeste t(config);
  t.ate_avancando(corredor);
  TEST_ASSERT_EQUAL(EstadoCiclo::Falha, t.bater());
  TEST_ASSERT_EQUAL(FalhaExecucao::Nenhuma, t.ciclo.falhaExecucao());
  TEST_ASSERT_EQUAL_UINT32(1, t.ciclo.colisoes());
  TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.recuperacoes());
}

void test_colisao_fora_do_movimento_e_ignorada() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  // Aguardando a largada (sem permissão para andar).
  EntradaCiclo espera = t.entradaBase(&corredor);
  espera.liberado = false;
  espera.colisao = TipoColisao::Impacto;
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, t.ciclo.atualizar(espera, t.motores));
  TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.colisoes());
  // Percebendo (o primeiro ciclo liberado larga e vai para percebendo; o robô está parado).
  t.passo(&corredor);
  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.estado());
  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.passo(nullptr, 0, TipoColisao::FrenteParada));
  TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.colisoes());
}

void test_colisao_durante_a_recuperacao_nao_conta_de_novo() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.ate_avancando(corredor);
  t.bater();
  TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.passo(nullptr, 34 * UM_MM, TipoColisao::Impacto));
  TEST_ASSERT_EQUAL_UINT32(1, t.ciclo.colisoes());
}

void test_so_a_segunda_batida_na_mesma_frente_grava_a_parede_se_o_tof_nao_a_viu() {
  CicloDeTeste t;
  const DistanciasLaterais cego{SEM_ALVO, PAREDE_LADO, PAREDE_LADO};
  t.ate_avancando(cego);
  TEST_ASSERT_EQUAL(EstadoParede::Livre, t.mapa.obterParede({0, 0}, Direcao::Norte));

  t.bater();
  t.recuarEPerceber(cego);
  TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.paredesPorColisao());
  TEST_ASSERT_EQUAL(EstadoParede::Livre, t.mapa.obterParede({0, 0}, Direcao::Norte));

  // Avança de novo e bate no mesmo lugar: agora a parede é gravada, e o caminho é replanejado.
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, t.ciclo.estado());
  t.bater();
  TEST_ASSERT_EQUAL_UINT32(1, t.ciclo.paredesPorColisao());
  TEST_ASSERT_EQUAL(EstadoParede::Parede, t.mapa.obterParede({0, 0}, Direcao::Norte));
}

void test_a_parede_nao_e_gravada_se_o_tof_mede_a_frente_livre() {
  CicloDeTeste t;
  const DistanciasLaterais livre{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.ate_avancando(livre);
  t.bater();
  t.recuarEPerceber(livre);
  t.bater();
  TEST_ASSERT_EQUAL_UINT32(2, t.ciclo.colisoes());
  TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.paredesPorColisao());
  TEST_ASSERT_EQUAL(EstadoParede::Livre, t.mapa.obterParede({0, 0}, Direcao::Norte));
}

void test_a_parede_nao_e_gravada_se_o_robo_nao_esta_onde_a_batida_na_parede_acontece() {
  const DistanciasLaterais cego{SEM_ALVO, PAREDE_LADO, PAREDE_LADO};
  {  // Longe da parede da própria célula (a 3 mm do centro): foi uma raspada em outro lugar.
    CicloDeTeste t;
    t.ate_avancando(cego);
    t.bater(TipoColisao::Impacto, 3 * UM_MM);
    t.recuarEPerceber(cego, 3 * UM_MM);
    t.bater(TipoColisao::Impacto, 3 * UM_MM);
    TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.paredesPorColisao());
  }
  {  // Já além da faixa (no lado da célula seguinte): a parede da frente não é a desta célula.
    CicloDeTeste t;
    t.ate_avancando(cego);
    t.bater(TipoColisao::Impacto, 85 * UM_MM);
    t.recuarEPerceber(cego, 85 * UM_MM);
    t.bater(TipoColisao::Impacto, 85 * UM_MM);
    TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.paredesPorColisao());
  }
  {  // A odometria já está na célula seguinte: não confia na pose.
    CicloDeTeste t;
    t.ate_avancando(cego);
    t.passo(nullptr, 34 * UM_MM, TipoColisao::Impacto, Pose{{1, 0}, Direcao::Norte});
    t.recuarEPerceber(cego);
    t.passo(nullptr, 34 * UM_MM, TipoColisao::Impacto, Pose{{1, 0}, Direcao::Norte});
    TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.paredesPorColisao());
  }
}

void test_colisoes_demais_seguidas_param_o_ciclo_em_falha() {
  ConfiguracaoColisao config;
  config.colisoesSeguidasMaximas = 1;
  CicloDeTeste t(config);
  const DistanciasLaterais cego{SEM_ALVO, PAREDE_LADO, PAREDE_LADO};
  t.ate_avancando(cego);

  // A primeira colisão ainda cabe no máximo; a segunda, sem concluir uma célula, passa dele.
  t.bater();
  TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.ciclo.estado());
  t.recuarEPerceber(cego);
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, t.ciclo.estado());
  t.bater();
  TEST_ASSERT_EQUAL(EstadoCiclo::Falha, t.ciclo.estado());
  TEST_ASSERT_EQUAL(FalhaExecucao::ColisoesSeguidas, t.ciclo.falhaExecucao());
  TEST_ASSERT_TRUE(t.motores.freados());
  TEST_ASSERT_EQUAL_STRING("colisoes_seguidas", nomeDaFalhaExecucao(FalhaExecucao::ColisoesSeguidas));
}

void test_concluir_uma_celula_zera_a_contagem_das_colisoes_seguidas() {
  ConfiguracaoColisao config;
  config.colisoesSeguidasMaximas = 1;
  CicloDeTeste t(config);
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.ate_avancando(corredor);
  t.bater();
  t.recuarEPerceber(corredor);
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, t.ciclo.estado());

  // Conclui a célula (1): a contagem recomeça, e uma colisão a mais não passa do máximo de 1.
  t.passo(&corredor, 0, TipoColisao::Nenhuma, Pose{{0, 0}, Direcao::Norte});
  t.passo(&corredor, 90 * UM_MM, TipoColisao::Nenhuma, Pose{{0, 0}, Direcao::Norte});
  t.passo(&corredor, 178 * UM_MM, TipoColisao::Nenhuma, Pose{{1, 0}, Direcao::Norte});
  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.estado());
  TEST_ASSERT_EQUAL_UINT32(1, t.ciclo.celulasAvancadas());
  for (int i = 0; i < 3; i++) {
    t.passo(&corredor, 180 * UM_MM, TipoColisao::Nenhuma, Pose{{1, 0}, Direcao::Norte});
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, t.ciclo.estado());
  t.passo(nullptr, (180 + 34) * UM_MM, TipoColisao::Impacto, Pose{{1, 0}, Direcao::Norte});
  TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.ciclo.estado());
}

void test_colisao_no_giro_volta_o_rumo_alvo_para_o_da_direcao_atual() {
  CicloDeTeste t;
  // Parede na frente e na esquerda: gira para a direita (Leste), em -90°.
  const DistanciasLaterais precisaGirar{30, PAREDE_LADO, LONGE};
  for (int i = 0; i < 5; i++) {
    t.passo(&precisaGirar);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Girando, t.ciclo.estado());
  TEST_ASSERT_TRUE(t.ciclo.executor().rumoAlvoMiligraus() < 0);

  // Bate a quina no meio do giro, a 20° do rumo inicial.
  EntradaCiclo e = t.entradaBase(nullptr);
  e.giro = giroValido(-20000);
  e.colisao = TipoColisao::Impacto;
  TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.ciclo.atualizar(e, t.motores));

  // O recuo segura o rumo do Norte (0), e não o do giro interrompido.
  TEST_ASSERT_EQUAL(Direcao::Norte, t.ciclo.direcao());
  TEST_ASSERT_EQUAL_INT32(0, t.ciclo.executor().rumoAlvoMiligraus());
  TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.girosFeitos());
}

void test_colisao_no_giro_sem_giroscopio_desfaz_a_variacao_do_giro_no_rumo_alvo() {
  CicloDeTeste t;
  const DistanciasLaterais precisaGirar{30, PAREDE_LADO, LONGE};
  for (int i = 0; i < 5; i++) {
    t.passo(&precisaGirar);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Girando, t.ciclo.estado());
  TEST_ASSERT_EQUAL_INT32(-90000, t.ciclo.executor().rumoAlvoMiligraus());

  // Sem o ângulo do giroscópio não há rumo medido: desfaz só a variação do giro interrompido.
  EntradaCiclo e = t.entradaBase(nullptr);
  e.giro.valida = false;
  e.colisao = TipoColisao::FrenteParada;
  TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.ciclo.atualizar(e, t.motores));
  TEST_ASSERT_EQUAL_INT32(0, t.ciclo.executor().rumoAlvoMiligraus());
  TEST_ASSERT_EQUAL(TipoColisao::FrenteParada, t.ciclo.ultimaColisao());
}

void test_tirar_a_permissao_na_recuperacao_volta_a_aguardar_a_largada() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.ate_avancando(corredor);
  t.bater();
  TEST_ASSERT_EQUAL(EstadoCiclo::Recuperando, t.ciclo.estado());
  EntradaCiclo e = t.entradaBase(nullptr);
  e.liberado = false;
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, t.ciclo.atualizar(e, t.motores));
  TEST_ASSERT_TRUE(t.motores.freados());
}

void test_recuo_travado_pelo_tempo_para_em_falha() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.ate_avancando(corredor);
  t.bater();
  // A posição não muda: o robô não consegue recuar.
  for (int i = 0; i < 400 && t.ciclo.estado() == EstadoCiclo::Recuperando; i++) {
    t.passo(nullptr, 34 * UM_MM);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Falha, t.ciclo.estado());
  TEST_ASSERT_EQUAL(FalhaExecucao::TempoEsgotado, t.ciclo.falhaExecucao());
}

void test_reiniciar_zera_os_contadores_da_colisao() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.ate_avancando(corredor);
  t.bater();
  t.ciclo.reiniciar(Pose{{0, 0}, Direcao::Norte}, t.motores);
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, t.ciclo.estado());
  TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.colisoes());
  TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.recuperacoes());
  TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.paredesPorColisao());
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, t.ciclo.ultimaColisao());
}

void test_nome_do_estado_recuperando() {
  TEST_ASSERT_EQUAL_STRING("recuperando", nomeDoEstadoCiclo(EstadoCiclo::Recuperando));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_o_recuo_nao_e_um_movimento_de_giro_nem_avanco);
  RUN_TEST(test_o_recuo_so_comeca_por_iniciar_recuo_e_com_distancia);
  RUN_TEST(test_o_recuo_anda_de_re_e_para_na_distancia);
  RUN_TEST(test_o_recuo_mantem_o_rumo_pelo_giroscopio);
  RUN_TEST(test_o_recuo_falha_por_tempo_se_o_robo_nao_anda);
  RUN_TEST(test_o_recuo_termina_no_centro_quando_a_parede_da_frente_esta_a_vista);
  RUN_TEST(test_o_recuo_ignora_parede_de_outra_celula_e_frente_sem_alvo);
  RUN_TEST(test_cancelar_o_recuo_freia);
  RUN_TEST(test_colisao_avancando_vai_para_recuperando_e_o_robo_recua);
  RUN_TEST(test_depois_do_recuo_o_ciclo_percebe_de_novo_na_mesma_celula_sem_mexer_no_mapa);
  RUN_TEST(test_o_recuo_vai_ate_o_centro_pela_odometria_com_minimo_e_maximo);
  RUN_TEST(test_sem_pose_valida_o_recuo_e_o_fixo);
  RUN_TEST(test_recuo_recusado_pelo_executor_para_o_ciclo_em_falha);
  RUN_TEST(test_colisao_fora_do_movimento_e_ignorada);
  RUN_TEST(test_colisao_durante_a_recuperacao_nao_conta_de_novo);
  RUN_TEST(test_so_a_segunda_batida_na_mesma_frente_grava_a_parede_se_o_tof_nao_a_viu);
  RUN_TEST(test_a_parede_nao_e_gravada_se_o_tof_mede_a_frente_livre);
  RUN_TEST(test_a_parede_nao_e_gravada_se_o_robo_nao_esta_onde_a_batida_na_parede_acontece);
  RUN_TEST(test_colisoes_demais_seguidas_param_o_ciclo_em_falha);
  RUN_TEST(test_concluir_uma_celula_zera_a_contagem_das_colisoes_seguidas);
  RUN_TEST(test_colisao_no_giro_volta_o_rumo_alvo_para_o_da_direcao_atual);
  RUN_TEST(test_colisao_no_giro_sem_giroscopio_desfaz_a_variacao_do_giro_no_rumo_alvo);
  RUN_TEST(test_tirar_a_permissao_na_recuperacao_volta_a_aguardar_a_largada);
  RUN_TEST(test_recuo_travado_pelo_tempo_para_em_falha);
  RUN_TEST(test_reiniciar_zera_os_contadores_da_colisao);
  RUN_TEST(test_nome_do_estado_recuperando);
  return UNITY_END();
}
