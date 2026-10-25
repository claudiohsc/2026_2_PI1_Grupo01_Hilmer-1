/// @file test_fase_corrida.cpp
/// @brief Testes do estado "Aguardando Largada" (#242): a `FaseCorrida` e o caminho até o servidor
/// (amostra montada, buffer, reenvio e JSON do lote).
/// Roda no computador: `pio test -e native -f test_fase_corrida`.

#include <unity.h>

#include <cstdint>
#include <cstring>

#include "AmostraTelemetria.h"
#include "ContratoTelemetria.h"
#include "ControladorEnvio.h"
#include "EnvioSimulado.h"
#include "FaseCorrida.h"
#include "TravaNula.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint32_t CORRIDA = 7;
constexpr uint32_t PERIODO_CICLO_MS = 1000;  ///< Ritmo da tarefa de telemetria (`PERIODO_TELEMETRIA_MS`).

FaseCorrida calibrada(const ConfiguracaoLargada& config = ConfiguracaoLargada{}) {
  FaseCorrida fase(config);
  fase.calibracaoConcluida();
  return fase;
}

void verificarEstado(EstadoCorrida esperado, EstadoCorrida obtido) {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(esperado), static_cast<uint8_t>(obtido));
}

void verificarFase(FaseCorrida::Fase esperada, FaseCorrida::Fase obtida) {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(esperada), static_cast<uint8_t>(obtida));
}

}  // namespace

// ---------------------------------------------------------------- fases e calibração

void test_comeca_aguardando_a_calibracao_e_nao_registra_nada() {
  FaseCorrida fase;
  verificarFase(FaseCorrida::Fase::AguardandoCalibracao, fase.fase());
  TEST_ASSERT_FALSE(fase.calibrado());
  TEST_ASSERT_FALSE(fase.decidir(0, 0, false).registrar);
  TEST_ASSERT_FALSE(fase.decidir(500, 10000, true).registrar);  // nem largada nem objetivo contam
  verificarFase(FaseCorrida::Fase::AguardandoCalibracao, fase.fase());
}

void test_calibracao_concluida_passa_a_aguardar_a_largada() {
  FaseCorrida fase;
  fase.calibracaoConcluida();
  verificarFase(FaseCorrida::Fase::AguardandoLargada, fase.fase());
  TEST_ASSERT_TRUE(fase.calibrado());
}

void test_calibracao_concluida_de_novo_nao_volta_a_fase() {
  FaseCorrida fase = calibrada();
  fase.decidir(0, 0, false);
  fase.decidir(100, 1000, false);  // largou
  verificarFase(FaseCorrida::Fase::EmCorrida, fase.fase());

  fase.calibracaoConcluida();  // por exemplo, o giroscópio terminou a calibração própria

  verificarFase(FaseCorrida::Fase::EmCorrida, fase.fase());
}

void test_nomes_das_fases() {
  TEST_ASSERT_EQUAL_STRING("aguardando_calibracao", nomeDaFase(FaseCorrida::Fase::AguardandoCalibracao));
  TEST_ASSERT_EQUAL_STRING("aguardando_largada", nomeDaFase(FaseCorrida::Fase::AguardandoLargada));
  TEST_ASSERT_EQUAL_STRING("em_corrida", nomeDaFase(FaseCorrida::Fase::EmCorrida));
  TEST_ASSERT_EQUAL_STRING("encerrada", nomeDaFase(FaseCorrida::Fase::Encerrada));
}

// ---------------------------------------------------------------- aguardando a largada

void test_aguardando_envia_logo_a_primeira_amostra_com_tempo_zero() {
  FaseCorrida fase = calibrada();
  const DecisaoColeta d = fase.decidir(0, 123456, false);
  TEST_ASSERT_TRUE(d.registrar);
  verificarEstado(EstadoCorrida::Aguardando, d.estado);
  TEST_ASSERT_EQUAL_UINT32(0, d.tempoDecorridoMs);
  TEST_ASSERT_FALSE(d.largouAgora);
  TEST_ASSERT_FALSE(d.ultima);
}

void test_aguardando_so_repete_a_amostra_a_cada_periodo() {
  ConfiguracaoLargada config;
  config.periodoAmostraAguardandoMs = 5000;
  FaseCorrida fase = calibrada(config);

  TEST_ASSERT_TRUE(fase.decidir(0, 1000, false).registrar);   // entrou no estado
  TEST_ASSERT_FALSE(fase.decidir(0, 2000, false).registrar);
  TEST_ASSERT_FALSE(fase.decidir(0, 5999, false).registrar);  // 4999 ms depois: ainda não
  TEST_ASSERT_TRUE(fase.decidir(0, 6000, false).registrar);   // exatamente o período
  TEST_ASSERT_FALSE(fase.decidir(0, 7000, false).registrar);
  TEST_ASSERT_TRUE(fase.decidir(0, 11000, false).registrar);
}

void test_aguardando_conta_o_periodo_mesmo_quando_o_millis_da_a_volta() {
  FaseCorrida fase = calibrada();
  TEST_ASSERT_TRUE(fase.decidir(0, 0xFFFFFF00u, false).registrar);
  TEST_ASSERT_FALSE(fase.decidir(0, 0xFFFFFF00u + 3000u, false).registrar);  // já deu a volta no contador
  TEST_ASSERT_TRUE(fase.decidir(0, 0xFFFFFF00u + 5000u, false).registrar);
}

void test_pequeno_deslocamento_parado_nao_conta_como_largada() {
  FaseCorrida fase = calibrada();
  const DecisaoColeta d = fase.decidir(static_cast<int32_t>(DESLOCAMENTO_MINIMO_LARGADA_MM) - 1, 0, false);
  TEST_ASSERT_FALSE(d.largouAgora);
  verificarFase(FaseCorrida::Fase::AguardandoLargada, fase.fase());
}

// ---------------------------------------------------------------- largada

void test_largada_no_deslocamento_minimo_exato() {
  FaseCorrida fase = calibrada();
  const DecisaoColeta d = fase.decidir(static_cast<int32_t>(DESLOCAMENTO_MINIMO_LARGADA_MM), 8000, false);
  TEST_ASSERT_TRUE(d.largouAgora);
  verificarFase(FaseCorrida::Fase::EmCorrida, fase.fase());
}

void test_largada_para_tras_tambem_conta() {
  FaseCorrida fase = calibrada();
  TEST_ASSERT_TRUE(fase.decidir(-static_cast<int32_t>(DESLOCAMENTO_MINIMO_LARGADA_MM), 0, false).largouAgora);
}

void test_deslocamento_extremo_negativo_nao_estoura() {
  FaseCorrida fase = calibrada();
  TEST_ASSERT_TRUE(fase.decidir(INT32_MIN, 0, false).largouAgora);
}

void test_largada_registra_a_primeira_amostra_da_corrida_com_tempo_zero() {
  FaseCorrida fase = calibrada();
  fase.decidir(0, 1000, false);  // aguardava

  const DecisaoColeta d = fase.decidir(25, 9000, false);

  TEST_ASSERT_TRUE(d.largouAgora);
  TEST_ASSERT_TRUE(d.registrar);
  verificarEstado(EstadoCorrida::Explorando, d.estado);
  TEST_ASSERT_EQUAL_UINT32(0, d.tempoDecorridoMs);  // o cronômetro começa na largada, e não na criação
  TEST_ASSERT_FALSE(d.ultima);
}

void test_o_tempo_conta_da_largada_e_nao_do_inicio_da_espera() {
  FaseCorrida fase = calibrada();
  fase.decidir(0, 1000, false);
  fase.decidir(30, 61000, false);  // largou depois de 60 s de espera

  const DecisaoColeta d = fase.decidir(80, 62500, false);

  TEST_ASSERT_FALSE(d.largouAgora);
  verificarEstado(EstadoCorrida::Explorando, d.estado);
  TEST_ASSERT_EQUAL_UINT32(1500, d.tempoDecorridoMs);
}

void test_em_corrida_registra_uma_amostra_por_ciclo() {
  FaseCorrida fase = calibrada();
  fase.decidir(40, 0, false);
  for (uint32_t t = 1000; t <= 5000; t += 1000) {
    TEST_ASSERT_TRUE(fase.decidir(100, t, false).registrar);
  }
}

void test_tempo_decorrido_certo_com_a_volta_do_millis_depois_da_largada() {
  FaseCorrida fase = calibrada();
  fase.decidir(40, 0xFFFFFC00u, false);  // largou perto do fim do contador

  const DecisaoColeta d = fase.decidir(100, 500, false);  // o contador deu a volta: 1024 + 500 ms depois

  TEST_ASSERT_EQUAL_UINT32(1524, d.tempoDecorridoMs);
}

// ---------------------------------------------------------------- fim da corrida

void test_objetivo_encerra_com_amostra_parada_e_a_fase_encerrada() {
  FaseCorrida fase = calibrada();
  fase.decidir(40, 0, false);

  const DecisaoColeta d = fase.decidir(500, 40000, true);

  TEST_ASSERT_TRUE(d.registrar);
  TEST_ASSERT_TRUE(d.ultima);
  TEST_ASSERT_FALSE(d.tempoEsgotado);
  verificarEstado(EstadoCorrida::Parado, d.estado);
  verificarFase(FaseCorrida::Fase::Encerrada, fase.fase());
  TEST_ASSERT_FALSE(fase.decidir(500, 41000, false).registrar);  // depois de encerrada, nada mais
}

void test_o_fim_do_tempo_encerra_exatamente_nos_10_minutos() {
  ConfiguracaoLargada config;
  config.limiteTempoMs = 600000;
  FaseCorrida fase = calibrada(config);
  fase.decidir(40, 1000, false);  // largou em 1000

  const DecisaoColeta quase = fase.decidir(100, 1000 + 599999, false);
  TEST_ASSERT_FALSE(quase.ultima);
  const DecisaoColeta fim = fase.decidir(100, 1000 + 600000, false);
  TEST_ASSERT_TRUE(fim.ultima);
  TEST_ASSERT_TRUE(fim.tempoEsgotado);
  verificarEstado(EstadoCorrida::Parado, fim.estado);
  verificarFase(FaseCorrida::Fase::Encerrada, fase.fase());
}

void test_o_tempo_de_espera_nao_gasta_os_10_minutos() {
  ConfiguracaoLargada config;
  config.limiteTempoMs = 600000;
  FaseCorrida fase = calibrada(config);
  fase.decidir(0, 0, false);
  fase.decidir(0, 700000, false);  // 11 min esperando a largada: nada se esgota

  verificarFase(FaseCorrida::Fase::AguardandoLargada, fase.fase());
  const DecisaoColeta d = fase.decidir(50, 710000, false);
  TEST_ASSERT_TRUE(d.largouAgora);
  TEST_ASSERT_FALSE(d.ultima);
}

void test_objetivo_no_mesmo_ciclo_da_largada_encerra_na_hora() {
  FaseCorrida fase = calibrada();
  const DecisaoColeta d = fase.decidir(50, 5000, true);
  TEST_ASSERT_TRUE(d.largouAgora);
  TEST_ASSERT_TRUE(d.ultima);
  verificarFase(FaseCorrida::Fase::Encerrada, fase.fase());
}

// ---------------------------------------------------------------- reiniciar

void test_reiniciar_em_corrida_volta_a_aguardar_a_largada_e_avisa_de_novo() {
  FaseCorrida fase = calibrada();
  fase.decidir(0, 0, false);
  fase.decidir(60, 2000, false);  // largou
  verificarFase(FaseCorrida::Fase::EmCorrida, fase.fase());

  fase.reiniciar();  // robô recolocado na partida à mão

  verificarFase(FaseCorrida::Fase::AguardandoLargada, fase.fase());
  const DecisaoColeta d = fase.decidir(0, 3000, false);
  TEST_ASSERT_TRUE(d.registrar);  // avisa já, sem esperar o período
  verificarEstado(EstadoCorrida::Aguardando, d.estado);
}

void test_encerrada_so_depois_do_objetivo_ou_do_tempo_e_o_reinicio_nao_desfaz() {
  FaseCorrida fase = calibrada();
  TEST_ASSERT_FALSE(fase.encerrada());  // aguardando a largada
  fase.decidir(60, 0, false);
  TEST_ASSERT_FALSE(fase.encerrada());  // em corrida
  fase.decidir(500, 40000, true);
  TEST_ASSERT_TRUE(fase.encerrada());   // objetivo

  fase.reiniciar();  // comando `Z`: não começa nova tentativa

  TEST_ASSERT_TRUE(fase.encerrada());
  TEST_ASSERT_FALSE(fase.decidir(0, 50000, false).registrar);  // e continua sem enviar nada
}

void test_sem_calibracao_a_fase_nao_esta_encerrada() {
  FaseCorrida fase;
  TEST_ASSERT_FALSE(fase.encerrada());
}

void test_reiniciar_nao_desencerra_nem_pula_a_calibracao() {
  FaseCorrida semCalibracao;
  semCalibracao.reiniciar();
  verificarFase(FaseCorrida::Fase::AguardandoCalibracao, semCalibracao.fase());

  FaseCorrida encerrada = calibrada();
  encerrada.decidir(60, 0, true);
  encerrada.reiniciar();
  verificarFase(FaseCorrida::Fase::Encerrada, encerrada.fase());
}

// ---------------------------------------------------------------- do robô ao servidor (integração)

namespace {

/// O que a tarefa de telemetria faz a cada ciclo: pergunta à fase, monta a amostra e a registra.
struct RoboSimulado {
  EnvioSimulado servidor;
  TravaNula trava;
  ControladorEnvio controlador{servidor, trava};
  FaseCorrida fase = calibrada();
  uint32_t agoraMs = 0;

  RoboSimulado() { controlador.iniciarCorrida(CORRIDA); }

  /// Um ciclo de 1 s: coleta e dá uma volta no reenvio.
  void ciclo(int32_t deslocamentoMm, bool chegouAoObjetivo = false) {
    const DecisaoColeta d = fase.decidir(deslocamentoMm, agoraMs, chegouAoObjetivo);
    if (d.registrar) {
      const Pose partida = {{0, 0}, Direcao::Norte};
      controlador.registrar(montarAmostra(
          partida, d.tempoDecorridoMs, 0, d.estado,
          chegouAoObjetivo ? EventoTelemetria::ObjetivoAlcancado : EventoTelemetria::Nenhum));
    }
    controlador.ciclo(agoraMs);
    agoraMs += PERIODO_CICLO_MS;
  }
};

}  // namespace

void test_integracao_o_servidor_recebe_primeiro_a_espera_e_depois_a_corrida() {
  RoboSimulado robo;

  for (int i = 0; i < 12; ++i) {
    robo.ciclo(0);  // 12 s esperando na partida: amostras em 0 s, 5 s e 10 s
  }
  robo.ciclo(40);   // largou
  robo.ciclo(120);
  robo.ciclo(400, true);  // objetivo

  // Dá tempo de o controlador enviar tudo.
  for (int i = 0; i < 5; ++i) {
    robo.controlador.ciclo(robo.agoraMs);
    robo.agoraMs += PERIODO_CICLO_MS;
  }

  TEST_ASSERT_EQUAL_UINT16(6, robo.servidor.amostrasArmazenadas());
  const EstadoCorrida esperados[6] = {EstadoCorrida::Aguardando, EstadoCorrida::Aguardando, EstadoCorrida::Aguardando,
                                      EstadoCorrida::Explorando, EstadoCorrida::Explorando, EstadoCorrida::Parado};
  for (uint16_t i = 0; i < 6; ++i) {
    verificarEstado(esperados[i], robo.servidor.armazenada(i).estado);
    TEST_ASSERT_EQUAL_UINT32(i + 1, robo.servidor.armazenada(i).numeroSequencia);
  }
  // Na espera, tudo zerado e na célula de partida; o tempo só corre depois da largada.
  for (uint16_t i = 0; i < 3; ++i) {
    TEST_ASSERT_EQUAL_UINT32(0, robo.servidor.armazenada(i).tempoDecorridoMs);
    TEST_ASSERT_EQUAL_INT32(0, robo.servidor.armazenada(i).velocidadeMmPorS);
    TEST_ASSERT_EQUAL_UINT8(0, robo.servidor.armazenada(i).posicaoLinha);
    TEST_ASSERT_EQUAL_UINT8(0, robo.servidor.armazenada(i).posicaoColuna);
  }
  TEST_ASSERT_EQUAL_UINT32(0, robo.servidor.armazenada(3).tempoDecorridoMs);
  TEST_ASSERT_EQUAL_UINT32(1000, robo.servidor.armazenada(4).tempoDecorridoMs);
  TEST_ASSERT_EQUAL_UINT32(2000, robo.servidor.armazenada(5).tempoDecorridoMs);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(EventoTelemetria::ObjetivoAlcancado),
                          static_cast<uint8_t>(robo.servidor.armazenada(5).evento));
  TEST_ASSERT_EQUAL_UINT32(0, robo.servidor.duplicatasIgnoradas());
}

void test_integracao_o_json_do_lote_leva_o_estado_aguardando_do_contrato() {
  RoboSimulado robo;
  robo.ciclo(0);
  robo.ciclo(0);
  robo.controlador.ciclo(robo.agoraMs);
  TEST_ASSERT_EQUAL_UINT16(1, robo.servidor.amostrasArmazenadas());

  char json[512];
  const AmostraTelemetria enviada = robo.servidor.armazenada(0);
  const size_t escritos = serializarLote(CORRIDA, &enviada, 1, json, sizeof(json));

  TEST_ASSERT_GREATER_THAN_UINT32(0, escritos);
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"estado_corrida\":\"aguardando\""));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"posicao_linha\":0"));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"velocidade\":0.000"));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"tempo_decorrido\":0.000"));
  TEST_ASSERT_NULL(std::strstr(json, "\"evento\""));
}

void test_integracao_sem_rede_na_espera_nada_se_perde_e_o_estado_chega_depois() {
  RoboSimulado robo;
  robo.servidor.definirOffline(true);
  for (int i = 0; i < 11; ++i) {
    robo.ciclo(0);  // espera sem rede: 3 amostras ficam no buffer
  }
  robo.ciclo(50);   // larga ainda sem rede
  TEST_ASSERT_EQUAL_UINT16(0, robo.servidor.amostrasArmazenadas());

  robo.servidor.definirOffline(false);
  for (int i = 0; i < 40; ++i) {
    robo.controlador.ciclo(robo.agoraMs);
    robo.agoraMs += PERIODO_CICLO_MS;
  }

  TEST_ASSERT_EQUAL_UINT16(4, robo.servidor.amostrasArmazenadas());
  verificarEstado(EstadoCorrida::Aguardando, robo.servidor.armazenada(0).estado);
  verificarEstado(EstadoCorrida::Aguardando, robo.servidor.armazenada(2).estado);
  verificarEstado(EstadoCorrida::Explorando, robo.servidor.armazenada(3).estado);
  TEST_ASSERT_EQUAL_UINT32(0, robo.controlador.amostrasPerdidas());
}

void test_integracao_a_espera_de_5_minutos_gera_poucas_amostras() {
  RoboSimulado robo;
  for (int i = 0; i < 300; ++i) {
    robo.ciclo(0);  // 300 s esperando
  }
  // Uma a cada 5 s (e a primeira): 60 amostras, e não 300.
  TEST_ASSERT_EQUAL_UINT16(60, robo.controlador.ocupacao() + robo.servidor.amostrasArmazenadas());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_comeca_aguardando_a_calibracao_e_nao_registra_nada);
  RUN_TEST(test_calibracao_concluida_passa_a_aguardar_a_largada);
  RUN_TEST(test_calibracao_concluida_de_novo_nao_volta_a_fase);
  RUN_TEST(test_nomes_das_fases);
  RUN_TEST(test_aguardando_envia_logo_a_primeira_amostra_com_tempo_zero);
  RUN_TEST(test_aguardando_so_repete_a_amostra_a_cada_periodo);
  RUN_TEST(test_aguardando_conta_o_periodo_mesmo_quando_o_millis_da_a_volta);
  RUN_TEST(test_pequeno_deslocamento_parado_nao_conta_como_largada);
  RUN_TEST(test_largada_no_deslocamento_minimo_exato);
  RUN_TEST(test_largada_para_tras_tambem_conta);
  RUN_TEST(test_deslocamento_extremo_negativo_nao_estoura);
  RUN_TEST(test_largada_registra_a_primeira_amostra_da_corrida_com_tempo_zero);
  RUN_TEST(test_o_tempo_conta_da_largada_e_nao_do_inicio_da_espera);
  RUN_TEST(test_em_corrida_registra_uma_amostra_por_ciclo);
  RUN_TEST(test_tempo_decorrido_certo_com_a_volta_do_millis_depois_da_largada);
  RUN_TEST(test_objetivo_encerra_com_amostra_parada_e_a_fase_encerrada);
  RUN_TEST(test_o_fim_do_tempo_encerra_exatamente_nos_10_minutos);
  RUN_TEST(test_o_tempo_de_espera_nao_gasta_os_10_minutos);
  RUN_TEST(test_objetivo_no_mesmo_ciclo_da_largada_encerra_na_hora);
  RUN_TEST(test_reiniciar_em_corrida_volta_a_aguardar_a_largada_e_avisa_de_novo);
  RUN_TEST(test_reiniciar_nao_desencerra_nem_pula_a_calibracao);
  RUN_TEST(test_encerrada_so_depois_do_objetivo_ou_do_tempo_e_o_reinicio_nao_desfaz);
  RUN_TEST(test_sem_calibracao_a_fase_nao_esta_encerrada);
  RUN_TEST(test_integracao_o_servidor_recebe_primeiro_a_espera_e_depois_a_corrida);
  RUN_TEST(test_integracao_o_json_do_lote_leva_o_estado_aguardando_do_contrato);
  RUN_TEST(test_integracao_sem_rede_na_espera_nada_se_perde_e_o_estado_chega_depois);
  RUN_TEST(test_integracao_a_espera_de_5_minutos_gera_poucas_amostras);
  return UNITY_END();
}
