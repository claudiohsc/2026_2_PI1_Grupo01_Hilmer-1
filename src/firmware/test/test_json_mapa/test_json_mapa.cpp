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

/// Campos de uma celula no JSON, os mesmos das colunas da tabela `mapa_celulas` do banco.
const char* const CAMPOS_CELULA[] = {"linha",        "coluna",       "parede_norte",
                                     "parede_sul",   "parede_leste", "parede_oeste"};

/// Campos do objeto raiz do JSON.
const char* const CAMPOS_RAIZ[] = {"dimensao", "linhas", "colunas", "celulas"};

/// Byte usado para marcar o buffer antes da serializacao e detectar escrita indevida.
constexpr char SENTINELA = '#';

/// Campo do JSON que guarda a parede daquele lado da celula.
const char* campoDaParede(Direcao d) {
  return d == Direcao::Norte   ? "parede_norte"
         : d == Direcao::Leste ? "parede_leste"
         : d == Direcao::Sul   ? "parede_sul"
                               : "parede_oeste";
}

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
        const char* campo = campoDaParede(d);
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

/// `true` se `nome` esta na lista de campos esperados.
template <size_t N>
bool estaNaLista(const char* nome, const char* const (&esperados)[N]) {
  for (const char* esperado : esperados) {
    if (strcmp(nome, esperado) == 0) {
      return true;
    }
  }
  return false;
}

/// Confere que o objeto tem exatamente os campos esperados: nenhum a mais, nenhum a menos.
/// O tamanho igual ao da lista garante que, sem campo estranho, todos os esperados estao la.
template <size_t N>
void verificarCampos(JsonObjectConst objeto, const char* const (&esperados)[N]) {
  TEST_ASSERT_EQUAL_UINT32(N, objeto.size());
  for (JsonPairConst par : objeto) {
    TEST_ASSERT_TRUE_MESSAGE(estaNaLista(par.key().c_str(), esperados), par.key().c_str());
  }
}

/// Remonta o mapa a partir do JSON, como o dashboard faria ao receber a telemetria.
/// `destino` precisa ter a mesma dimensao do mapa serializado.
void reconstruirDoJson(const char* json, Mapa& destino) {
  JsonDocument doc;
  TEST_ASSERT_TRUE(deserializeJson(doc, json) == DeserializationError::Ok);
  for (JsonObjectConst celula : doc["celulas"].as<JsonArrayConst>()) {
    const PosicaoCelula posicao = {celula["linha"].as<uint8_t>(), celula["coluna"].as<uint8_t>()};
    for (Direcao d : DIRECOES) {
      const EstadoParede estado = estadoDoJson(celula[campoDaParede(d)]);
      if (estado == EstadoParede::Desconhecido) {
        continue;  // `null`: o robo ainda nao viu essa parede, nada a registrar.
      }
      const ResultadoRegistro resultado = destino.registrarParede(posicao, d, estado);
      TEST_ASSERT_TRUE(resultado == ResultadoRegistro::Registrada ||
                       resultado == ResultadoRegistro::JaConhecida);
    }
  }
}

/// Confere que os `quantidade` bytes a partir de `inicio` continuam com a sentinela.
void verificarSentinelas(const char* inicio, size_t quantidade) {
  for (size_t i = 0; i < quantidade; i++) {
    TEST_ASSERT_EQUAL_CHAR(SENTINELA, inicio[i]);
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

void test_json_tem_exatamente_os_campos_combinados_para_a_telemetria() {
  // Contrato com o dashboard: campo a mais ou a menos quebra a leitura do mapa e a insercao
  // na tabela `mapa_celulas` (que recebe esses nomes, mais o `corrida_id`).
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  char buffer[CAPACIDADE];
  MapaJson::serializar(mapa, buffer, sizeof(buffer));

  JsonDocument doc;
  TEST_ASSERT_TRUE(deserializeJson(doc, buffer) == DeserializationError::Ok);
  verificarCampos(doc.as<JsonObjectConst>(), CAMPOS_RAIZ);
  for (JsonObjectConst celula : doc["celulas"].as<JsonArrayConst>()) {
    verificarCampos(celula, CAMPOS_CELULA);
  }
}

void test_json_sai_compacto_sem_espacos_nem_quebras_de_linha() {
  // O pacote vai pela rede: nenhum caractere de formatacao deve entrar no texto.
  Mapa mapa(DimensaoMapa::Labirinto12x4);
  char buffer[CAPACIDADE];
  const size_t tamanho = MapaJson::serializar(mapa, buffer, sizeof(buffer));
  TEST_ASSERT_GREATER_THAN_UINT32(0, tamanho);
  TEST_ASSERT_NULL(strpbrk(buffer, " \t\r\n"));
}

void test_dimensao_combina_com_as_linhas_e_as_colunas_nos_tres_tamanhos() {
  // `dimensao` e escrito como colunas x linhas, ao contrario dos campos `linhas` e `colunas`.
  struct Caso {
    DimensaoMapa dimensao;
    const char* nome;
    uint8_t linhas;
    uint8_t colunas;
  };
  const Caso casos[] = {{DimensaoMapa::Labirinto4x4, "4x4", 4, 4},
                        {DimensaoMapa::Labirinto8x4, "8x4", 4, 8},
                        {DimensaoMapa::Labirinto12x4, "12x4", 4, 12}};

  for (const Caso& caso : casos) {
    const Mapa mapa(caso.dimensao);
    char buffer[CAPACIDADE];
    TEST_ASSERT_GREATER_THAN_UINT32(0, MapaJson::serializar(mapa, buffer, sizeof(buffer)));

    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, buffer) == DeserializationError::Ok);
    TEST_ASSERT_EQUAL_STRING(caso.nome, doc["dimensao"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT8(caso.linhas, doc["linhas"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(caso.colunas, doc["colunas"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT32(caso.linhas * caso.colunas, doc["celulas"].as<JsonArrayConst>().size());
  }
}

void test_mapa_12x4_completo_e_reconstruido_a_partir_do_json() {
  // O JSON precisa levar o mapa inteiro: quem recebe remonta um `Mapa` igual ao do robo.
  Mapa mapa(DimensaoMapa::Labirinto12x4);
  preencherParedesInternas(mapa, [](uint8_t linha, uint8_t coluna, Direcao d) {
    return ((linha * 5 + coluna * 2 + static_cast<uint8_t>(d)) % 4 == 0) ? EstadoParede::Parede : EstadoParede::Livre;
  });

  char buffer[CAPACIDADE];
  TEST_ASSERT_GREATER_THAN_UINT32(0, MapaJson::serializar(mapa, buffer, sizeof(buffer)));

  Mapa reconstruido(DimensaoMapa::Labirinto12x4);
  reconstruirDoJson(buffer, reconstruido);
  TEST_ASSERT_TRUE(mapa == reconstruido);
}

void test_mapa_parcial_reconstruido_mantem_as_paredes_desconhecidas() {
  // Mapa do meio da corrida: o `null` tem que voltar como `Desconhecido`, e nao como `Livre`.
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  mapa.registrarParede({0, 0}, Direcao::Norte, EstadoParede::Livre);
  mapa.registrarParede({1, 1}, Direcao::Leste, EstadoParede::Parede);

  char buffer[CAPACIDADE];
  TEST_ASSERT_GREATER_THAN_UINT32(0, MapaJson::serializar(mapa, buffer, sizeof(buffer)));

  Mapa reconstruido(DimensaoMapa::Labirinto4x4);
  reconstruirDoJson(buffer, reconstruido);
  TEST_ASSERT_TRUE(mapa == reconstruido);
  verificarEstado(EstadoParede::Desconhecido, reconstruido.obterParede({2, 2}, Direcao::Norte));
}

void test_serializar_nao_altera_o_mapa() {
  // A telemetria so le o mapa; a navegacao continua com ele durante a corrida (HU-04).
  Mapa mapa(DimensaoMapa::Labirinto8x4);
  preencherParedesInternas(mapa, [](uint8_t linha, uint8_t, Direcao) {
    return (linha % 2 == 0) ? EstadoParede::Parede : EstadoParede::Livre;
  });
  const Mapa antes = mapa;

  char buffer[CAPACIDADE];
  TEST_ASSERT_GREATER_THAN_UINT32(0, MapaJson::serializar(mapa, buffer, sizeof(buffer)));
  TEST_ASSERT_TRUE(mapa == antes);
}

void test_serializacoes_repetidas_do_mesmo_mapa_geram_o_mesmo_json() {
  // Sem isso, o dashboard veria o mapa "mudar" entre dois pacotes iguais.
  Mapa mapa(DimensaoMapa::Labirinto12x4);
  preencherParedesInternas(mapa, [](uint8_t linha, uint8_t coluna, Direcao) {
    return (coluna % 2 == 0 || linha == 1) ? EstadoParede::Parede : EstadoParede::Livre;
  });

  char primeiro[CAPACIDADE];
  char segundo[CAPACIDADE];
  const size_t tamanho = MapaJson::serializar(mapa, primeiro, sizeof(primeiro));
  TEST_ASSERT_GREATER_THAN_UINT32(0, tamanho);
  TEST_ASSERT_EQUAL_UINT32(tamanho, MapaJson::serializar(mapa, segundo, sizeof(segundo)));
  TEST_ASSERT_EQUAL_STRING(primeiro, segundo);
}

void test_nao_escreve_depois_do_fim_da_capacidade_informada() {
  // O buffer da telemetria e estatico e vizinho de outros dados: nada pode passar da capacidade.
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  char referencia[CAPACIDADE];
  const size_t tamanho = MapaJson::serializar(mapa, referencia, sizeof(referencia));
  TEST_ASSERT_GREATER_THAN_UINT32(0, tamanho);

  // Capacidade exata (JSON + `\0`): o resto da area fica intacto.
  char area[CAPACIDADE];
  const size_t capacidadeExata = tamanho + 1;
  memset(area, SENTINELA, sizeof(area));
  TEST_ASSERT_EQUAL_UINT32(tamanho, MapaJson::serializar(mapa, area, capacidadeExata));
  TEST_ASSERT_EQUAL_STRING(referencia, area);
  verificarSentinelas(area + capacidadeExata, sizeof(area) - capacidadeExata);

  // Capacidade insuficiente: so a string vazia entra, sem rabiscar depois do limite.
  memset(area, SENTINELA, sizeof(area));
  TEST_ASSERT_EQUAL_UINT32(0, MapaJson::serializar(mapa, area, tamanho));
  TEST_ASSERT_EQUAL_CHAR('\0', area[0]);
  verificarSentinelas(area + tamanho, sizeof(area) - tamanho);
}

void test_capacidade_de_um_byte_devolve_zero_e_string_vazia() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  char buffer[2];
  memset(buffer, SENTINELA, sizeof(buffer));
  TEST_ASSERT_EQUAL_UINT32(0, MapaJson::serializar(mapa, buffer, 1));
  TEST_ASSERT_EQUAL_CHAR('\0', buffer[0]);
  TEST_ASSERT_EQUAL_CHAR(SENTINELA, buffer[1]);
}

void test_dimensao_fora_do_enum_sai_como_texto_vazio() {
  // Fail-safe: um valor fora de `DimensaoMapa` (alcancavel so por conversao) nao pode travar
  // nem inventar um tamanho. O mapa nasce sem celulas e a dimensao sai vazia.
  const Mapa mapa(static_cast<DimensaoMapa>(99));
  char buffer[CAPACIDADE];
  const size_t tamanho = MapaJson::serializar(mapa, buffer, sizeof(buffer));
  TEST_ASSERT_GREATER_THAN_UINT32(0, tamanho);
  TEST_ASSERT_EQUAL_STRING("{\"dimensao\":\"\",\"linhas\":0,\"colunas\":0,\"celulas\":[]}", buffer);
}

int main(int, char**) {
  UNITY_BEGIN();
  // Formato do JSON
  RUN_TEST(test_mapa_vazio_4x4_tem_so_o_perimetro_conhecido);
  RUN_TEST(test_cabecalho_traz_dimensao_linhas_e_colunas);
  RUN_TEST(test_parede_livre_e_parede_viram_false_e_true);
  RUN_TEST(test_parede_registrada_aparece_nas_duas_celulas_vizinhas);
  RUN_TEST(test_json_tem_exatamente_os_campos_combinados_para_a_telemetria);
  RUN_TEST(test_json_sai_compacto_sem_espacos_nem_quebras_de_linha);
  RUN_TEST(test_dimensao_combina_com_as_linhas_e_as_colunas_nos_tres_tamanhos);
  // Os tres tamanhos da competicao
  RUN_TEST(test_serializa_mapa_4x4_completo);
  RUN_TEST(test_serializa_mapa_8x4);
  RUN_TEST(test_serializa_mapa_12x4_completo);
  RUN_TEST(test_serializa_mapa_12x4_parcial);
  // Ida e volta: o dashboard remonta o mapa
  RUN_TEST(test_mapa_12x4_completo_e_reconstruido_a_partir_do_json);
  RUN_TEST(test_mapa_parcial_reconstruido_mantem_as_paredes_desconhecidas);
  RUN_TEST(test_serializar_nao_altera_o_mapa);
  RUN_TEST(test_serializacoes_repetidas_do_mesmo_mapa_geram_o_mesmo_json);
  // Limites do buffer
  RUN_TEST(test_pior_caso_cabe_em_tamanho_max_json_mapa);
  RUN_TEST(test_buffer_pequeno_devolve_zero_e_deixa_string_vazia);
  RUN_TEST(test_buffer_com_o_tamanho_exato_serve_e_um_a_menos_nao);
  RUN_TEST(test_buffer_nulo_ou_sem_capacidade_devolve_zero);
  RUN_TEST(test_nao_escreve_depois_do_fim_da_capacidade_informada);
  RUN_TEST(test_capacidade_de_um_byte_devolve_zero_e_string_vazia);
  // Entrada fora do esperado
  RUN_TEST(test_dimensao_fora_do_enum_sai_como_texto_vazio);
  return UNITY_END();
}
