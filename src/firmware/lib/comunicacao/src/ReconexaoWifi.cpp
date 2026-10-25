#include "ReconexaoWifi.h"

namespace micromouse {

namespace {

/// `true` se `agoraMs` já alcançou `alvoMs`, correto mesmo quando o contador de ms dá a volta.
bool chegou(uint32_t agoraMs, uint32_t alvoMs) { return static_cast<int32_t>(agoraMs - alvoMs) >= 0; }

}  // namespace

ReconexaoWifi::ReconexaoWifi(IRedeWifi& rede, uint32_t tempoLimiteConexaoMs, const ConfiguracaoReenvio& esperas)
    : rede_(rede), tempoLimiteConexaoMs_(tempoLimiteConexaoMs), esperas_(esperas) {}

void ReconexaoWifi::comecarTentativa(uint32_t agoraMs) {
  rede_.iniciarConexao();
  ++tentativas_;
  estado_ = EstadoWifi::Conectando;
  instanteMs_ = agoraMs;
}

bool ReconexaoWifi::ciclo(uint32_t agoraMs) {
  switch (estado_) {
    case EstadoWifi::Conectado:
      if (rede_.conectada()) {
        return true;
      }
      ++quedas_;
      comecarTentativa(agoraMs);  // caiu: tenta de novo já
      return false;

    case EstadoWifi::Conectando:
      if (rede_.conectada()) {
        estado_ = EstadoWifi::Conectado;
        esperas_.registrarSucesso();
        return true;
      }
      if (static_cast<uint32_t>(agoraMs - instanteMs_) >= tempoLimiteConexaoMs_) {
        rede_.desconectar();
        estado_ = EstadoWifi::Aguardando;
        instanteMs_ = agoraMs + esperas_.registrarFalha();
      }
      return false;

    case EstadoWifi::Aguardando:
    default:
      if (rede_.conectada()) {  // conectou sozinho (reconexão automática do próprio Wi-Fi)
        estado_ = EstadoWifi::Conectado;
        esperas_.registrarSucesso();
        primeiraVez_ = false;
        return true;
      }
      if (primeiraVez_ || chegou(agoraMs, instanteMs_)) {
        primeiraVez_ = false;
        comecarTentativa(agoraMs);
      }
      return false;
  }
}

}  // namespace micromouse
