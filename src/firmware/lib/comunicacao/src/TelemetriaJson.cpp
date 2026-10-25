#include "TelemetriaJson.h"

#include <ArduinoJson.h>

namespace micromouse {

size_t TelemetriaJson::serializar(const PacoteTelemetria& pacote, char* saida, size_t capacidade) {
  if (saida == nullptr || capacidade == 0) {
    return 0;
  }
  saida[0] = '\0';

  JsonDocument doc;
  // Campos mapeados 1:1 com a tabela `telemetria` do Supabase:
  doc["numero_sequencia"] = pacote.numeroSequencia;
  doc["posicao_linha"] = pacote.pose.celula.linha;
  doc["posicao_coluna"] = pacote.pose.celula.coluna;

  const char orientacaoChar[2] = {letraDaDirecao(pacote.pose.direcao), '\0'};
  doc["orientacao"] = orientacaoChar;
  doc["velocidade"] = pacote.velocidade;
  doc["tensao_bateria"] = pacote.tensaoBateria;

  if (pacote.evento != nullptr) {
    doc["evento"] = pacote.evento;
  } else {
    doc["evento"] = nullptr;
  }

  // Campos diagnósticos de encoders e calibração de bancada:
  doc["esq_pulsos"] = pacote.pulsosEsq;
  doc["dir_pulsos"] = pacote.pulsosDir;
  doc["esq_mm"] = pacote.esqMm;
  doc["dir_mm"] = pacote.dirMm;
  doc["medio_mm"] = pacote.medioMm;
  doc["pose_valida"] = pacote.poseValida;

  // Campos diagnósticos dos sensores de distância (#344):
  doc["tof_frente_mm"] = pacote.tofFrenteMm;
  doc["tof_esq_mm"] = pacote.tofEsqMm;
  doc["tof_dir_mm"] = pacote.tofDirMm;
  doc["tof_valida"] = pacote.tofValida;
  doc["tof_idade_ms"] = pacote.tofIdadeMs;

  // Campos diagnósticos do deslocamento sem encoder (#380):
  doc["deslocamento"] = pacote.modoDeslocamento != nullptr ? pacote.modoDeslocamento : "encoder";
  doc["sem_correcao_mm"] = pacote.semCorrecaoMm;

  // Resultado do autoteste do boot (#50):
  doc["boot_ok"] = pacote.bootOk;
  doc["boot_falhas"] = pacote.falhasBoot != nullptr ? pacote.falhasBoot : "";
  doc["boot_ms"] = pacote.bootMs;
  doc["fase"] = pacote.fase != nullptr ? pacote.fase : "";

  if (measureJson(doc) >= capacidade) {
    return 0;
  }

  return serializeJson(doc, saida, capacidade);
}

}  // namespace micromouse
