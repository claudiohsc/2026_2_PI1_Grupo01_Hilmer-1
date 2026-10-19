#pragma once

/// @file TelemetriaJson.h
/// @brief Serialização dos pacotes de telemetria periódicos em JSON para a transmissão serial/Wi-Fi e banco de dados.

#include <stddef.h>
#include <stdint.h>

#include "Direcao.h"
#include "Tipos.h"

namespace micromouse {

/// Dados reportados na telemetria periódica do robô.
struct PacoteTelemetria {
  int32_t pulsosEsq = 0;
  int32_t pulsosDir = 0;
  int32_t esqMm = 0;
  int32_t dirMm = 0;
  int32_t medioMm = 0;
  Pose pose = {{0, 0}, Direcao::Norte};
  bool poseValida = true;
  const char* evento = nullptr;  ///< Evento pontual ("objetivo_alcancado", "colisao" ou nullptr)
};

/// Serializador de dados de telemetria em formato JSON compatível com a tabela `telemetria` e o dashboard web.
class TelemetriaJson {
 public:
  /// Serializa o pacote em cadeia de caracteres JSON em `saida`, terminada com `\0`.
  /// @param pacote Dados da telemetria a serializar.
  /// @param saida Buffer de destino.
  /// @param capacidade Tamanho do buffer em bytes.
  /// @return Quantidade de bytes gravados (sem o `\0`), ou `0` se não couber ou buffer inválido.
  static size_t serializar(const PacoteTelemetria& pacote, char* saida, size_t capacidade);
};

}  // namespace micromouse
