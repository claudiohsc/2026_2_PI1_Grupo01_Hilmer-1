#pragma once

/// @file RedeWifiEsp32.h
/// @brief `IRedeWifi` real: o Wi-Fi do ESP32-C3 em modo estação (#238).

#include <Arduino.h>
#include <WiFi.h>

#include "IRedeWifi.h"

namespace micromouse {

/// Wi-Fi do ESP32 em modo estação. A reconexão automática do próprio Wi-Fi fica desligada: quem
/// decide quando tentar de novo, com espera crescente, é a `ReconexaoWifi`.
class RedeWifiEsp32 : public IRedeWifi {
 public:
  /// @param ssid Rede Wi-Fi.
  /// @param senha Senha (vazia numa rede aberta).
  RedeWifiEsp32(const char* ssid, const char* senha) : ssid_(ssid), senha_(senha) {}

  /// Põe o Wi-Fi em modo estação, sem gravar a rede na flash. Chame uma vez, na tarefa de rede.
  void iniciar() {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
  }

  bool conectada() override { return WiFi.status() == WL_CONNECTED; }
  void iniciarConexao() override { WiFi.begin(ssid_, senha_); }
  void desconectar() override { WiFi.disconnect(); }

 private:
  const char* ssid_;
  const char* senha_;
};

}  // namespace micromouse
