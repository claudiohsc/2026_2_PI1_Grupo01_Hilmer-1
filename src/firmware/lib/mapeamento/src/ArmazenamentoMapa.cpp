#include "ArmazenamentoMapa.h"

namespace micromouse {

void ArmazenamentoMapa::iniciarCorrida(DimensaoMapa dimensao) {
  mapa_ = Mapa(dimensao);
  corridaIniciada_ = true;
  mapaCongelado_ = false;
}

bool ArmazenamentoMapa::congelarMapa() {
  if (!corridaIniciada_ || mapaCongelado_) {
    return false;
  }
  mapaFinal_ = mapa_;
  mapaCongelado_ = true;
  return true;
}

const Mapa* ArmazenamentoMapa::mapaFinal() const {
  if (!mapaCongelado_) {
    return nullptr;
  }
  return &mapaFinal_;
}

}  // namespace micromouse
