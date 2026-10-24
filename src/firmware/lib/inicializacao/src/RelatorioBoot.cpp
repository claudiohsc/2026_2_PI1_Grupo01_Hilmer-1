#include "RelatorioBoot.h"

#include <ArduinoJson.h>

namespace micromouse {

size_t RelatorioBoot::serializar(const ResultadoBoot& resultado, const AutotesteBoot& autoteste,
                                 uint32_t duracaoMs, char* saida, size_t capacidade) {
  if (saida == nullptr || capacidade == 0) {
    return 0;
  }
  saida[0] = '\0';

  char falhas[TAMANHO_MAX_FALHAS_BOOT];
  AutotesteBoot::descreverFalhas(resultado, falhas, sizeof(falhas));

  JsonDocument doc;
  doc["tipo"] = "boot";
  doc["ok"] = resultado.aprovado();
  doc["motores_liberados"] = resultado.aprovado();
  doc["falhas"] = falhas;
  doc["boot_ms"] = duracaoMs;
  doc["prazo_ms"] = autoteste.tempoMaximoMs();
  doc["no_prazo"] = autoteste.dentroDoPrazo(duracaoMs);

  JsonObject dispositivos = doc["dispositivos"].to<JsonObject>();
  for (uint8_t i = 0; i < QUANTIDADE_DISPOSITIVOS; i++) {
    dispositivos[nomeDispositivo(static_cast<Dispositivo>(i))] = nomeEstado(resultado.dispositivos[i]);
  }

  doc["tof_0x29"] = resultado.tofNoEnderecoDeFabrica;
  if (resultado.whoAmILido) {
    doc["mpu6050_who_am_i"] = resultado.whoAmIMpu6050;
  } else {
    doc["mpu6050_who_am_i"] = nullptr;
  }

  JsonArray inesperados = doc["inesperados"].to<JsonArray>();
  const uint8_t guardados = resultado.quantidadeInesperados < MAXIMO_ENDERECOS_INESPERADOS
                                ? resultado.quantidadeInesperados
                                : MAXIMO_ENDERECOS_INESPERADOS;
  for (uint8_t i = 0; i < guardados; i++) {
    inesperados.add(resultado.inesperados[i]);
  }
  doc["inesperados_total"] = resultado.quantidadeInesperados;

  if (measureJson(doc) >= capacidade) {
    return 0;
  }
  return serializeJson(doc, saida, capacidade);
}

}  // namespace micromouse
