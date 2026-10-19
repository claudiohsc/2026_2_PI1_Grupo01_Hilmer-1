#include "TelemetriaJson.h"

#include <ArduinoJson.h>

namespace micromouse {

size_t TelemetriaJson::serializar(const PacoteTelemetria& pacote, char* saida, size_t capacidade) {
  if (saida == nullptr || capacidade == 0) {
    return 0;
  }
  saida[0] = '\0';

  JsonDocument doc;
  doc["esq_pulsos"] = pacote.pulsosEsq;
  doc["dir_pulsos"] = pacote.pulsosDir;
  doc["esq_mm"] = pacote.esqMm;
  doc["dir_mm"] = pacote.dirMm;
  doc["medio_mm"] = pacote.medioMm;
  doc["linha"] = pacote.pose.celula.linha;
  doc["coluna"] = pacote.pose.celula.coluna;

  const char orientacaoChar[2] = {letraDaDirecao(pacote.pose.direcao), '\0'};
  doc["orientacao"] = orientacaoChar;
  doc["pose_valida"] = pacote.poseValida;

  if (pacote.evento != nullptr) {
    doc["evento"] = pacote.evento;
  } else {
    doc["evento"] = nullptr;
  }

  if (measureJson(doc) >= capacidade) {
    return 0;
  }

  return serializeJson(doc, saida, capacidade);
}

}  // namespace micromouse
