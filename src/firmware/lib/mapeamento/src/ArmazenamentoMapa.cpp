#include "ArmazenamentoMapa.h"

namespace micromouse {

void ArmazenamentoMapa::iniciarCorrida(DimensaoMapa dimensao) {
  mapa_ = Mapa(dimensao);
  corridaIniciada_ = true;
  mapaCongelado_ = false;
}

bool ArmazenamentoMapa::corridaIniciada() const {
  return corridaIniciada_;
}

Mapa& ArmazenamentoMapa::mapa() {
  return mapa_;
}

const Mapa& ArmazenamentoMapa::mapa() const {
  return mapa_;
}

bool ArmazenamentoMapa::congelarMapa() {
  if (!corridaIniciada_ || mapaCongelado_) {
    return false;
  }
  mapaCongelado_ = true;
  return true;
}

bool ArmazenamentoMapa::mapaCongelado() const {
  return mapaCongelado_;
}

const Mapa* ArmazenamentoMapa::mapaFinal() const {
  if (!mapaCongelado_) {
    return nullptr;
  }
  return &mapa_;
}

}  // namespace micromouse
