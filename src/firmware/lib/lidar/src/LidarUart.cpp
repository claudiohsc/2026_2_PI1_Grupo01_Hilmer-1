#include "LidarUart.h"

namespace micromouse {

LidarUart::LidarUart(IFonteBytes& fonte, IDecodificadorLidar& decodificador,
                     FuncaoRelogioMs agoraMs, const ConfiguracaoLidar& config)
    : fonte_(fonte),
      decodificador_(decodificador),
      agoraMs_(agoraMs),
      config_(config),
      agregador_(config) {}

bool LidarUart::lerDistanciasLaterais(DistanciasLaterais& saida) {
  processarBytesDisponiveis();
  if (!temVolta_) {
    return false;
  }
  // A subtração sem sinal continua correta quando o contador de ms dá a volta (~49 dias).
  if (agora() - instanteUltimaVoltaMs_ > config_.idadeMaximaLeituraMs) {
    return false;
  }
  if (!ultimaVoltaValida_) {
    return false;
  }
  saida = ultimaLeitura_;
  return true;
}

void LidarUart::processarBytesDisponiveis() {
  uint16_t lidos = 0;
  uint8_t byte = 0;
  while (lidos < config_.maxBytesPorChamada && fonte_.lerByte(byte)) {
    lidos++;
    if (!decodificador_.alimentar(byte)) {
      continue;
    }
    for (uint8_t i = 0; i < decodificador_.quantidadePontos(); i++) {
      if (agregador_.adicionarPonto(decodificador_.ponto(i))) {
        temVolta_ = true;
        instanteUltimaVoltaMs_ = agora();
        ultimaVoltaValida_ = agregador_.voltaValida();
        if (ultimaVoltaValida_) {
          ultimaLeitura_ = agregador_.distancias();
        }
      }
    }
  }
}

}  // namespace micromouse
