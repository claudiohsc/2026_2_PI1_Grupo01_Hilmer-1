/// @file test_telemetria_json.cpp
/// @brief Testes da serialização JSON do pacote de telemetria e inclusão do evento objetivo_alcancado (#189, HU-07).
/// Roda no computador: `pio test -e native -f test_telemetria_json`.

#include <ArduinoJson.h>
#include <string.h>
#include <unity.h>

#include "TelemetriaJson.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr size_t CAPACIDADE = 256;

}  // namespace

void test_serializa_telemetria_padrao_sem_evento() {
  PacoteTelemetria pacote;
  pacote.numeroSequencia = 42;
  pacote.pulsosEsq = 1200;
  pacote.pulsosDir = 1205;
  pacote.esqMm = 300;
  pacote.dirMm = 301;
  pacote.medioMm = 300;
  pacote.pose = Pose{{2, 1}, Direcao::Leste};
  pacote.poseValida = true;
  pacote.velocidade = 0.25f;
  pacote.tensaoBateria = 7.4f;
  pacote.evento = nullptr;

  char buffer[CAPACIDADE];
  const size_t bytes = TelemetriaJson::serializar(pacote, buffer, sizeof(buffer));
  TEST_ASSERT_GREATER_THAN_UINT32(0, bytes);

  JsonDocument doc;
  const DeserializationError erro = deserializeJson(doc, buffer);
  TEST_ASSERT_EQUAL(DeserializationError::Ok, erro.code());

  // Campos mapeados para a tabela telemetria do Supabase
  TEST_ASSERT_EQUAL_UINT32(42, doc["numero_sequencia"].as<uint32_t>());
  TEST_ASSERT_EQUAL_UINT8(2, doc["posicao_linha"].as<uint8_t>());
  TEST_ASSERT_EQUAL_UINT8(1, doc["posicao_coluna"].as<uint8_t>());
  TEST_ASSERT_EQUAL_STRING("L", doc["orientacao"].as<const char*>());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.25f, doc["velocidade"].as<float>());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 7.4f, doc["tensao_bateria"].as<float>());
  TEST_ASSERT_TRUE(doc["evento"].isNull());

  // Campos de diagnóstico de encoders e pose
  TEST_ASSERT_EQUAL_INT32(1200, doc["esq_pulsos"].as<int32_t>());
  TEST_ASSERT_EQUAL_INT32(1205, doc["dir_pulsos"].as<int32_t>());
  TEST_ASSERT_EQUAL_INT32(300, doc["esq_mm"].as<int32_t>());
  TEST_ASSERT_EQUAL_INT32(301, doc["dir_mm"].as<int32_t>());
  TEST_ASSERT_EQUAL_INT32(300, doc["medio_mm"].as<int32_t>());
  TEST_ASSERT_TRUE(doc["pose_valida"].as<bool>());
}

void test_serializa_telemetria_com_evento_objetivo_alcancado() {
  PacoteTelemetria pacote;
  pacote.numeroSequencia = 100;
  pacote.pulsosEsq = 4320;
  pacote.pulsosDir = 4320;
  pacote.esqMm = 1080;
  pacote.dirMm = 1080;
  pacote.medioMm = 1080;
  pacote.pose = Pose{{3, 3}, Direcao::Norte};
  pacote.poseValida = true;
  pacote.velocidade = 0.0f;
  pacote.tensaoBateria = 7.3f;
  pacote.evento = "objetivo_alcancado";

  char buffer[CAPACIDADE];
  const size_t bytes = TelemetriaJson::serializar(pacote, buffer, sizeof(buffer));
  TEST_ASSERT_GREATER_THAN_UINT32(0, bytes);

  JsonDocument doc;
  const DeserializationError erro = deserializeJson(doc, buffer);
  TEST_ASSERT_EQUAL(DeserializationError::Ok, erro.code());

  TEST_ASSERT_EQUAL_UINT32(100, doc["numero_sequencia"].as<uint32_t>());
  TEST_ASSERT_EQUAL_UINT8(3, doc["posicao_linha"].as<uint8_t>());
  TEST_ASSERT_EQUAL_UINT8(3, doc["posicao_coluna"].as<uint8_t>());
  TEST_ASSERT_EQUAL_STRING("N", doc["orientacao"].as<const char*>());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, doc["velocidade"].as<float>());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 7.3f, doc["tensao_bateria"].as<float>());
  TEST_ASSERT_FALSE(doc["evento"].isNull());
  TEST_ASSERT_EQUAL_STRING("objetivo_alcancado", doc["evento"].as<const char*>());
}

void test_buffer_insuficiente_retorna_zero() {
  PacoteTelemetria pacote;
  char bufferPequeno[10];

  const size_t bytes = TelemetriaJson::serializar(pacote, bufferPequeno, sizeof(bufferPequeno));
  TEST_ASSERT_EQUAL_UINT32(0, bytes);
  TEST_ASSERT_EQUAL_CHAR('\0', bufferPequeno[0]);
}

void test_buffer_nulo_ou_capacidade_zero() {
  PacoteTelemetria pacote;
  TEST_ASSERT_EQUAL_UINT32(0, TelemetriaJson::serializar(pacote, nullptr, 100));

  char buffer[10];
  TEST_ASSERT_EQUAL_UINT32(0, TelemetriaJson::serializar(pacote, buffer, 0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_serializa_telemetria_padrao_sem_evento);
  RUN_TEST(test_serializa_telemetria_com_evento_objetivo_alcancado);
  RUN_TEST(test_buffer_insuficiente_retorna_zero);
  RUN_TEST(test_buffer_nulo_ou_capacidade_zero);
  return UNITY_END();
}
