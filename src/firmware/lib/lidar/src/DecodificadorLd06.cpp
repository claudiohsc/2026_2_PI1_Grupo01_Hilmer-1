#include "DecodificadorLd06.h"

namespace micromouse {

namespace {

constexpr uint8_t POLINOMIO_CRC8 = 0x4D;
constexpr uint32_t VOLTA_CENTI_GRAUS = 36000;

uint16_t lerUint16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }

}  // namespace

uint8_t DecodificadorLd06::calcularCrc8(const uint8_t* dados, uint8_t tamanho) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < tamanho; i++) {
    crc ^= dados[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ POLINOMIO_CRC8)
                         : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc;
}

bool DecodificadorLd06::alimentar(uint8_t byte) {
  if (recebidos_ == 0 && byte != CABECALHO) {
    return false;  // fora de um pacote: espera o cabeçalho
  }
  buffer_[recebidos_++] = byte;
  if (recebidos_ == 2 && buffer_[1] != TIPO_E_QUANTIDADE) {
    ressincronizar();
    return false;
  }
  if (recebidos_ < TAMANHO_PACOTE) {
    return false;
  }
  // Pacote completo.
  if (calcularCrc8(buffer_, TAMANHO_PACOTE - 1) != buffer_[TAMANHO_PACOTE - 1]) {
    ressincronizar();
    return false;
  }
  const bool valido = extrairPontos();
  recebidos_ = 0;
  return valido;
}

PontoLidar DecodificadorLd06::ponto(uint8_t indice) const {
  return indice < PONTOS_POR_PACOTE ? pontos_[indice] : PontoLidar{0, 0};
}

/// Descarta o primeiro byte do buffer e tudo até o próximo cabeçalho, repetindo enquanto o
/// segundo byte restante não for o esperado. Assim, um pacote que começa no meio de bytes ruins
/// não se perde.
void DecodificadorLd06::ressincronizar() {
  do {
    uint8_t i = 1;
    while (i < recebidos_ && buffer_[i] != CABECALHO) {
      i++;
    }
    const uint8_t restantes = recebidos_ - i;
    for (uint8_t k = 0; k < restantes; k++) {
      buffer_[k] = buffer_[i + k];
    }
    recebidos_ = restantes;
  } while (recebidos_ >= 2 && buffer_[1] != TIPO_E_QUANTIDADE);
}

bool DecodificadorLd06::extrairPontos() {
  const uint32_t inicio = lerUint16(&buffer_[4]);
  uint32_t fim = lerUint16(&buffer_[42]);
  if (inicio >= VOLTA_CENTI_GRAUS || fim >= VOLTA_CENTI_GRAUS) {
    return false;
  }
  if (fim < inicio) {
    fim += VOLTA_CENTI_GRAUS;  // o pacote cruza o 0°
  }
  for (uint8_t i = 0; i < PONTOS_POR_PACOTE; i++) {
    const uint8_t* dados = &buffer_[6 + 3 * i];
    const uint32_t angulo = (inicio + (fim - inicio) * i / (PONTOS_POR_PACOTE - 1)) % VOLTA_CENTI_GRAUS;
    pontos_[i].anguloCentiGraus = static_cast<uint16_t>(angulo);
    pontos_[i].distanciaMm = lerUint16(dados);
  }
  return true;
}

}  // namespace micromouse
