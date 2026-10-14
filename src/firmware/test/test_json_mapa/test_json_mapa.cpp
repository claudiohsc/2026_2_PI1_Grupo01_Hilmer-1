/// @file test_json_mapa.cpp
/// @brief Testes da serialização do mapa em JSON (`MapaJson`).
/// Roda no computador: `pio test -e native -f test_json_mapa`.

#include <ArduinoJson.h>
#include <string.h>
#include <unity.h>

#include "Direcao.h"
#include "Mapa.h"
#include "MapaJson.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

/// Buffer com o tamanho recomendado para qualquer mapa da competição.
constexpr size_t CAPACIDADE = TAMANHO_MAX_JSON_MAPA + 1;

const Direcao DIRECOES[] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};

/// `true`/`false`/`null` do JSON para o estado da parede.
EstadoParede estadoDoJson(JsonVariantConst valor) {
  if (valor.isNull()) {
    return EstadoParede::Desconhecido;
  }
  return valor.as<bool>() ? EstadoParede::Parede : EstadoParede::Livre;
}

void verificarEstado(EstadoParede esperado, EstadoParede obtido) {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(esperado), static_cast<uint8_t>(obtido));
}

/// Serializa, lê o JSON de volta e confere dimensões e as quatro paredes de todas as células.
/// @return Tamanho do JSON gerado.
size_t serializarEConferir(const Mapa& mapa, const char* dimensaoEsperada) {
  char buffer[CAPACIDADE];
  const size_t tamanho = MapaJson::serializar(mapa, buffer, sizeof(buffer));
  TEST_ASSERT_GREATER_THAN_UINT32(0, tamanho);
  TEST_ASSERT_EQUAL_UINT32(strlen(buffer), tamanho);

  JsonDocument doc;
  TEST_ASSERT_TRUE(deserializeJson(doc, buffer) == DeserializationError::Ok);
  TEST_ASSERT_EQUAL_STRING(dimensaoEsperada, doc["dimensao"].as<const char*>());
  TEST_ASSERT_EQUAL_UINT8(mapa.obterLinhas(), doc["linhas"].as<uint8_t>());
  TEST_ASSERT_EQUAL_UINT8(mapa.obterColunas(), doc["colunas"].as<uint8_t>());

  JsonArrayConst celulas = doc["celulas"].as<JsonArrayConst>();
  TEST_ASSERT_EQUAL_UINT32(mapa.obterLinhas() * mapa.obterColunas(), celulas.size());

  size_t indice = 0;
  for (uint8_t linha = 0; linha < mapa.obterLinhas(); linha++) {
    for (uint8_t coluna = 0; coluna < mapa.obterColunas(); coluna++) {
      JsonObjectConst c = celulas[indice++];
      TEST_ASSERT_EQUAL_UINT8(linha, c["linha"].as<uint8_t>());
      TEST_ASSERT_EQUAL_UINT8(coluna, c["coluna"].as<uint8_t>());
      for (Direcao d : DIRECOES) {
        const char* campo = d == Direcao::Norte   ? "parede_norte"
                            : d == Direcao::Leste ? "parede_leste"
                            : d == Direcao::Sul   ? "parede_sul"
                                                  : "parede_oeste";
        TEST_ASSERT_TRUE(c[campo].is<bool>() || c[campo].isNull());
        verificarEstado(mapa.obterParede({linha, coluna}, d), estadoDoJson(c[campo]));
      }
    }
  }
  return tamanho;
}

/// Registra em todas as paredes internas do mapa o estado devolvido por `escolher`.
template <typename Escolhe>
void preencherParedesInternas(Mapa& mapa, Escolhe escolher) {
  for (uint8_t linha = 0; linha < mapa.obterLinhas(); linha++) {
    for (uint8_t coluna = 0; coluna < mapa.obterColunas(); coluna++) {
      for (Direcao d : {Direcao::Norte, Direcao::Leste}) {
        PosicaoCelula vizinha_;
        if (vizinha({linha, coluna}, d, vizinha_) && mapa.posValida(vizinha_)) {
          mapa.registrarParede({linha, coluna}, d, escolher(linha, coluna, d));
        }
      }
    }
  }
}

}  // namespace

void test_mapa_vazio_4x4_tem_so_o_perimetro_conhecido() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  serializarEConferir(mapa, "4x4");

  char buffer[CAPACIDADE];
  MapaJson::serializar(mapa, buffer, sizeof(buffer));
  // Célula (0, 0): Sul e Oeste são o perímetro; Norte e Leste ainda são desconhecidos.
  TEST_ASSERT_NOT_NULL(strstr(buffer,
      "{\"linha\":0,\"coluna\":0,\"parede_norte\":null,\"parede_sul\":true,"
      "\"parede_leste\":null,\"parede_oeste\":true}"));
}

void test_cabecalho_traz_dimensao_linhas_e_colunas() {
  char buffer[CAPACIDADE];
  Mapa mapa(DimensaoMapa::Labirinto12x4);
  MapaJson::serializar(mapa, buffer, sizeof(buffer));
  const char* inicio = "{\"dimensao\":\"12x4\",\"linhas\":4,\"colunas\":12,\"celulas\":[";
  TEST_ASSERT_EQUAL_INT(0, strncmp(buffer, inicio, strlen(inicio)));
}

void test_parede_livre_e_parede_viram_false_e_true() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  mapa.registrarParede({1, 1}, Direcao::Norte, EstadoParede::Parede);
  mapa.registrarParede({1, 1}, Direcao::Leste, EstadoParede::Livre);

  char buffer[CAPACIDADE];
  MapaJson::serializar(mapa, buffer, sizeof(buffer));
  TEST_ASSERT_NOT_NULL(strstr(buffer,
      "{\"linha\":1,\"coluna\":1,\"parede_norte\":true,\"parede_sul\":null,"
      "\"parede_leste\":false,\"parede_oeste\":null}"));
  serializarEConferir(mapa, "4x4");
}

void test_parede_registrada_aparece_nas_duas_celulas_vizinhas() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  mapa.registrarParede({0, 0}, Direcao::Leste, EstadoParede::Parede);

  char buffer[CAPACIDADE];
  MapaJson::serializar(mapa, buffer, sizeof(buffer));
  // Leste de (0, 0) e Oeste de (0, 1) são a mesma parede.
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"coluna\":0,\"parede_norte\":null,\"parede_sul\":true,\"parede_leste\":true"));
  TEST_ASSERT_NOT_NULL(strstr(buffer, "\"coluna\":1,\"parede_norte\":null,\"parede_sul\":true,\"parede_leste\":null,\"parede_oeste\":true"));
}

void test_serializa_mapa_4x4_completo() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  preencherParedesInternas(mapa, [](uint8_t linha, uint8_t coluna, Direcao d) {
    return ((linha + coluna + static_cast<uint8_t>(d)) % 2 == 0) ? EstadoParede::Parede : EstadoParede::Livre;
  });
  serializarEConferir(mapa, "4x4");
}

void test_serializa_mapa_8x4() {
  Mapa mapa(DimensaoMapa::Labirinto8x4);
  preencherParedesInternas(mapa, [](uint8_t linha, uint8_t coluna, Direcao) {
    return (coluna % 3 == 0 || linha == 2) ? EstadoParede::Parede : EstadoParede::Livre;
  });
  serializarEConferir(mapa, "8x4");
}

void test_serializa_mapa_12x4_completo() {
  Mapa mapa(DimensaoMapa::Labirinto12x4);
  preencherParedesInternas(mapa, [](uint8_t linha, uint8_t coluna, Direcao d) {
    return ((linha * 7 + coluna * 3 + static_cast<uint8_t>(d)) % 3 == 0) ? EstadoParede::Parede : EstadoParede::Livre;
  });
  const size_t tamanho = serializarEConferir(mapa, "12x4");
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(TAMANHO_MAX_JSON_MAPA, tamanho);
}

void test_serializa_mapa_12x4_parcial() {
  // Só algumas paredes conhecidas, como no meio de uma corrida.
  Mapa mapa(DimensaoMapa::Labirinto12x4);
  mapa.registrarParede({0, 0}, Direcao::Norte, EstadoParede::Livre);
  mapa.registrarParede({1, 0}, Direcao::Leste, EstadoParede::Parede);
  mapa.registrarParede({3, 11}, Direcao::Oeste, EstadoParede::Livre);
  serializarEConferir(mapa, "12x4");
}

void test_pior_caso_cabe_em_tamanho_max_json_mapa() {
  // Todas as paredes internas livres: `false` tem mais caracteres que `true` e `null`.
  Mapa mapa(DimensaoMapa::Labirinto12x4);
  preencherParedesInternas(mapa, [](uint8_t, uint8_t, Direcao) { return EstadoParede::Livre; });
  const size_t tamanho = serializarEConferir(mapa, "12x4");
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(TAMANHO_MAX_JSON_MAPA, tamanho);
  // A constante não está folgada demais (menos de 10% acima do pior caso).
  TEST_ASSERT_LESS_THAN_UINT32(tamanho + tamanho / 10, TAMANHO_MAX_JSON_MAPA);
}

void test_buffer_pequeno_devolve_zero_e_deixa_string_vazia() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  char buffer[64];
  memset(buffer, 'x', sizeof(buffer));
  TEST_ASSERT_EQUAL_UINT32(0, MapaJson::serializar(mapa, buffer, sizeof(buffer)));
  TEST_ASSERT_EQUAL_CHAR('\0', buffer[0]);
}

void test_buffer_com_o_tamanho_exato_serve_e_um_a_menos_nao() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  char grande[CAPACIDADE];
  const size_t tamanho = MapaJson::serializar(mapa, grande, sizeof(grande));
  TEST_ASSERT_GREATER_THAN_UINT32(0, tamanho);

  char exato[CAPACIDADE];
  TEST_ASSERT_EQUAL_UINT32(tamanho, MapaJson::serializar(mapa, exato, tamanho + 1));
  TEST_ASSERT_EQUAL_STRING(grande, exato);
  TEST_ASSERT_EQUAL_UINT32(0, MapaJson::serializar(mapa, exato, tamanho));
}

void test_buffer_nulo_ou_sem_capacidade_devolve_zero() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  char buffer[8];
  TEST_ASSERT_EQUAL_UINT32(0, MapaJson::serializar(mapa, nullptr, 100));
  TEST_ASSERT_EQUAL_UINT32(0, MapaJson::serializar(mapa, buffer, 0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_mapa_vazio_4x4_tem_so_o_perimetro_conhecido);
  RUN_TEST(test_cabecalho_traz_dimensao_linhas_e_colunas);
  RUN_TEST(test_parede_livre_e_parede_viram_false_e_true);
  RUN_TEST(test_parede_registrada_aparece_nas_duas_celulas_vizinhas);
  RUN_TEST(test_serializa_mapa_4x4_completo);
  RUN_TEST(test_serializa_mapa_8x4);
  RUN_TEST(test_serializa_mapa_12x4_completo);
  RUN_TEST(test_serializa_mapa_12x4_parcial);
  RUN_TEST(test_pior_caso_cabe_em_tamanho_max_json_mapa);
  RUN_TEST(test_buffer_pequeno_devolve_zero_e_deixa_string_vazia);
  RUN_TEST(test_buffer_com_o_tamanho_exato_serve_e_um_a_menos_nao);
  RUN_TEST(test_buffer_nulo_ou_sem_capacidade_devolve_zero);
  return UNITY_END();
}
