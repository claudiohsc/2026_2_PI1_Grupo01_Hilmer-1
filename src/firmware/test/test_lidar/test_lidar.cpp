/// @file test_lidar.cpp
/// @brief Testes unitários do driver do LiDAR: agregação por setor (mediana, filtros, montagem do
/// sensor) e `LidarUart` (bytes → leitura, falhas, leitura velha, limite de bytes). Usam um
/// protocolo inventado (`DecodificadorSimulado`), pois o decodificador do ST-L50B2 ainda não existe.
/// Roda no computador: `pio test -e native -f test_lidar`.

#include <unity.h>

#include "AgregadorSetores.h"
#include "DecodificadorSimulado.h"
#include "FonteBytesSimulada.h"
#include "LidarUart.h"

using namespace micromouse;

static uint32_t g_agoraMs = 0;
static uint32_t relogioFalso() { return g_agoraMs; }

void setUp() { g_agoraMs = 1000; }
void tearDown() {}

// ---------------------------------------------------------------------------------------------
// "Mundo" simulado: distância medida em cada ângulo do sensor (em graus), para a montagem padrão
// (deslocamento 0, ângulo crescendo no sentido horário): frente 100, direita 150, esquerda 270 e
// 500 nas demais direções. Os setores padrão têm ±15° e a volta é amostrada de 5 em 5 graus.
// ---------------------------------------------------------------------------------------------

using FuncaoDistancia = uint16_t (*)(uint16_t graus);

static bool naFrente(uint16_t g) { return g <= 15 || g >= 345; }
static bool naDireita(uint16_t g) { return g >= 75 && g <= 105; }
static bool naEsquerda(uint16_t g) { return g >= 255 && g <= 285; }

static uint16_t mundoPadrao(uint16_t g) {
  if (naFrente(g)) return 100;
  if (naDireita(g)) return 150;
  if (naEsquerda(g)) return 270;
  return 500;
}

/// Como o padrão, mas com dois pontos espúrios na frente (um muito perto, outro muito longe).
static uint16_t mundoComRuidoNaFrente(uint16_t g) {
  if (g == 5) return 40;
  if (g == 10) return 400;
  return mundoPadrao(g);
}

/// Frente sem nenhum retorno (distância 0 em todos os pontos).
static uint16_t mundoFrenteSemRetorno(uint16_t g) { return naFrente(g) ? 0 : mundoPadrao(g); }

/// Frente com só três pontos válidos; os demais não tiveram retorno.
static uint16_t mundoFrenteComTresRetornos(uint16_t g) {
  if (g == 0 || g == 5 || g == 10) return 100;
  return naFrente(g) ? 0 : mundoPadrao(g);
}

/// Frente com dois pontos válidos e um ponto acima da distância máxima.
static uint16_t mundoFrenteComDoisRetornos(uint16_t g) {
  if (g == 0 || g == 5) return 100;
  if (g == 10) return 65535;
  return naFrente(g) ? 0 : mundoPadrao(g);
}

/// Entrega uma volta completa (0° a 355°) ao agregador. Retorna `true` se algum ponto fechou
/// uma volta anterior.
static bool adicionarVolta(AgregadorSetores& agregador, FuncaoDistancia mundo) {
  bool fechou = false;
  for (uint16_t g = 0; g < 360; g += 5) {
    fechou |= agregador.adicionarPonto(PontoLidar{static_cast<uint16_t>(g * 100), mundo(g)});
  }
  return fechou;
}

/// Entrega duas voltas e o primeiro ponto da seguinte. A primeira volta é descartada pelo
/// agregador; o resultado é o da segunda. Retorna `voltaValida()`.
static bool agregarDuasVoltas(AgregadorSetores& agregador, FuncaoDistancia mundo) {
  adicionarVolta(agregador, mundo);
  adicionarVolta(agregador, mundo);
  agregador.adicionarPonto(PontoLidar{0, 500});
  return agregador.voltaValida();
}

static void verificar(const DistanciasLaterais& saida, uint16_t frente, uint16_t esquerda,
                      uint16_t direita) {
  TEST_ASSERT_EQUAL_UINT16(frente, saida.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(esquerda, saida.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(direita, saida.direitaMm);
}

// ------------------------------------------------------------------------- AgregadorSetores ---

void test_agregador_descarta_a_primeira_volta_e_usa_a_segunda() {
  AgregadorSetores agregador;
  adicionarVolta(agregador, mundoPadrao);  // começa no meio de uma volta
  TEST_ASSERT_FALSE(adicionarVolta(agregador, mundoPadrao));  // fecha a 1ª: descartada
  TEST_ASSERT_FALSE(agregador.voltaValida());
  TEST_ASSERT_TRUE(agregador.adicionarPonto(PontoLidar{0, 500}));  // fecha a 2ª
  TEST_ASSERT_TRUE(agregador.voltaValida());
}

void test_agregador_calcula_a_mediana_de_cada_setor() {
  AgregadorSetores agregador;
  TEST_ASSERT_TRUE(agregarDuasVoltas(agregador, mundoPadrao));
  verificar(agregador.distancias(), 100, 270, 150);
}

void test_agregador_mediana_ignora_pontos_espurios() {
  AgregadorSetores agregador;
  TEST_ASSERT_TRUE(agregarDuasVoltas(agregador, mundoComRuidoNaFrente));
  verificar(agregador.distancias(), 100, 270, 150);  // a menor distância daria 40
}

void test_agregador_ignora_pontos_sem_retorno_e_usa_os_validos() {
  AgregadorSetores agregador;
  TEST_ASSERT_TRUE(agregarDuasVoltas(agregador, mundoFrenteComTresRetornos));
  verificar(agregador.distancias(), 100, 270, 150);
}

void test_agregador_setor_sem_pontos_invalida_a_volta() {
  AgregadorSetores agregador;
  TEST_ASSERT_FALSE(agregarDuasVoltas(agregador, mundoFrenteSemRetorno));
}

void test_agregador_setor_com_poucos_pontos_validos_invalida_a_volta() {
  AgregadorSetores agregador;
  TEST_ASSERT_FALSE(agregarDuasVoltas(agregador, mundoFrenteComDoisRetornos));
}

void test_agregador_considera_o_deslocamento_da_frente() {
  ConfiguracaoLidar config;
  config.deslocamentoFrenteCentiGraus = 9000;  // a frente do robô é o ângulo 90° do sensor
  AgregadorSetores agregador(config);
  TEST_ASSERT_TRUE(agregarDuasVoltas(agregador, mundoPadrao));
  verificar(agregador.distancias(), 150, 100, 500);
}

void test_agregador_sentido_anti_horario_troca_esquerda_e_direita() {
  ConfiguracaoLidar config;
  config.anguloCrescenteHorario = false;
  AgregadorSetores agregador(config);
  TEST_ASSERT_TRUE(agregarDuasVoltas(agregador, mundoPadrao));
  verificar(agregador.distancias(), 100, 150, 270);
}

void test_agregador_aguenta_mais_pontos_que_a_capacidade_do_setor() {
  AgregadorSetores agregador;
  // Resolução de 0,1°: a frente tem 300 pontos por volta, mais que a capacidade (256).
  for (int volta = 0; volta < 2; volta++) {
    for (uint16_t c = 0; c < 36000; c += 10) {
      agregador.adicionarPonto(PontoLidar{c, mundoPadrao(c / 100)});
    }
  }
  agregador.adicionarPonto(PontoLidar{0, 500});
  TEST_ASSERT_TRUE(agregador.voltaValida());
  verificar(agregador.distancias(), 100, 270, 150);
}

void test_agregador_reiniciar_esquece_a_volta_anterior() {
  AgregadorSetores agregador;
  TEST_ASSERT_TRUE(agregarDuasVoltas(agregador, mundoPadrao));
  agregador.reiniciar();
  TEST_ASSERT_FALSE(agregador.voltaValida());
}

// ------------------------------------------------------------------------------- LidarUart ---

static void enfileirarPonto(FonteBytesSimulada& fonte, uint16_t anguloCentiGraus,
                            uint16_t distanciaMm, bool corromper = false) {
  uint8_t pacote[DecodificadorSimulado::TAMANHO_PACOTE];
  DecodificadorSimulado::montarPacote(anguloCentiGraus, distanciaMm, pacote);
  if (corromper) {
    pacote[5] ^= 0xFF;  // verificação errada
  }
  TEST_ASSERT_TRUE(fonte.enfileirar(pacote, sizeof(pacote)));
}

/// Enfileira os bytes de uma volta completa. Se `grauCorrompido` for um ângulo da volta, o
/// pacote desse ângulo sai com erro de verificação.
static void enfileirarVolta(FonteBytesSimulada& fonte, FuncaoDistancia mundo,
                            int grauCorrompido = -1) {
  for (uint16_t g = 0; g < 360; g += 5) {
    enfileirarPonto(fonte, static_cast<uint16_t>(g * 100), mundo(g), g == grauCorrompido);
  }
}

/// Enfileira duas voltas e o primeiro ponto da seguinte (a primeira volta é descartada).
static void enfileirarDuasVoltas(FonteBytesSimulada& fonte, FuncaoDistancia mundo) {
  enfileirarVolta(fonte, mundo);
  enfileirarVolta(fonte, mundo);
  enfileirarPonto(fonte, 0, 500);
}

void test_uart_sem_bytes_a_leitura_falha_sem_travar() {
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart lidar(fonte, decodificador, relogioFalso);
  DistanciasLaterais saida = {};
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));
}

void test_uart_bytes_de_uma_volta_geram_a_leitura() {
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart lidar(fonte, decodificador, relogioFalso);
  enfileirarDuasVoltas(fonte, mundoPadrao);
  DistanciasLaterais saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  verificar(saida, 100, 270, 150);
}

void test_uart_volta_incompleta_ainda_nao_gera_leitura() {
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart lidar(fonte, decodificador, relogioFalso);
  enfileirarVolta(fonte, mundoPadrao);  // só uma volta, sem fechar a segunda
  DistanciasLaterais saida = {};
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));
}

void test_uart_repete_a_mesma_leitura_enquanto_ela_e_recente() {
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart lidar(fonte, decodificador, relogioFalso);
  enfileirarDuasVoltas(fonte, mundoPadrao);
  DistanciasLaterais saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  g_agoraMs += 10;  // o laço de navegação roda a cada 10 ms; o sensor ainda não fechou outra volta
  saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  verificar(saida, 100, 270, 150);
}

void test_uart_leitura_velha_falha() {
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart lidar(fonte, decodificador, relogioFalso);
  enfileirarDuasVoltas(fonte, mundoPadrao);
  DistanciasLaterais saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  g_agoraMs += IDADE_MAXIMA_LEITURA_MS;  // no limite: ainda vale
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  g_agoraMs += 1;  // passou do limite: sensor parado
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));
}

void test_uart_volta_com_setor_sem_pontos_validos_falha() {
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart lidar(fonte, decodificador, relogioFalso);
  enfileirarDuasVoltas(fonte, mundoFrenteSemRetorno);
  DistanciasLaterais saida = {};
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));
}

void test_uart_bytes_de_lixo_nao_geram_leitura_nem_travam() {
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart lidar(fonte, decodificador, relogioFalso);
  const uint8_t lixo[] = {0xAA, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x13, 0x00, 0xAA, 0xAA};
  for (int i = 0; i < 40; i++) {
    TEST_ASSERT_TRUE(fonte.enfileirar(lixo, sizeof(lixo)));
  }
  DistanciasLaterais saida = {};
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(0, fonte.quantidadeDisponivel());  // consumiu tudo e voltou
}

void test_uart_pacote_corrompido_e_ignorado_e_a_leitura_continua_valida() {
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart lidar(fonte, decodificador, relogioFalso);
  enfileirarVolta(fonte, mundoPadrao);
  enfileirarVolta(fonte, mundoPadrao, /*grauCorrompido=*/5);  // 1 dos 7 pontos da frente se perde
  enfileirarPonto(fonte, 0, 500);
  DistanciasLaterais saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  verificar(saida, 100, 270, 150);
}

void test_uart_limite_de_bytes_por_chamada() {
  ConfiguracaoLidar config;
  config.maxBytesPorChamada = 12;  // dois pacotes por chamada
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart lidar(fonte, decodificador, relogioFalso, config);
  enfileirarDuasVoltas(fonte, mundoPadrao);
  const uint16_t total = fonte.quantidadeDisponivel();
  DistanciasLaterais saida = {};
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(total - 12, fonte.quantidadeDisponivel());  // só leu 12 bytes
  int chamadas = 1;
  while (!lidar.lerDistanciasLaterais(saida) && chamadas < 200) {
    chamadas++;
  }
  TEST_ASSERT_TRUE(chamadas < 200);
  verificar(saida, 100, 270, 150);
}

void test_uart_volta_a_responder_depois_de_ficar_mudo() {
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart lidar(fonte, decodificador, relogioFalso);
  enfileirarDuasVoltas(fonte, mundoPadrao);
  DistanciasLaterais saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  g_agoraMs += 1000;  // sensor mudo por 1 s
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));
  enfileirarVolta(fonte, mundoPadrao);  // o sensor volta a transmitir
  enfileirarPonto(fonte, 0, 500);
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  verificar(saida, 100, 270, 150);
}

void test_uart_sem_relogio_nao_verifica_a_idade() {
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart lidar(fonte, decodificador, nullptr);
  enfileirarDuasVoltas(fonte, mundoPadrao);
  DistanciasLaterais saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
}

void test_uart_pode_ser_usado_pela_interface_ILidar() {
  FonteBytesSimulada fonte;
  DecodificadorSimulado decodificador;
  LidarUart uart(fonte, decodificador, relogioFalso);
  enfileirarDuasVoltas(fonte, mundoPadrao);
  ILidar& lidar = uart;
  DistanciasLaterais saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  verificar(saida, 100, 270, 150);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_agregador_descarta_a_primeira_volta_e_usa_a_segunda);
  RUN_TEST(test_agregador_calcula_a_mediana_de_cada_setor);
  RUN_TEST(test_agregador_mediana_ignora_pontos_espurios);
  RUN_TEST(test_agregador_ignora_pontos_sem_retorno_e_usa_os_validos);
  RUN_TEST(test_agregador_setor_sem_pontos_invalida_a_volta);
  RUN_TEST(test_agregador_setor_com_poucos_pontos_validos_invalida_a_volta);
  RUN_TEST(test_agregador_considera_o_deslocamento_da_frente);
  RUN_TEST(test_agregador_sentido_anti_horario_troca_esquerda_e_direita);
  RUN_TEST(test_agregador_aguenta_mais_pontos_que_a_capacidade_do_setor);
  RUN_TEST(test_agregador_reiniciar_esquece_a_volta_anterior);
  RUN_TEST(test_uart_sem_bytes_a_leitura_falha_sem_travar);
  RUN_TEST(test_uart_bytes_de_uma_volta_geram_a_leitura);
  RUN_TEST(test_uart_volta_incompleta_ainda_nao_gera_leitura);
  RUN_TEST(test_uart_repete_a_mesma_leitura_enquanto_ela_e_recente);
  RUN_TEST(test_uart_leitura_velha_falha);
  RUN_TEST(test_uart_volta_com_setor_sem_pontos_validos_falha);
  RUN_TEST(test_uart_bytes_de_lixo_nao_geram_leitura_nem_travam);
  RUN_TEST(test_uart_pacote_corrompido_e_ignorado_e_a_leitura_continua_valida);
  RUN_TEST(test_uart_limite_de_bytes_por_chamada);
  RUN_TEST(test_uart_volta_a_responder_depois_de_ficar_mudo);
  RUN_TEST(test_uart_sem_relogio_nao_verifica_a_idade);
  RUN_TEST(test_uart_pode_ser_usado_pela_interface_ILidar);
  return UNITY_END();
}
