#include "VigiaTof.h"

namespace micromouse {

LeituraI2c VigiaTof::avaliar(Lado lado, const AmostraTof& amostra, uint32_t agoraMs) {
  Acompanhamento& a = acompanhamentos_[static_cast<uint8_t>(lado)];
  if (!a.comecou) {
    a.comecou = true;
    a.referenciaMs = agoraMs;
  }

  // Diferença com sinal: continua certa quando o `millis()` dá a volta.
  const bool depoisDoReinicio = !a.reiniciado || static_cast<int32_t>(amostra.instanteMs - a.reinicioMs) >= 0;
  const bool nova = amostra.estado != EstadoAmostraTof::SemAmostra && depoisDoReinicio &&
                    (!a.viuAmostra || amostra.instanteMs != a.ultimoInstanteMs);
  if (nova) {
    a.viuAmostra = true;
    a.ultimoInstanteMs = amostra.instanteMs;
    a.referenciaMs = agoraMs;
    return amostra.estado == EstadoAmostraTof::Falha ? LeituraI2c::Falha : LeituraI2c::Ok;
  }
  if (agoraMs - a.referenciaMs > idadeMaximaMs_) {
    a.referenciaMs = agoraMs;  // conta de novo: uma falha a cada `idadeMaximaMs` sem medida
    return LeituraI2c::Falha;
  }
  return LeituraI2c::SemNovidade;
}

void VigiaTof::sensorReiniciado(Lado lado, uint32_t agoraMs) {
  Acompanhamento& a = acompanhamentos_[static_cast<uint8_t>(lado)];
  a.comecou = true;
  a.reiniciado = true;
  a.reinicioMs = agoraMs;
  a.referenciaMs = agoraMs;
}

}  // namespace micromouse
