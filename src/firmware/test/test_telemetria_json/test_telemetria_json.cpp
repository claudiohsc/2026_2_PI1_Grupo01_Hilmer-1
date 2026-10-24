/// @file test_telemetria_json.cpp
/// @brief Testes da serialização JSON do pacote de telemetria e inclusão do evento objetivo_alcancado (#189, HU-07),
/// dos campos de diagnóstico do ToF (#344), do resultado do boot (#50) e do pior caso no buffer de
/// `TAMANHO_MAX_JSON_TELEMETRIA`.
/// Roda no computador: `pio test -e native -f test_telemetria_json`.

#include <ArduinoJson.h>
#include <stdint.h>
#include <string.h>
#include <unity.h>

#include "TelemetriaJson.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr size_t CAPACIDADE = TAMANHO_MAX_JSON_TELEMETRIA;

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

void test_serializa_campos_de_diagnostico_do_tof() {
  PacoteTelemetria pacote;
  pacote.tofFrenteMm = 250;
  pacote.tofEsqMm = 40;
  pacote.tofDirMm = 8190;
  pacote.tofValida = false;
  pacote.tofIdadeMs = 33;

  char buffer[CAPACIDADE];
  TEST_ASSERT_GREATER_THAN_UINT32(0, TelemetriaJson::serializar(pacote, buffer, sizeof(buffer)));

  JsonDocument doc;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, buffer).code());
  TEST_ASSERT_EQUAL_UINT16(250, doc["tof_frente_mm"].as<uint16_t>());
  TEST_ASSERT_EQUAL_UINT16(40, doc["tof_esq_mm"].as<uint16_t>());
  TEST_ASSERT_EQUAL_UINT16(8190, doc["tof_dir_mm"].as<uint16_t>());
  TEST_ASSERT_FALSE(doc["tof_valida"].as<bool>());
  TEST_ASSERT_EQUAL_UINT32(33, doc["tof_idade_ms"].as<uint32_t>());
}

void test_serializa_resultado_do_boot() {
  PacoteTelemetria padrao;  // sem preencher: boot aprovado, sem falhas
  char buffer[CAPACIDADE];
  TEST_ASSERT_GREATER_THAN_UINT32(0, TelemetriaJson::serializar(padrao, buffer, sizeof(buffer)));
  JsonDocument doc;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, buffer).code());
  TEST_ASSERT_TRUE(doc["boot_ok"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("", doc["boot_falhas"].as<const char*>());
  TEST_ASSERT_EQUAL_UINT32(0, doc["boot_ms"].as<uint32_t>());

  PacoteTelemetria falhou;
  falhou.bootOk = false;
  falhou.falhasBoot = "tof_frontal,mpu6050";
  falhou.bootMs = 640;
  TEST_ASSERT_GREATER_THAN_UINT32(0, TelemetriaJson::serializar(falhou, buffer, sizeof(buffer)));
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, buffer).code());
  TEST_ASSERT_FALSE(doc["boot_ok"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("tof_frontal,mpu6050", doc["boot_falhas"].as<const char*>());
  TEST_ASSERT_EQUAL_UINT32(640, doc["boot_ms"].as<uint32_t>());
}

void test_pior_caso_cabe_em_tamanho_max_json_telemetria() {
  PacoteTelemetria pacote;
  pacote.numeroSequencia = UINT32_MAX;
  pacote.pose = Pose{{255, 255}, Direcao::Oeste};
  pacote.velocidade = -123456.789f;
  pacote.tensaoBateria = -123456.789f;
  pacote.evento = "objetivo_alcancado";
  pacote.pulsosEsq = INT32_MIN;
  pacote.pulsosDir = INT32_MIN;
  pacote.esqMm = INT32_MIN;
  pacote.dirMm = INT32_MIN;
  pacote.medioMm = INT32_MIN;
  pacote.poseValida = false;
  pacote.tofFrenteMm = UINT16_MAX;
  pacote.tofEsqMm = UINT16_MAX;
  pacote.tofDirMm = UINT16_MAX;
  pacote.tofValida = false;
  pacote.tofIdadeMs = UINT32_MAX;
  pacote.bootOk = false;
  pacote.falhasBoot = "tof_esquerdo,tof_frontal,tof_direito,mpu6050,tof_0x29";
  pacote.bootMs = UINT32_MAX;

  char buffer[TAMANHO_MAX_JSON_TELEMETRIA];
  const size_t bytes = TelemetriaJson::serializar(pacote, buffer, sizeof(buffer));
  TEST_ASSERT_GREATER_THAN_UINT32(0, bytes);
  // Os antigos buffers de 256 e de 512 bytes não comportam este caso: a linha seria descartada.
  TEST_ASSERT_GREATER_THAN_UINT32(512, bytes);
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
  RUN_TEST(test_serializa_campos_de_diagnostico_do_tof);
  RUN_TEST(test_serializa_resultado_do_boot);
  RUN_TEST(test_pior_caso_cabe_em_tamanho_max_json_telemetria);
  RUN_TEST(test_buffer_insuficiente_retorna_zero);
  RUN_TEST(test_buffer_nulo_ou_capacidade_zero);
  return UNITY_END();
}
