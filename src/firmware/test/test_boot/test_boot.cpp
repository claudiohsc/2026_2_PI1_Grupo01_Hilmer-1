/// @file test_boot.cpp
/// @brief Testes do autoteste do boot (#50): varredura I²C com dispositivos simulados, relatório e
/// trava dos motores.
/// Roda no computador: `pio test -e native -f test_boot`.

#include <ArduinoJson.h>
#include <string.h>
#include <unity.h>

#include "AutotesteBoot.h"
#include "BarramentoI2cSimulado.h"
#include "MotoresComTrava.h"
#include "MotoresSimulados.h"
#include "Pinos.h"
#include "RelatorioBoot.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr InicializacaoTof TODOS_INICIADOS = {true, true, true};

/// Barramento da Rev 2.0 montada e funcionando: os 3 ToF nos endereços novos e o MPU6050 original.
BarramentoI2cSimulado barramentoCompleto() {
  BarramentoI2cSimulado barramento;
  barramento.conectar(pinos::ENDERECO_TOF_ESQ);
  barramento.conectar(pinos::ENDERECO_TOF_FRONTAL);
  barramento.conectar(pinos::ENDERECO_TOF_DIR);
  barramento.conectar(pinos::ENDERECO_MPU6050);
  barramento.definirRegistrador(pinos::ENDERECO_MPU6050, REGISTRADOR_WHO_AM_I_MPU6050, 0x68);
  return barramento;
}

const uint8_t ENDERECO_DE[QUANTIDADE_DISPOSITIVOS] = {pinos::ENDERECO_TOF_ESQ, pinos::ENDERECO_TOF_FRONTAL,
                                                     pinos::ENDERECO_TOF_DIR, pinos::ENDERECO_MPU6050};

/// Falhas do resultado como texto, para comparar com `TEST_ASSERT_EQUAL_STRING`.
const char* falhas(const ResultadoBoot& resultado) {
  static char texto[TAMANHO_MAX_FALHAS_BOOT];
  AutotesteBoot::descreverFalhas(resultado, texto, sizeof(texto));
  return texto;
}

}  // namespace

// --- Varredura e conferência dos 4 dispositivos ---

void test_os_quatro_presentes_aprovam_o_boot() {
  BarramentoI2cSimulado barramento = barramentoCompleto();
  const ResultadoBoot resultado = AutotesteBoot().verificar(barramento, TODOS_INICIADOS);

  TEST_ASSERT_TRUE(resultado.aprovado());
  for (uint8_t i = 0; i < QUANTIDADE_DISPOSITIVOS; i++) {
    TEST_ASSERT_EQUAL(EstadoDispositivo::Ok, resultado.dispositivos[i]);
  }
  TEST_ASSERT_FALSE(resultado.tofNoEnderecoDeFabrica);
  TEST_ASSERT_TRUE(resultado.whoAmILido);
  TEST_ASSERT_EQUAL_HEX8(0x68, resultado.whoAmIMpu6050);
  TEST_ASSERT_EQUAL_UINT8(0, resultado.quantidadeInesperados);
  TEST_ASSERT_EQUAL_STRING("", falhas(resultado));
}

void test_a_varredura_cobre_0x08_a_0x77() {
  BarramentoI2cSimulado barramento = barramentoCompleto();
  AutotesteBoot().verificar(barramento, TODOS_INICIADOS);
  TEST_ASSERT_EQUAL_UINT32(0x77 - 0x08 + 1, barramento.consultas());
}

void test_cada_dispositivo_ausente_reprova_e_e_nomeado() {
  const char* nomes[QUANTIDADE_DISPOSITIVOS] = {"tof_esquerdo", "tof_frontal", "tof_direito", "mpu6050"};
  for (uint8_t i = 0; i < QUANTIDADE_DISPOSITIVOS; i++) {
    BarramentoI2cSimulado barramento = barramentoCompleto();
    barramento.desconectar(ENDERECO_DE[i]);
    const InicializacaoTof iniciados = {i != 0, i != 1, i != 2};

    const ResultadoBoot resultado = AutotesteBoot().verificar(barramento, iniciados);

    TEST_ASSERT_FALSE_MESSAGE(resultado.aprovado(), nomes[i]);
    TEST_ASSERT_EQUAL_MESSAGE(EstadoDispositivo::Ausente, resultado.dispositivos[i], nomes[i]);
    for (uint8_t j = 0; j < QUANTIDADE_DISPOSITIVOS; j++) {
      if (j != i) {
        TEST_ASSERT_EQUAL_MESSAGE(EstadoDispositivo::Ok, resultado.dispositivos[j], nomes[i]);
      }
    }
    TEST_ASSERT_EQUAL_STRING(nomes[i], falhas(resultado));
    TEST_ASSERT_EQUAL_STRING(nomes[i], nomeDispositivo(static_cast<Dispositivo>(i)));
  }
}

void test_tof_que_responde_mas_nao_iniciou_reprova() {
  BarramentoI2cSimulado barramento = barramentoCompleto();
  const ResultadoBoot resultado = AutotesteBoot().verificar(barramento, {true, false, true});

  TEST_ASSERT_FALSE(resultado.aprovado());
  TEST_ASSERT_EQUAL(EstadoDispositivo::NaoIniciou, resultado.estado(Dispositivo::TofFrontal));
  TEST_ASSERT_EQUAL_STRING("tof_frontal", falhas(resultado));
}

void test_tof_iniciado_pelo_driver_mas_sem_resposta_fica_ausente() {
  // O driver diz que iniciou, mas o sensor sumiu do barramento antes da varredura.
  BarramentoI2cSimulado barramento = barramentoCompleto();
  barramento.desconectar(pinos::ENDERECO_TOF_DIR);
  const ResultadoBoot resultado = AutotesteBoot().verificar(barramento, TODOS_INICIADOS);
  TEST_ASSERT_EQUAL(EstadoDispositivo::Ausente, resultado.estado(Dispositivo::TofDireito));
  TEST_ASSERT_FALSE(resultado.aprovado());
}

void test_todos_ausentes_listam_todas_as_falhas_em_ordem() {
  BarramentoI2cSimulado vazio;
  const ResultadoBoot resultado = AutotesteBoot().verificar(vazio, {false, false, false});

  TEST_ASSERT_FALSE(resultado.aprovado());
  TEST_ASSERT_FALSE(resultado.whoAmILido);
  TEST_ASSERT_EQUAL_STRING("tof_esquerdo,tof_frontal,tof_direito,mpu6050", falhas(resultado));
}

// --- MPU6050: WHO_AM_I ---

void test_outro_chip_no_0x68_nao_passa_por_mpu6050() {
  BarramentoI2cSimulado barramento = barramentoCompleto();
  barramento.definirRegistrador(pinos::ENDERECO_MPU6050, REGISTRADOR_WHO_AM_I_MPU6050, 0x70);

  const ResultadoBoot resultado = AutotesteBoot().verificar(barramento, TODOS_INICIADOS);

  TEST_ASSERT_FALSE(resultado.aprovado());
  TEST_ASSERT_EQUAL(EstadoDispositivo::NaoIdentificado, resultado.estado(Dispositivo::Mpu6050));
  TEST_ASSERT_TRUE(resultado.whoAmILido);
  TEST_ASSERT_EQUAL_HEX8(0x70, resultado.whoAmIMpu6050);
  TEST_ASSERT_EQUAL_STRING("mpu6050", falhas(resultado));
}

void test_who_am_i_que_nao_le_reprova() {
  BarramentoI2cSimulado barramento = barramentoCompleto();
  barramento.falharLeituras(true);

  const ResultadoBoot resultado = AutotesteBoot().verificar(barramento, TODOS_INICIADOS);

  TEST_ASSERT_FALSE(resultado.aprovado());
  TEST_ASSERT_EQUAL(EstadoDispositivo::NaoIdentificado, resultado.estado(Dispositivo::Mpu6050));
  TEST_ASSERT_FALSE(resultado.whoAmILido);
}

void test_who_am_i_aceito_vem_da_configuracao() {
  // Clone confirmado pela eletrônica: basta trocar o valor em ConfiguracaoBoot.h.
  BarramentoI2cSimulado barramento = barramentoCompleto();
  barramento.definirRegistrador(pinos::ENDERECO_MPU6050, REGISTRADOR_WHO_AM_I_MPU6050, 0x70);
  ConfiguracaoBoot config;
  config.whoAmIMpu6050 = 0x70;

  TEST_ASSERT_TRUE(AutotesteBoot(config).verificar(barramento, TODOS_INICIADOS).aprovado());
}

// --- Endereços que sobram no barramento ---

void test_tof_no_endereco_de_fabrica_reprova() {
  // Um quarto ToF (ou um XSHUT trocado) deixa alguém no 0x29 mesmo com os três esperados presentes.
  BarramentoI2cSimulado barramento = barramentoCompleto();
  barramento.conectar(pinos::ENDERECO_FABRICA_TOF);

  const ResultadoBoot resultado = AutotesteBoot().verificar(barramento, TODOS_INICIADOS);

  TEST_ASSERT_FALSE(resultado.aprovado());
  TEST_ASSERT_TRUE(resultado.tofNoEnderecoDeFabrica);
  TEST_ASSERT_EQUAL_UINT8(0, resultado.quantidadeInesperados);  // 0x29 tem falha própria, não é aviso
  TEST_ASSERT_EQUAL_STRING("tof_0x29", falhas(resultado));
}

void test_endereco_inesperado_so_avisa() {
  BarramentoI2cSimulado barramento = barramentoCompleto();
  barramento.conectar(0x3C);  // um display, por exemplo

  const ResultadoBoot resultado = AutotesteBoot().verificar(barramento, TODOS_INICIADOS);

  TEST_ASSERT_TRUE(resultado.aprovado());
  TEST_ASSERT_EQUAL_UINT8(1, resultado.quantidadeInesperados);
  TEST_ASSERT_EQUAL_HEX8(0x3C, resultado.inesperados[0]);
}

void test_inesperados_alem_da_capacidade_so_entram_na_contagem() {
  BarramentoI2cSimulado barramento = barramentoCompleto();
  for (uint8_t endereco = 0x40; endereco < 0x40 + MAXIMO_ENDERECOS_INESPERADOS + 3; endereco++) {
    barramento.conectar(endereco);
  }

  const ResultadoBoot resultado = AutotesteBoot().verificar(barramento, TODOS_INICIADOS);

  TEST_ASSERT_TRUE(resultado.aprovado());
  TEST_ASSERT_EQUAL_UINT8(MAXIMO_ENDERECOS_INESPERADOS + 3, resultado.quantidadeInesperados);
  TEST_ASSERT_EQUAL_HEX8(0x40, resultado.inesperados[0]);
  TEST_ASSERT_EQUAL_HEX8(0x40 + MAXIMO_ENDERECOS_INESPERADOS - 1,
                         resultado.inesperados[MAXIMO_ENDERECOS_INESPERADOS - 1]);
}

void test_enderecos_fora_da_faixa_varrida_sao_ignorados() {
  BarramentoI2cSimulado barramento = barramentoCompleto();
  barramento.conectar(0x00);  // chamada geral
  barramento.conectar(0x7F);  // reservado

  const ResultadoBoot resultado = AutotesteBoot().verificar(barramento, TODOS_INICIADOS);
  TEST_ASSERT_TRUE(resultado.aprovado());
  TEST_ASSERT_EQUAL_UINT8(0, resultado.quantidadeInesperados);
}

// --- Descrição das falhas ---

void test_pior_caso_das_falhas_cabe_em_tamanho_max_falhas_boot() {
  BarramentoI2cSimulado barramento;
  barramento.conectar(pinos::ENDERECO_FABRICA_TOF);
  const ResultadoBoot resultado = AutotesteBoot().verificar(barramento, {false, false, false});

  char texto[TAMANHO_MAX_FALHAS_BOOT];
  const size_t bytes = AutotesteBoot::descreverFalhas(resultado, texto, sizeof(texto));
  TEST_ASSERT_EQUAL_STRING("tof_esquerdo,tof_frontal,tof_direito,mpu6050,tof_0x29", texto);
  TEST_ASSERT_EQUAL_UINT32(strlen(texto), bytes);
}

void test_falhas_que_nao_cabem_devolvem_texto_vazio() {
  BarramentoI2cSimulado vazio;
  const ResultadoBoot resultado = AutotesteBoot().verificar(vazio, {false, false, false});

  char pequeno[20];
  TEST_ASSERT_EQUAL_UINT32(0, AutotesteBoot::descreverFalhas(resultado, pequeno, sizeof(pequeno)));
  TEST_ASSERT_EQUAL_STRING("", pequeno);
  TEST_ASSERT_EQUAL_UINT32(0, AutotesteBoot::descreverFalhas(resultado, nullptr, 10));

  // Cabe só o primeiro nome: o 0x29 sozinho também precisa caber inteiro.
  BarramentoI2cSimulado soFabrica = barramentoCompleto();
  soFabrica.conectar(pinos::ENDERECO_FABRICA_TOF);
  const ResultadoBoot comFabrica = AutotesteBoot().verificar(soFabrica, TODOS_INICIADOS);
  char curto[5];
  TEST_ASSERT_EQUAL_UINT32(0, AutotesteBoot::descreverFalhas(comFabrica, curto, sizeof(curto)));
  TEST_ASSERT_EQUAL_STRING("", curto);
}

void test_nomes_dos_estados() {
  TEST_ASSERT_EQUAL_STRING("ok", nomeEstado(EstadoDispositivo::Ok));
  TEST_ASSERT_EQUAL_STRING("ausente", nomeEstado(EstadoDispositivo::Ausente));
  TEST_ASSERT_EQUAL_STRING("nao_iniciou", nomeEstado(EstadoDispositivo::NaoIniciou));
  TEST_ASSERT_EQUAL_STRING("nao_identificado", nomeEstado(EstadoDispositivo::NaoIdentificado));
  TEST_ASSERT_EQUAL_STRING("?", nomeEstado(static_cast<EstadoDispositivo>(99)));
  TEST_ASSERT_EQUAL_STRING("?", nomeDispositivo(static_cast<Dispositivo>(99)));
}

// --- Prazo do RNF-09 ---

void test_prazo_de_2_s_do_rnf_09() {
  const AutotesteBoot autoteste;
  TEST_ASSERT_EQUAL_UINT32(2000, autoteste.tempoMaximoMs());
  TEST_ASSERT_TRUE(autoteste.dentroDoPrazo(0));
  TEST_ASSERT_TRUE(autoteste.dentroDoPrazo(2000));
  TEST_ASSERT_FALSE(autoteste.dentroDoPrazo(2001));

  ConfiguracaoBoot config;
  config.tempoMaximoMs = 500;
  TEST_ASSERT_FALSE(AutotesteBoot(config).dentroDoPrazo(501));
}

// --- Relatório JSON ---

void test_relatorio_do_boot_aprovado() {
  BarramentoI2cSimulado barramento = barramentoCompleto();
  const AutotesteBoot autoteste;
  const ResultadoBoot resultado = autoteste.verificar(barramento, TODOS_INICIADOS);

  char buffer[TAMANHO_MAX_JSON_RELATORIO_BOOT];
  TEST_ASSERT_GREATER_THAN_UINT32(0, RelatorioBoot::serializar(resultado, autoteste, 412, buffer, sizeof(buffer)));

  JsonDocument doc;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, buffer).code());
  TEST_ASSERT_EQUAL_STRING("boot", doc["tipo"].as<const char*>());
  TEST_ASSERT_TRUE(doc["ok"].as<bool>());
  TEST_ASSERT_TRUE(doc["motores_liberados"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("", doc["falhas"].as<const char*>());
  TEST_ASSERT_EQUAL_UINT32(412, doc["boot_ms"].as<uint32_t>());
  TEST_ASSERT_EQUAL_UINT32(2000, doc["prazo_ms"].as<uint32_t>());
  TEST_ASSERT_TRUE(doc["no_prazo"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("ok", doc["dispositivos"]["tof_esquerdo"].as<const char*>());
  TEST_ASSERT_EQUAL_STRING("ok", doc["dispositivos"]["mpu6050"].as<const char*>());
  TEST_ASSERT_FALSE(doc["tof_0x29"].as<bool>());
  TEST_ASSERT_EQUAL_UINT8(0x68, doc["mpu6050_who_am_i"].as<uint8_t>());
  TEST_ASSERT_EQUAL_UINT32(0, doc["inesperados"].size());
  TEST_ASSERT_EQUAL_UINT8(0, doc["inesperados_total"].as<uint8_t>());
}

void test_relatorio_do_boot_com_falhas_e_atrasado() {
  BarramentoI2cSimulado barramento = barramentoCompleto();
  barramento.desconectar(pinos::ENDERECO_TOF_FRONTAL);
  barramento.conectar(0x3C);
  barramento.falharLeituras(true);
  const AutotesteBoot autoteste;
  const ResultadoBoot resultado = autoteste.verificar(barramento, {true, false, true});

  char buffer[TAMANHO_MAX_JSON_RELATORIO_BOOT];
  TEST_ASSERT_GREATER_THAN_UINT32(0, RelatorioBoot::serializar(resultado, autoteste, 2500, buffer, sizeof(buffer)));

  JsonDocument doc;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, buffer).code());
  TEST_ASSERT_FALSE(doc["ok"].as<bool>());
  TEST_ASSERT_FALSE(doc["motores_liberados"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("tof_frontal,mpu6050", doc["falhas"].as<const char*>());
  TEST_ASSERT_FALSE(doc["no_prazo"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("ausente", doc["dispositivos"]["tof_frontal"].as<const char*>());
  TEST_ASSERT_EQUAL_STRING("nao_identificado", doc["dispositivos"]["mpu6050"].as<const char*>());
  TEST_ASSERT_TRUE(doc["mpu6050_who_am_i"].isNull());
  TEST_ASSERT_EQUAL_UINT8(0x3C, doc["inesperados"][0].as<uint8_t>());
  TEST_ASSERT_EQUAL_UINT8(1, doc["inesperados_total"].as<uint8_t>());
}

void test_pior_caso_do_relatorio_cabe_e_buffer_pequeno_devolve_zero() {
  BarramentoI2cSimulado barramento;
  barramento.conectar(pinos::ENDERECO_FABRICA_TOF);
  barramento.conectar(pinos::ENDERECO_MPU6050);
  barramento.definirRegistrador(pinos::ENDERECO_MPU6050, REGISTRADOR_WHO_AM_I_MPU6050, 0xFF);
  for (uint8_t endereco = 0x60; endereco < 0x60 + MAXIMO_ENDERECOS_INESPERADOS + 5; endereco++) {
    if (endereco != pinos::ENDERECO_MPU6050) {
      barramento.conectar(endereco);
    }
  }
  barramento.conectar(pinos::ENDERECO_TOF_ESQ);
  const AutotesteBoot autoteste;
  const ResultadoBoot resultado = autoteste.verificar(barramento, {false, false, false});

  char buffer[TAMANHO_MAX_JSON_RELATORIO_BOOT];
  TEST_ASSERT_GREATER_THAN_UINT32(
      0, RelatorioBoot::serializar(resultado, autoteste, UINT32_MAX, buffer, sizeof(buffer)));

  char pequeno[16];
  TEST_ASSERT_EQUAL_UINT32(0, RelatorioBoot::serializar(resultado, autoteste, 0, pequeno, sizeof(pequeno)));
  TEST_ASSERT_EQUAL_STRING("", pequeno);
  TEST_ASSERT_EQUAL_UINT32(0, RelatorioBoot::serializar(resultado, autoteste, 0, nullptr, 100));
}

// --- Trava dos motores ---

void test_trava_comeca_travada_e_freia_em_vez_de_andar() {
  MotoresSimulados driver;
  MotoresComTrava tracao(driver);

  TEST_ASSERT_TRUE(tracao.travado());
  driver.definirPotencia({100, 100});  // estado qualquer antes
  tracao.definirPotencia({400, 400});
  TEST_ASSERT_TRUE(driver.freados());
  TEST_ASSERT_EQUAL_INT16(0, driver.ultimaPotencia().esquerda);
}

void test_trava_liberada_repassa_a_potencia() {
  MotoresSimulados driver;
  MotoresComTrava tracao(driver);
  tracao.liberar();

  TEST_ASSERT_FALSE(tracao.travado());
  tracao.definirPotencia({400, -300});
  TEST_ASSERT_FALSE(driver.freados());
  TEST_ASSERT_EQUAL_INT16(400, driver.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(-300, driver.ultimaPotencia().direita);
}

void test_travar_freia_na_hora_e_parar_sempre_passa() {
  MotoresSimulados driver;
  MotoresComTrava tracao(driver);
  tracao.liberar();
  tracao.definirPotencia({400, 400});

  tracao.travar();
  TEST_ASSERT_TRUE(driver.freados());
  tracao.definirPotencia({400, 400});
  TEST_ASSERT_TRUE(driver.freados());

  tracao.liberar();
  tracao.definirPotencia({200, 200});
  tracao.parar();
  TEST_ASSERT_TRUE(driver.freados());
  const uint32_t antes = driver.comandos();
  tracao.travar();
  tracao.parar();
  TEST_ASSERT_EQUAL_UINT32(antes + 2, driver.comandos());
}

// --- Sequência do boot como no main.cpp ---

void test_sequencia_do_boot_so_libera_os_motores_com_tudo_presente() {
  MotoresSimulados driver;

  BarramentoI2cSimulado semImu = barramentoCompleto();
  semImu.desconectar(pinos::ENDERECO_MPU6050);
  MotoresComTrava comFalha(driver);
  if (AutotesteBoot().verificar(semImu, TODOS_INICIADOS).aprovado()) {
    comFalha.liberar();
  }
  comFalha.definirPotencia({400, 400});
  TEST_ASSERT_TRUE(comFalha.travado());
  TEST_ASSERT_TRUE(driver.freados());

  BarramentoI2cSimulado completo = barramentoCompleto();
  MotoresComTrava semFalha(driver);
  if (AutotesteBoot().verificar(completo, TODOS_INICIADOS).aprovado()) {
    semFalha.liberar();
  }
  semFalha.definirPotencia({400, 400});
  TEST_ASSERT_FALSE(semFalha.travado());
  TEST_ASSERT_FALSE(driver.freados());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_os_quatro_presentes_aprovam_o_boot);
  RUN_TEST(test_a_varredura_cobre_0x08_a_0x77);
  RUN_TEST(test_cada_dispositivo_ausente_reprova_e_e_nomeado);
  RUN_TEST(test_tof_que_responde_mas_nao_iniciou_reprova);
  RUN_TEST(test_tof_iniciado_pelo_driver_mas_sem_resposta_fica_ausente);
  RUN_TEST(test_todos_ausentes_listam_todas_as_falhas_em_ordem);
  RUN_TEST(test_outro_chip_no_0x68_nao_passa_por_mpu6050);
  RUN_TEST(test_who_am_i_que_nao_le_reprova);
  RUN_TEST(test_who_am_i_aceito_vem_da_configuracao);
  RUN_TEST(test_tof_no_endereco_de_fabrica_reprova);
  RUN_TEST(test_endereco_inesperado_so_avisa);
  RUN_TEST(test_inesperados_alem_da_capacidade_so_entram_na_contagem);
  RUN_TEST(test_enderecos_fora_da_faixa_varrida_sao_ignorados);
  RUN_TEST(test_pior_caso_das_falhas_cabe_em_tamanho_max_falhas_boot);
  RUN_TEST(test_falhas_que_nao_cabem_devolvem_texto_vazio);
  RUN_TEST(test_nomes_dos_estados);
  RUN_TEST(test_prazo_de_2_s_do_rnf_09);
  RUN_TEST(test_relatorio_do_boot_aprovado);
  RUN_TEST(test_relatorio_do_boot_com_falhas_e_atrasado);
  RUN_TEST(test_pior_caso_do_relatorio_cabe_e_buffer_pequeno_devolve_zero);
  RUN_TEST(test_trava_comeca_travada_e_freia_em_vez_de_andar);
  RUN_TEST(test_trava_liberada_repassa_a_potencia);
  RUN_TEST(test_travar_freia_na_hora_e_parar_sempre_passa);
  RUN_TEST(test_sequencia_do_boot_so_libera_os_motores_com_tudo_presente);
  return UNITY_END();
}
