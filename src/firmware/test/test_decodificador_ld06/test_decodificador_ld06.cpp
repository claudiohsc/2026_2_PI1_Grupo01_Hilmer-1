/// @file test_decodificador_ld06.cpp
/// @brief Testes do decodificador do protocolo LD06: pacote de exemplo do manual, CRC, bytes de
/// lixo, ressincronização, pacote que cruza 0° e o driver completo (`LidarUart`) lendo pacotes
/// reais. Roda no computador: `pio test -e native -f test_decodificador_ld06`.

#include <unity.h>

#include <string.h>

#include "DecodificadorLd06.h"
#include "FonteBytesSimulada.h"
#include "LidarUart.h"

using namespace micromouse;

static uint32_t g_agoraMs = 0;
static uint32_t relogioFalso() { return g_agoraMs; }

void setUp() { g_agoraMs = 1000; }
void tearDown() {}

/// Pacote de exemplo do manual do LD06 (47 bytes): 12 pontos de 224 a 192 mm, de 324,27° a 334,70°.
static const uint8_t PACOTE_DO_MANUAL[DecodificadorLd06::TAMANHO_PACOTE] = {
    0x54, 0x2C, 0x68, 0x08, 0xAB, 0x7E, 0xE0, 0x00, 0xE4, 0xDC, 0x00, 0xE2, 0xD9, 0x00, 0xE5, 0xD5,
    0x00, 0xE3, 0xD3, 0x00, 0xE4, 0xD0, 0x00, 0xE9, 0xCD, 0x00, 0xE4, 0xCA, 0x00, 0xE2, 0xC7, 0x00,
    0xE9, 0xC5, 0x00, 0xE5, 0xC2, 0x00, 0xE5, 0xC0, 0x00, 0xE5, 0xBE, 0x82, 0x3A, 0x1A, 0x50};

/// Monta um pacote LD06 válido (com CRC) com os ângulos e distâncias indicados.
static void montarPacote(uint16_t inicioCentiGraus, uint16_t fimCentiGraus,
                         const uint16_t (&distanciasMm)[12],
                         uint8_t (&pacote)[DecodificadorLd06::TAMANHO_PACOTE]) {
  memset(pacote, 0, sizeof(pacote));
  pacote[0] = DecodificadorLd06::CABECALHO;
  pacote[1] = DecodificadorLd06::TIPO_E_QUANTIDADE;
  pacote[2] = 0x68;  // 2152 graus/s, como no manual
  pacote[3] = 0x08;
  pacote[4] = inicioCentiGraus & 0xFF;
  pacote[5] = inicioCentiGraus >> 8;
  for (int i = 0; i < 12; i++) {
    pacote[6 + 3 * i] = distanciasMm[i] & 0xFF;
    pacote[7 + 3 * i] = distanciasMm[i] >> 8;
    pacote[8 + 3 * i] = 200;  // intensidade
  }
  pacote[42] = fimCentiGraus & 0xFF;
  pacote[43] = fimCentiGraus >> 8;
  pacote[44] = 0x3A;
  pacote[45] = 0x1A;
  pacote[46] = DecodificadorLd06::calcularCrc8(pacote, 46);
}

/// Entrega os bytes ao decodificador e conta quantos pacotes válidos foram completados.
static int alimentarTudo(DecodificadorLd06& decodificador, const uint8_t* bytes, size_t quantidade) {
  int pacotes = 0;
  for (size_t i = 0; i < quantidade; i++) {
    if (decodificador.alimentar(bytes[i])) {
      pacotes++;
    }
  }
  return pacotes;
}

void test_crc8_confere_com_o_exemplo_do_manual() {
  TEST_ASSERT_EQUAL_UINT8(0x50, DecodificadorLd06::calcularCrc8(PACOTE_DO_MANUAL, 46));
  TEST_ASSERT_EQUAL_UINT8(0x00, DecodificadorLd06::calcularCrc8(PACOTE_DO_MANUAL, 47));
}

void test_decodifica_o_pacote_do_manual() {
  DecodificadorLd06 decodificador;
  for (int i = 0; i < 46; i++) {
    TEST_ASSERT_FALSE(decodificador.alimentar(PACOTE_DO_MANUAL[i]));
  }
  TEST_ASSERT_TRUE(decodificador.alimentar(PACOTE_DO_MANUAL[46]));
  TEST_ASSERT_EQUAL_UINT8(12, decodificador.quantidadePontos());

  const uint16_t distancias[12] = {224, 220, 217, 213, 211, 208, 205, 202, 199, 197, 194, 192};
  const uint16_t angulos[12] = {32427, 32521, 32616, 32711, 32806, 32901,
                                32995, 33090, 33185, 33280, 33375, 33470};
  for (uint8_t i = 0; i < 12; i++) {
    TEST_ASSERT_EQUAL_UINT16(distancias[i], decodificador.ponto(i).distanciaMm);
    TEST_ASSERT_EQUAL_UINT16(angulos[i], decodificador.ponto(i).anguloCentiGraus);
  }
}

void test_pacote_com_byte_alterado_e_descartado() {
  uint8_t pacote[DecodificadorLd06::TAMANHO_PACOTE];
  memcpy(pacote, PACOTE_DO_MANUAL, sizeof(pacote));
  pacote[10] ^= 0x01;  // um bit errado na distância
  DecodificadorLd06 decodificador;
  TEST_ASSERT_EQUAL_INT(0, alimentarTudo(decodificador, pacote, sizeof(pacote)));
}

void test_ignora_bytes_de_lixo_antes_do_pacote() {
  const uint8_t lixo[] = {0x00, 0x54, 0x00, 0x54, 0x54, 0x2C, 0x10, 0xFF, 0x54, 0x13};
  DecodificadorLd06 decodificador;
  TEST_ASSERT_EQUAL_INT(0, alimentarTudo(decodificador, lixo, sizeof(lixo)));
  TEST_ASSERT_EQUAL_INT(1, alimentarTudo(decodificador, PACOTE_DO_MANUAL, sizeof(PACOTE_DO_MANUAL)));
  TEST_ASSERT_EQUAL_UINT16(224, decodificador.ponto(0).distanciaMm);
}

void test_decodifica_pacotes_em_sequencia() {
  uint8_t fluxo[3 * DecodificadorLd06::TAMANHO_PACOTE];
  for (int p = 0; p < 3; p++) {
    memcpy(&fluxo[p * DecodificadorLd06::TAMANHO_PACOTE], PACOTE_DO_MANUAL,
           DecodificadorLd06::TAMANHO_PACOTE);
  }
  DecodificadorLd06 decodificador;
  TEST_ASSERT_EQUAL_INT(3, alimentarTudo(decodificador, fluxo, sizeof(fluxo)));
}

void test_recupera_o_pacote_valido_depois_de_um_pacote_corrompido() {
  uint8_t ruim[DecodificadorLd06::TAMANHO_PACOTE];
  memcpy(ruim, PACOTE_DO_MANUAL, sizeof(ruim));
  ruim[46] ^= 0xFF;  // CRC errado
  DecodificadorLd06 decodificador;
  TEST_ASSERT_EQUAL_INT(0, alimentarTudo(decodificador, ruim, sizeof(ruim)));
  TEST_ASSERT_EQUAL_INT(1, alimentarTudo(decodificador, PACOTE_DO_MANUAL, sizeof(PACOTE_DO_MANUAL)));
}

void test_recupera_o_pacote_seguinte_a_um_pacote_truncado() {
  DecodificadorLd06 decodificador;
  // O sensor "engasga": só 20 bytes do primeiro pacote chegam, e logo vem o pacote seguinte inteiro.
  TEST_ASSERT_EQUAL_INT(0, alimentarTudo(decodificador, PACOTE_DO_MANUAL, 20));
  TEST_ASSERT_EQUAL_INT(1, alimentarTudo(decodificador, PACOTE_DO_MANUAL, sizeof(PACOTE_DO_MANUAL)));
  TEST_ASSERT_EQUAL_UINT16(224, decodificador.ponto(0).distanciaMm);
  TEST_ASSERT_EQUAL_UINT16(192, decodificador.ponto(11).distanciaMm);
}

void test_pacote_que_cruza_o_zero_grau() {
  const uint16_t distancias[12] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
  uint8_t pacote[DecodificadorLd06::TAMANHO_PACOTE];
  montarPacote(35500, 300, distancias, pacote);  // de 355° a 3° passando pelo 0°
  DecodificadorLd06 decodificador;
  TEST_ASSERT_EQUAL_INT(1, alimentarTudo(decodificador, pacote, sizeof(pacote)));
  TEST_ASSERT_EQUAL_UINT16(35500, decodificador.ponto(0).anguloCentiGraus);
  TEST_ASSERT_EQUAL_UINT16(300, decodificador.ponto(11).anguloCentiGraus);
  for (uint8_t i = 1; i < 12; i++) {
    TEST_ASSERT_TRUE(decodificador.ponto(i).anguloCentiGraus < 36000);
  }
  TEST_ASSERT_EQUAL_UINT16(35500 + 800 * 4 / 11, decodificador.ponto(4).anguloCentiGraus);
}

void test_pacote_com_angulo_invalido_e_descartado() {
  const uint16_t distancias[12] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
  uint8_t pacote[DecodificadorLd06::TAMANHO_PACOTE];
  montarPacote(40000, 100, distancias, pacote);  // início acima de 359,99°, mas com CRC válido
  DecodificadorLd06 decodificador;
  TEST_ASSERT_EQUAL_INT(0, alimentarTudo(decodificador, pacote, sizeof(pacote)));
}

void test_indice_de_ponto_fora_do_pacote_devolve_ponto_vazio() {
  DecodificadorLd06 decodificador;
  alimentarTudo(decodificador, PACOTE_DO_MANUAL, sizeof(PACOTE_DO_MANUAL));
  TEST_ASSERT_EQUAL_UINT16(0, decodificador.ponto(12).distanciaMm);
}

// ------------------------------------------------- driver completo com pacotes no formato real ---

/// Distância "do mundo" em graus do sensor (montagem padrão): frente 100, direita 150, esquerda 270, resto 500.
static uint16_t mundo(uint16_t g) {
  if (g <= 15 || g >= 345) return 100;
  if (g >= 75 && g <= 105) return 150;
  if (g >= 255 && g <= 285) return 270;
  return 500;
}

/// Enfileira uma volta completa de pacotes LD06: um pacote a cada 10°, cobrindo 9° cada.
static void enfileirarVolta(FonteBytesSimulada& fonte) {
  for (uint16_t inicio = 0; inicio < 36000; inicio += 1000) {
    const uint16_t fim = inicio + 900;
    uint16_t distancias[12];
    for (int i = 0; i < 12; i++) {
      const uint32_t angulo = (inicio + 900u * i / 11) % 36000;
      distancias[i] = mundo(static_cast<uint16_t>(angulo / 100));
    }
    uint8_t pacote[DecodificadorLd06::TAMANHO_PACOTE];
    montarPacote(inicio, fim, distancias, pacote);
    TEST_ASSERT_TRUE(fonte.enfileirar(pacote, sizeof(pacote)));
  }
}

void test_driver_completo_le_pacotes_ld06() {
  FonteBytesSimulada fonte;
  DecodificadorLd06 decodificador;
  LidarUart lidar(fonte, decodificador, relogioFalso);
  DistanciasLaterais saida = {};
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));  // sensor ainda mudo

  enfileirarVolta(fonte);  // a 1ª volta começa "no meio" e é descartada
  enfileirarVolta(fonte);
  const uint16_t umaDistancia[12] = {500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500};
  uint8_t proximo[DecodificadorLd06::TAMANHO_PACOTE];
  montarPacote(0, 900, umaDistancia, proximo);  // início da volta seguinte fecha a anterior
  TEST_ASSERT_TRUE(fonte.enfileirar(proximo, sizeof(proximo)));

  // O driver lê no máximo `MAX_BYTES_POR_CHAMADA` por chamada; o laço de navegação chama a cada 10 ms.
  bool leu = false;
  for (int chamada = 0; chamada < 10 && !leu; chamada++) {
    leu = lidar.lerDistanciasLaterais(saida);
  }
  TEST_ASSERT_TRUE(leu);
  TEST_ASSERT_EQUAL_UINT16(100, saida.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(270, saida.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(150, saida.direitaMm);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_crc8_confere_com_o_exemplo_do_manual);
  RUN_TEST(test_decodifica_o_pacote_do_manual);
  RUN_TEST(test_pacote_com_byte_alterado_e_descartado);
  RUN_TEST(test_ignora_bytes_de_lixo_antes_do_pacote);
  RUN_TEST(test_decodifica_pacotes_em_sequencia);
  RUN_TEST(test_recupera_o_pacote_valido_depois_de_um_pacote_corrompido);
  RUN_TEST(test_recupera_o_pacote_seguinte_a_um_pacote_truncado);
  RUN_TEST(test_pacote_que_cruza_o_zero_grau);
  RUN_TEST(test_pacote_com_angulo_invalido_e_descartado);
  RUN_TEST(test_indice_de_ponto_fora_do_pacote_devolve_ponto_vazio);
  RUN_TEST(test_driver_completo_le_pacotes_ld06);
  return UNITY_END();
}
