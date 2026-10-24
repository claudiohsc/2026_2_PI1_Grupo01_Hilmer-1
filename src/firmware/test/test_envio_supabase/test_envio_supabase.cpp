/// @file test_envio_supabase.cpp
/// @brief Testes do lado do robô no envio ao Supabase (#238, cliente da #236): o JSON do lote e das
/// outras chamadas, a leitura das respostas, as credenciais, a reconexão do Wi-Fi e a cadeia coleta →
/// buffer → JSON → servidor simulado, com a rede caindo no meio.
/// Roda no computador: `pio test -e native -f test_envio_supabase`.

#include <ArduinoJson.h>
#include <unity.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "AmostraTelemetria.h"
#include "ConfiguracaoRede.h"
#include "ContratoTelemetria.h"
#include "ControladorEnvio.h"
#include "CredenciaisSupabase.h"
#include "EnvioSimulado.h"
#include "IRedeWifi.h"
#include "ReconexaoWifi.h"
#include "TravaNula.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

AmostraTelemetria amostraExemplo() {
  AmostraTelemetria a;
  a.numeroSequencia = 3;
  a.tempoDecorridoMs = 1500;
  a.velocidadeMmPorS = 120;
  a.tensaoBateriaMv = 7400;
  a.correnteBateriaMa = -250;
  a.posicaoLinha = 1;
  a.posicaoColuna = 2;
  a.orientacao = 'L';
  a.estado = EstadoCorrida::Explorando;
  a.evento = EventoTelemetria::Nenhum;
  return a;
}

/// A pior amostra possível para o tamanho do JSON: todos os números no maior comprimento.
AmostraTelemetria amostraMaisLonga() {
  AmostraTelemetria a;
  a.numeroSequencia = UINT32_MAX;
  a.tempoDecorridoMs = UINT32_MAX;
  a.velocidadeMmPorS = INT32_MIN;
  a.tensaoBateriaMv = UINT16_MAX;
  a.correnteBateriaMa = INT16_MIN;
  a.posicaoLinha = UINT8_MAX;
  a.posicaoColuna = UINT8_MAX;
  a.orientacao = 'O';
  a.estado = EstadoCorrida::Explorando;
  a.evento = EventoTelemetria::ObjetivoAlcancado;
  return a;
}

/// Rede Wi-Fi falsa: o teste decide quando ela conecta.
class RedeFalsa : public IRedeWifi {
 public:
  bool conectada() override { return conectada_; }
  void iniciarConexao() override { ++inicios_; }
  void desconectar() override { ++desconexoes_; conectada_ = false; }
  void definirConectada(bool conectada) { conectada_ = conectada; }
  int inicios() const { return inicios_; }
  int desconexoes() const { return desconexoes_; }

 private:
  bool conectada_ = false;
  int inicios_ = 0;
  int desconexoes_ = 0;
};

/// Cliente "HTTP" para o teste de integração: transforma o lote em JSON com `serializarLote`, lê o
/// JSON de volta como o backend faria, grava no servidor simulado e devolve a resposta também em
/// JSON, lida por `lerRespostaLote`. Assim a cadeia inteira passa pelo contrato.
class EnvioPorJson : public IEnvioTelemetria {
 public:
  explicit EnvioPorJson(EnvioSimulado& servidor) : servidor_(servidor) {}

  ResultadoEnvio enviarLote(uint32_t corridaId, const AmostraTelemetria* amostras, uint16_t quantidade) override {
    static char json[TAMANHO_MAXIMO_LOTE_JSON];
    if (serializarLote(corridaId, amostras, quantidade, json, sizeof(json)) == 0) {
      return ResultadoEnvio{-1000, 0, 0};
    }
    JsonDocument doc;
    TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, json).code());
    static AmostraTelemetria lidas[TAMANHO_MAXIMO_LOTE];
    uint16_t n = 0;
    for (JsonObjectConst o : doc["amostras"].as<JsonArrayConst>()) {
      AmostraTelemetria a;
      a.numeroSequencia = o["numero_sequencia"].as<uint32_t>();
      a.posicaoLinha = o["posicao_linha"].as<uint8_t>();
      a.posicaoColuna = o["posicao_coluna"].as<uint8_t>();
      a.orientacao = o["orientacao"].as<const char*>()[0];
      a.velocidadeMmPorS = static_cast<int32_t>(std::lround(o["velocidade"].as<double>() * 1000.0));
      a.tempoDecorridoMs = static_cast<uint32_t>(std::lround(o["tempo_decorrido"].as<double>() * 1000.0));
      lidas[n++] = a;
    }
    const ResultadoEnvio r = servidor_.enviarLote(doc["corrida_id"].as<uint32_t>(), lidas, n);
    if (r.codigoHttp < 200 || r.codigoHttp >= 300) {
      return r;
    }
    char resposta[96];
    std::snprintf(resposta, sizeof(resposta), "{\"gravadas\":%u,\"existentes\":%u,\"reconsolidada\":false}",
                  static_cast<unsigned>(r.gravadas), static_cast<unsigned>(r.existentes));
    ResultadoEnvio lido{r.codigoHttp, 0, 0};
    TEST_ASSERT_TRUE(lerRespostaLote(resposta, lido.gravadas, lido.existentes));
    return lido;
  }

  ResultadoEnvio encerrarCorrida(uint32_t corridaId) override { return servidor_.encerrarCorrida(corridaId); }

 private:
  EnvioSimulado& servidor_;
};

/// Valor numérico do JSON em milésimos, arredondado (0.12 → 120), para comparar sem ponto flutuante:
/// o `pio test -e native` não liga a precisão dupla do Unity.
long long milesimos(JsonVariantConst valor) { return std::llround(valor.as<double>() * 1000.0); }

/// Abre o contrato único do lote (`src/backend/tests/fixtures/lote-telemetria.json`). Tenta a partir
/// deste arquivo (`__FILE__` aponta para src/firmware/test/<teste>/...) e a partir da pasta do projeto
/// (`pio test` roda dentro de src/firmware). Devolve nullptr se não achar.
FILE* abrirContrato() {
  std::string base = __FILE__;
  for (int i = 0; i < 3 && base != "."; ++i) {
    const size_t barra = base.find_last_of("/\\");
    base = barra == std::string::npos ? std::string(".") : base.substr(0, barra);
  }
  const std::string candidatos[] = {base + "/../backend/tests/fixtures/lote-telemetria.json",
                                    "../backend/tests/fixtures/lote-telemetria.json"};
  for (const std::string& caminho : candidatos) {
    FILE* arquivo = std::fopen(caminho.c_str(), "rb");
    if (arquivo != nullptr) {
      return arquivo;
    }
  }
  return nullptr;
}

}  // namespace

// ---------------------------------------------------------------- JSON do lote

void test_lote_com_uma_amostra_tem_os_nomes_das_colunas_e_as_unidades_do_banco() {
  const AmostraTelemetria a = amostraExemplo();
  char json[512];
  const size_t n = serializarLote(7, &a, 1, json, sizeof(json));
  TEST_ASSERT_EQUAL_STRING(
      "{\"corrida_id\":7,\"amostras\":[{\"numero_sequencia\":3,\"posicao_linha\":1,\"posicao_coluna\":2,"
      "\"orientacao\":\"L\",\"velocidade\":0.120,\"tensao_bateria\":7.400,\"corrente_bateria\":-0.250,"
      "\"tempo_decorrido\":1.500,\"estado_corrida\":\"explorando\"}]}",
      json);
  TEST_ASSERT_EQUAL_size_t(std::strlen(json), n);
}

void test_evento_so_aparece_quando_existe() {
  AmostraTelemetria a = amostraExemplo();
  char json[512];
  serializarLote(1, &a, 1, json, sizeof(json));
  TEST_ASSERT_NULL(std::strstr(json, "evento"));
  a.evento = EventoTelemetria::Colisao;
  serializarLote(1, &a, 1, json, sizeof(json));
  TEST_ASSERT_NOT_NULL(std::strstr(json, ",\"evento\":\"colisao\"}"));
  a.evento = EventoTelemetria::ObjetivoAlcancado;
  serializarLote(1, &a, 1, json, sizeof(json));
  TEST_ASSERT_NOT_NULL(std::strstr(json, ",\"evento\":\"objetivo_alcancado\"}"));
}

void test_lote_com_varias_amostras_em_ordem_e_json_valido() {
  AmostraTelemetria amostras[3] = {amostraExemplo(), amostraExemplo(), amostraExemplo()};
  for (uint32_t i = 0; i < 3; ++i) {
    amostras[i].numeroSequencia = 10 + i;
  }
  char json[2048];
  TEST_ASSERT_TRUE(serializarLote(42, amostras, 3, json, sizeof(json)) > 0);
  JsonDocument doc;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, json).code());
  TEST_ASSERT_EQUAL_UINT32(42, doc["corrida_id"].as<uint32_t>());
  JsonArrayConst lista = doc["amostras"].as<JsonArrayConst>();
  TEST_ASSERT_EQUAL_size_t(3, lista.size());
  for (uint32_t i = 0; i < 3; ++i) {
    TEST_ASSERT_EQUAL_UINT32(10 + i, lista[i]["numero_sequencia"].as<uint32_t>());
    TEST_ASSERT_EQUAL_INT32(120, static_cast<int32_t>(milesimos(lista[i]["velocidade"])));
  }
}

void test_numeros_negativos_e_pequenos_com_tres_casas() {
  AmostraTelemetria a = amostraExemplo();
  a.velocidadeMmPorS = -5;      // −0,005 m/s
  a.correnteBateriaMa = -1000;  // −1,000 A
  a.tempoDecorridoMs = 0;
  char json[512];
  serializarLote(1, &a, 1, json, sizeof(json));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"velocidade\":-0.005,"));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"corrente_bateria\":-1.000,"));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"tempo_decorrido\":0.000,"));
}

void test_lote_vazio_e_orientacao_invalida_nao_quebram_o_json() {
  char json[128];
  TEST_ASSERT_TRUE(serializarLote(5, nullptr, 0, json, sizeof(json)) > 0);
  TEST_ASSERT_EQUAL_STRING("{\"corrida_id\":5,\"amostras\":[]}", json);

  AmostraTelemetria a = amostraExemplo();
  a.orientacao = '"';  // um caractere que quebraria o JSON
  char outro[512];
  serializarLote(1, &a, 1, outro, sizeof(outro));
  JsonDocument doc;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, outro).code());
  TEST_ASSERT_EQUAL_STRING("?", doc["amostras"][0]["orientacao"].as<const char*>());  // o servidor recusa
}

void test_lote_que_nao_cabe_devolve_zero_e_texto_vazio() {
  const AmostraTelemetria a = amostraExemplo();
  char json[512];
  const size_t cheio = serializarLote(7, &a, 1, json, sizeof(json));
  char justo[512];
  TEST_ASSERT_EQUAL_size_t(cheio, serializarLote(7, &a, 1, justo, cheio + 1));  // cabe com o '\0'
  TEST_ASSERT_EQUAL_size_t(0, serializarLote(7, &a, 1, justo, cheio));          // falta 1 byte
  TEST_ASSERT_EQUAL_STRING("", justo);
  TEST_ASSERT_EQUAL_size_t(0, serializarLote(7, &a, 1, nullptr, 100));
  TEST_ASSERT_EQUAL_size_t(0, serializarLote(7, nullptr, 1, justo, sizeof(justo)));
}

void test_pior_amostra_e_pior_lote_cabem_no_espaco_reservado() {
  const AmostraTelemetria a = amostraMaisLonga();
  static char json[TAMANHO_MAXIMO_LOTE_JSON];
  const size_t uma = serializarLote(UINT32_MAX, &a, 1, json, sizeof(json));
  TEST_ASSERT_TRUE(uma > 0);
  const size_t envelope = std::strlen("{\"corrida_id\":4294967295,\"amostras\":[]}");
  TEST_ASSERT_TRUE(uma - envelope <= TAMANHO_MAXIMO_AMOSTRA_JSON);

  static AmostraTelemetria lote[TAMANHO_MAXIMO_LOTE];
  for (uint16_t i = 0; i < TAMANHO_MAXIMO_LOTE; ++i) {
    lote[i] = a;
  }
  TEST_ASSERT_TRUE(serializarLote(UINT32_MAX, lote, TAMANHO_MAXIMO_LOTE, json, sizeof(json)) > 0);
  JsonDocument doc;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, json).code());
  TEST_ASSERT_EQUAL_size_t(TAMANHO_MAXIMO_LOTE, doc["amostras"].as<JsonArrayConst>().size());
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"velocidade\":-2147483.648,"));
}

/// Contrato único: o lote que o firmware gera para as amostras do exemplo do backend é igual, campo a
/// campo, ao `src/backend/tests/fixtures/lote-telemetria.json`, que os testes do backend aceitam.
void test_contrato_lote_igual_ao_exemplo_do_backend() {
  FILE* arquivo = abrirContrato();
  if (arquivo == nullptr) {
    TEST_IGNORE_MESSAGE("lote-telemetria.json ainda não está nesta branch (chega com a #229, PR #398)");
  }
  std::string texto;
  char pedaco[512];
  size_t lidos = 0;
  while ((lidos = std::fread(pedaco, 1, sizeof(pedaco), arquivo)) > 0) {
    texto.append(pedaco, lidos);
  }
  std::fclose(arquivo);

  JsonDocument esperado;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(esperado, texto).code());
  static AmostraTelemetria amostras[16];
  uint16_t n = 0;
  for (JsonObjectConst o : esperado["amostras"].as<JsonArrayConst>()) {
    AmostraTelemetria a;
    a.numeroSequencia = o["numero_sequencia"].as<uint32_t>();
    a.posicaoLinha = o["posicao_linha"].as<uint8_t>();
    a.posicaoColuna = o["posicao_coluna"].as<uint8_t>();
    a.orientacao = o["orientacao"].as<const char*>()[0];
    a.velocidadeMmPorS = static_cast<int32_t>(std::lround(o["velocidade"].as<double>() * 1000.0));
    a.tensaoBateriaMv = static_cast<uint16_t>(std::lround(o["tensao_bateria"].as<double>() * 1000.0));
    a.correnteBateriaMa = static_cast<int16_t>(std::lround(o["corrente_bateria"].as<double>() * 1000.0));
    a.tempoDecorridoMs = static_cast<uint32_t>(std::lround(o["tempo_decorrido"].as<double>() * 1000.0));
    const char* estado = o["estado_corrida"].as<const char*>();
    a.estado = std::strcmp(estado, "aguardando") == 0   ? EstadoCorrida::Aguardando
               : std::strcmp(estado, "explorando") == 0 ? EstadoCorrida::Explorando
               : std::strcmp(estado, "retornando") == 0 ? EstadoCorrida::Retornando
                                                        : EstadoCorrida::Parado;
    const char* evento = o["evento"] | "";
    a.evento = std::strcmp(evento, "colisao") == 0              ? EventoTelemetria::Colisao
               : std::strcmp(evento, "objetivo_alcancado") == 0 ? EventoTelemetria::ObjetivoAlcancado
                                                                : EventoTelemetria::Nenhum;
    amostras[n++] = a;
  }

  static char json[8192];
  TEST_ASSERT_TRUE(serializarLote(esperado["corrida_id"].as<uint32_t>(), amostras, n, json, sizeof(json)) > 0);
  JsonDocument gerado;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(gerado, json).code());
  TEST_ASSERT_EQUAL_UINT32(esperado["corrida_id"].as<uint32_t>(), gerado["corrida_id"].as<uint32_t>());
  JsonArrayConst a = esperado["amostras"].as<JsonArrayConst>();
  JsonArrayConst b = gerado["amostras"].as<JsonArrayConst>();
  TEST_ASSERT_EQUAL_size_t(a.size(), b.size());
  for (size_t i = 0; i < a.size(); ++i) {
    JsonObjectConst ea = a[i];
    JsonObjectConst gb = b[i];
    TEST_ASSERT_EQUAL_size_t(ea.size(), gb.size());  // os mesmos campos, nem mais nem menos
    for (JsonPairConst campo : ea) {
      JsonVariantConst valor = gb[campo.key()];
      TEST_ASSERT_FALSE_MESSAGE(valor.isNull(), campo.key().c_str());
      if (campo.value().is<const char*>()) {
        TEST_ASSERT_EQUAL_STRING(campo.value().as<const char*>(), valor.as<const char*>());
      } else {
        TEST_ASSERT_EQUAL_INT32_MESSAGE(static_cast<int32_t>(milesimos(campo.value())),
                                        static_cast<int32_t>(milesimos(valor)), campo.key().c_str());
      }
    }
  }
}

// ---------------------------------------------------------------- respostas do servidor

void test_le_as_contagens_da_resposta_do_lote() {
  uint16_t g = 99;
  uint16_t e = 99;
  TEST_ASSERT_TRUE(lerRespostaLote("{\"gravadas\":3,\"existentes\":2,\"reconsolidada\":false}", g, e));
  TEST_ASSERT_EQUAL_UINT16(3, g);
  TEST_ASSERT_EQUAL_UINT16(2, e);
  TEST_ASSERT_TRUE(lerRespostaLote("{\"existentes\":0,\"gravadas\":100}", g, e));
  TEST_ASSERT_EQUAL_UINT16(100, g);
}

void test_resposta_do_lote_sem_as_contagens_e_recusada() {
  uint16_t g = 7;
  uint16_t e = 7;
  const char* invalidas[] = {
      "{\"gravadas\":3}",                         // falta existentes
      "{\"gravadas\":-1,\"existentes\":0}",       // negativo
      "{\"gravadas\":1.5,\"existentes\":0}",      // não inteiro
      "{\"gravadas\":\"3\",\"existentes\":0}",    // texto
      "{\"gravadas\":70000,\"existentes\":0}",    // maior que 65535
      "[3,2]",                                    // não é objeto
      "{\"erro\":\"nao_autorizado\"}",            // corpo de erro
      "isto não é JSON",
      "",
  };
  for (const char* corpo : invalidas) {
    TEST_ASSERT_FALSE_MESSAGE(lerRespostaLote(corpo, g, e), corpo);
  }
  TEST_ASSERT_FALSE(lerRespostaLote(nullptr, g, e));
  TEST_ASSERT_EQUAL_UINT16(7, g);  // nada muda quando a leitura falha
  TEST_ASSERT_EQUAL_UINT16(7, e);
}

void test_le_o_id_da_corrida_criada() {
  uint32_t id = 0;
  TEST_ASSERT_TRUE(lerIdCorrida(
      "{\"id\":12,\"labirinto_id\":1,\"numero_tentativa\":3,\"status\":\"em_andamento\",\"tempo_final\":null}", id));
  TEST_ASSERT_EQUAL_UINT32(12, id);
  TEST_ASSERT_FALSE(lerIdCorrida("{\"labirinto_id\":1}", id));
  TEST_ASSERT_FALSE(lerIdCorrida("{\"id\":0}", id));
  TEST_ASSERT_FALSE(lerIdCorrida("{\"id\":\"12\"}", id));
  TEST_ASSERT_FALSE(lerIdCorrida("{\"id\":3000000000}", id));  // passa do inteiro de 32 bits do banco
  TEST_ASSERT_FALSE(lerIdCorrida("nada", id));
  TEST_ASSERT_FALSE(lerIdCorrida(nullptr, id));
  TEST_ASSERT_EQUAL_UINT32(12, id);
}

void test_corpos_da_criacao_e_do_encerramento() {
  char corpo[48];
  TEST_ASSERT_EQUAL_size_t(19, serializarCriacaoCorrida("4x4", corpo, sizeof(corpo)));
  TEST_ASSERT_EQUAL_STRING("{\"labirinto\":\"4x4\"}", corpo);
  TEST_ASSERT_TRUE(serializarEncerramento(true, corpo, sizeof(corpo)) > 0);
  TEST_ASSERT_EQUAL_STRING("{\"status\":\"concluida\"}", corpo);
  TEST_ASSERT_TRUE(serializarEncerramento(false, corpo, sizeof(corpo)) > 0);
  TEST_ASSERT_EQUAL_STRING("{\"status\":\"nao_concluida\"}", corpo);
  TEST_ASSERT_EQUAL_size_t(0, serializarCriacaoCorrida("12x4", corpo, 8));
  TEST_ASSERT_EQUAL_size_t(0, serializarEncerramento(true, corpo, 8));
  TEST_ASSERT_EQUAL_size_t(0, serializarCriacaoCorrida(nullptr, corpo, sizeof(corpo)));
  TEST_ASSERT_EQUAL_size_t(0, serializarEncerramento(true, nullptr, 0));
}

void test_amostra_montada_a_partir_da_pose() {
  const Pose pose{{2, 3}, Direcao::Leste};
  const AmostraTelemetria a =
      montarAmostra(pose, 4200, 150, EstadoCorrida::Parado, EventoTelemetria::ObjetivoAlcancado);
  TEST_ASSERT_EQUAL_UINT8(2, a.posicaoLinha);
  TEST_ASSERT_EQUAL_UINT8(3, a.posicaoColuna);
  TEST_ASSERT_EQUAL_CHAR('L', a.orientacao);
  TEST_ASSERT_EQUAL_UINT32(4200, a.tempoDecorridoMs);
  TEST_ASSERT_EQUAL_INT32(150, a.velocidadeMmPorS);
  TEST_ASSERT_EQUAL_UINT32(0, a.numeroSequencia);  // quem numera é o registrar()
  TEST_ASSERT_EQUAL_UINT16(0, a.tensaoBateriaMv);  // a Rev 2.0 não mede a bateria
  TEST_ASSERT_EQUAL_INT16(0, a.correnteBateriaMa);
  TEST_ASSERT_TRUE(a.estado == EstadoCorrida::Parado);
  TEST_ASSERT_TRUE(a.evento == EventoTelemetria::ObjetivoAlcancado);
}

// ---------------------------------------------------------------- credenciais e URLs

namespace {

CredenciaisSupabase credenciaisCompletas() {
  CredenciaisSupabase c;
  c.ssidWifi = "lab";
  c.senhaWifi = "senha";
  c.urlProjeto = "https://abc.supabase.co";
  c.chaveAnon = "anon";
  c.tokenDispositivo = "token";
  c.certificadoRaiz = "-----BEGIN CERTIFICATE-----";
  return c;
}

}  // namespace

void test_sem_credenciais_o_envio_fica_desligado() {
  TEST_ASSERT_FALSE(credenciaisValidas(CredenciaisSupabase{}));  // sem include/credenciais.h
  TEST_ASSERT_TRUE(credenciaisValidas(credenciaisCompletas()));

  CredenciaisSupabase c = credenciaisCompletas();
  c.tokenDispositivo = "";
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c = credenciaisCompletas();
  c.chaveAnon = nullptr;
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c = credenciaisCompletas();
  c.ssidWifi = "";
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c = credenciaisCompletas();
  c.senhaWifi = "";  // rede aberta: vale
  TEST_ASSERT_TRUE(credenciaisValidas(c));
}

void test_so_https_e_com_certificado_ou_excecao_de_bancada() {
  CredenciaisSupabase c = credenciaisCompletas();
  c.urlProjeto = "http://abc.supabase.co";
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c.urlProjeto = "https://";
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c = credenciaisCompletas();
  c.certificadoRaiz = "";
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c.semValidarCertificado = true;  // só na bancada
  TEST_ASSERT_TRUE(credenciaisValidas(c));
}

void test_url_das_funcoes() {
  char url[96];
  TEST_ASSERT_TRUE(montarUrlFuncao("https://abc.supabase.co", "telemetria", url, sizeof(url)) > 0);
  TEST_ASSERT_EQUAL_STRING("https://abc.supabase.co/functions/v1/telemetria", url);
  montarUrlFuncao("https://abc.supabase.co/", "/corridas/12/encerrar", url, sizeof(url));
  TEST_ASSERT_EQUAL_STRING("https://abc.supabase.co/functions/v1/corridas/12/encerrar", url);
  TEST_ASSERT_EQUAL_size_t(0, montarUrlFuncao("https://abc.supabase.co", "corridas", url, 20));
  TEST_ASSERT_EQUAL_STRING("", url);
  TEST_ASSERT_EQUAL_size_t(0, montarUrlFuncao(nullptr, "corridas", url, sizeof(url)));
}

// ---------------------------------------------------------------- reconexão do Wi-Fi

void test_wifi_conecta_na_primeira_tentativa() {
  RedeFalsa rede;
  ReconexaoWifi wifi(rede);
  TEST_ASSERT_FALSE(wifi.ciclo(0));
  TEST_ASSERT_EQUAL_INT(1, rede.inicios());
  TEST_ASSERT_TRUE(wifi.estado() == EstadoWifi::Conectando);
  rede.definirConectada(true);
  TEST_ASSERT_TRUE(wifi.ciclo(100));
  TEST_ASSERT_TRUE(wifi.estado() == EstadoWifi::Conectado);
  TEST_ASSERT_TRUE(wifi.ciclo(200));
  TEST_ASSERT_EQUAL_INT(1, rede.inicios());  // conectado: não tenta de novo
}

void test_wifi_espera_cada_vez_mais_entre_tentativas_e_volta_a_1s() {
  RedeFalsa rede;
  ReconexaoWifi wifi(rede, 1000);  // tentativa de 1 s, para o teste
  uint32_t t = 0;
  wifi.ciclo(t);                                   // 1ª tentativa
  wifi.ciclo(t += 1000);                           // esgotou: espera 1 s
  TEST_ASSERT_EQUAL_INT(1, rede.desconexoes());
  TEST_ASSERT_TRUE(wifi.estado() == EstadoWifi::Aguardando);
  wifi.ciclo(t += 900);
  TEST_ASSERT_EQUAL_INT(1, rede.inicios());        // ainda não deu 1 s
  wifi.ciclo(t += 100);
  TEST_ASSERT_EQUAL_INT(2, rede.inicios());        // 2ª tentativa
  wifi.ciclo(t += 1000);                           // esgotou: espera 2 s
  wifi.ciclo(t += 1900);
  TEST_ASSERT_EQUAL_INT(2, rede.inicios());
  wifi.ciclo(t += 100);
  TEST_ASSERT_EQUAL_INT(3, rede.inicios());        // 3ª tentativa, depois de 2 s
  TEST_ASSERT_EQUAL_UINT32(4000, wifi.proximaEsperaMs());

  rede.definirConectada(true);
  TEST_ASSERT_TRUE(wifi.ciclo(t += 100));
  TEST_ASSERT_EQUAL_UINT32(ESPERA_MINIMA_WIFI_MS, wifi.proximaEsperaMs());  // sucesso: volta a 1 s
}

void test_wifi_que_cai_tenta_de_novo_na_hora_e_conta_a_queda() {
  RedeFalsa rede;
  ReconexaoWifi wifi(rede);
  wifi.ciclo(0);
  rede.definirConectada(true);
  wifi.ciclo(100);
  rede.definirConectada(false);  // o roteador caiu
  TEST_ASSERT_FALSE(wifi.ciclo(200));
  TEST_ASSERT_EQUAL_UINT32(1, wifi.quedas());
  TEST_ASSERT_EQUAL_INT(2, rede.inicios());
  TEST_ASSERT_EQUAL_UINT32(2, wifi.tentativas());
}

void test_wifi_que_volta_sozinho_durante_a_espera_e_aceito() {
  RedeFalsa rede;
  ReconexaoWifi wifi(rede, 1000);
  wifi.ciclo(0);
  wifi.ciclo(1000);  // esgotou: aguardando
  rede.definirConectada(true);
  TEST_ASSERT_TRUE(wifi.ciclo(1100));
  TEST_ASSERT_TRUE(wifi.estado() == EstadoWifi::Conectado);
}

void test_wifi_com_o_relogio_dando_a_volta() {
  RedeFalsa rede;
  ReconexaoWifi wifi(rede, 1000);
  const uint32_t quaseNoFim = UINT32_MAX - 500;
  wifi.ciclo(quaseNoFim);
  wifi.ciclo(quaseNoFim + 1000);  // passou do fim do contador de ms
  TEST_ASSERT_TRUE(wifi.estado() == EstadoWifi::Aguardando);
  wifi.ciclo(quaseNoFim + 1999);
  TEST_ASSERT_EQUAL_INT(1, rede.inicios());
  wifi.ciclo(quaseNoFim + 2000);
  TEST_ASSERT_EQUAL_INT(2, rede.inicios());
}

// ---------------------------------------------------------------- integração: coleta → buffer → JSON → servidor

/// Uma corrida de 120 amostras pelo caminho real do contrato (JSON nos dois sentidos), com a rede fora
/// do ar no meio e uma resposta perdida. O servidor termina com as 120, em ordem e sem duplicata, e a
/// corrida só é encerrada depois do último lote.
void test_integracao_corrida_pelo_json_com_rede_caindo_no_meio() {
  EnvioSimulado servidor;
  EnvioPorJson cliente(servidor);
  TravaNula trava;
  ControladorEnvio controlador(cliente, trava);
  controlador.iniciarCorrida(9);

  uint32_t t = 0;
  for (uint32_t s = 1; s <= 120; ++s) {
    const bool ultima = s == 120;
    const Pose pose{{static_cast<uint8_t>(s % 4), static_cast<uint8_t>((s / 4) % 4)}, Direcao::Norte};
    controlador.registrar(montarAmostra(pose, s * 1000, 100,
                                        ultima ? EstadoCorrida::Parado : EstadoCorrida::Explorando,
                                        ultima ? EventoTelemetria::ObjetivoAlcancado : EventoTelemetria::Nenhum));
    if (s == 40) {
      servidor.definirOffline(true);  // o Wi-Fi cai por 30 amostras
    }
    if (s == 70) {
      servidor.definirOffline(false);
      servidor.perderProximasRespostas(1);  // grava, mas a resposta não chega
    }
    for (int c = 0; c < 10; ++c) {  // 10 ciclos de rede de 100 ms por amostra
      controlador.ciclo(t);
      t += 100;
    }
  }
  controlador.solicitarEncerramento(t);
  for (int c = 0; c < 2000 && !controlador.encerrada(); ++c) {
    controlador.ciclo(t);
    t += 100;
  }

  TEST_ASSERT_TRUE(controlador.encerrada());
  TEST_ASSERT_EQUAL_UINT16(120, servidor.amostrasArmazenadas());
  for (uint16_t i = 0; i < 120; ++i) {
    TEST_ASSERT_EQUAL_UINT32(i + 1, servidor.armazenada(i).numeroSequencia);
    TEST_ASSERT_EQUAL_UINT32((i + 1) * 1000, servidor.armazenada(i).tempoDecorridoMs);
  }
  TEST_ASSERT_EQUAL_UINT16(120, servidor.amostrasAoEncerrar());  // encerrou com tudo entregue
  TEST_ASSERT_TRUE(servidor.duplicatasIgnoradas() >= 1);         // o reenvio da resposta perdida
  TEST_ASSERT_EQUAL_UINT32(0, controlador.amostrasPerdidas());
  TEST_ASSERT_EQUAL_UINT16(0, controlador.ocupacao());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_lote_com_uma_amostra_tem_os_nomes_das_colunas_e_as_unidades_do_banco);
  RUN_TEST(test_evento_so_aparece_quando_existe);
  RUN_TEST(test_lote_com_varias_amostras_em_ordem_e_json_valido);
  RUN_TEST(test_numeros_negativos_e_pequenos_com_tres_casas);
  RUN_TEST(test_lote_vazio_e_orientacao_invalida_nao_quebram_o_json);
  RUN_TEST(test_lote_que_nao_cabe_devolve_zero_e_texto_vazio);
  RUN_TEST(test_pior_amostra_e_pior_lote_cabem_no_espaco_reservado);
  RUN_TEST(test_contrato_lote_igual_ao_exemplo_do_backend);
  RUN_TEST(test_le_as_contagens_da_resposta_do_lote);
  RUN_TEST(test_resposta_do_lote_sem_as_contagens_e_recusada);
  RUN_TEST(test_le_o_id_da_corrida_criada);
  RUN_TEST(test_corpos_da_criacao_e_do_encerramento);
  RUN_TEST(test_amostra_montada_a_partir_da_pose);
  RUN_TEST(test_sem_credenciais_o_envio_fica_desligado);
  RUN_TEST(test_so_https_e_com_certificado_ou_excecao_de_bancada);
  RUN_TEST(test_url_das_funcoes);
  RUN_TEST(test_wifi_conecta_na_primeira_tentativa);
  RUN_TEST(test_wifi_espera_cada_vez_mais_entre_tentativas_e_volta_a_1s);
  RUN_TEST(test_wifi_que_cai_tenta_de_novo_na_hora_e_conta_a_queda);
  RUN_TEST(test_wifi_que_volta_sozinho_durante_a_espera_e_aceito);
  RUN_TEST(test_wifi_com_o_relogio_dando_a_volta);
  RUN_TEST(test_integracao_corrida_pelo_json_com_rede_caindo_no_meio);
  return UNITY_END();
}
