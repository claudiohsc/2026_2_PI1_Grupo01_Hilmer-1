#include "ClassificadorParede.h"

namespace micromouse {
  ClassificadorParede::ClassificadorParede(uint16_t limiar) {
    limiarDistanciaMm = limiar;
  }

  void ClassificadorParede::definirLimiar(uint16_t limiarMm) {
    limiarDistanciaMm = limiarMm;
  }

  uint16_t ClassificadorParede::obterLimiar() const {
    return limiarDistanciaMm;
  }

  ClassificadorParede::ResultadoClassificacao ClassificadorParede::classificar(const DistanciasLaterais *amostras, size_t qtd) const {
    ResultadoClassificacao resultado;
    if (qtd == 0) {
      resultado.frente = EstadoParede::Desconhecido;
      resultado.direita = EstadoParede::Desconhecido;
      resultado.esquerda = EstadoParede::Desconhecido;
      return resultado;
    }
    uint32_t somaDireita = 0;
    uint32_t somaEsquerda = 0;
    uint32_t somaFrente = 0;

    for (size_t i = 0; i < qtd; i++) {
      somaFrente += amostras[i].frenteMm;
      somaEsquerda += amostras[i].esquerdaMm;
      somaDireita += amostras[i].direitaMm;
    }

    resultado.direita = (somaDireita <= limiarDistanciaMm * qtd) ? EstadoParede::Parede : EstadoParede::Livre;
    resultado.esquerda = (somaEsquerda <= limiarDistanciaMm * qtd) ? EstadoParede::Parede : EstadoParede::Livre;
    resultado.frente = (somaFrente <= limiarDistanciaMm * qtd) ? EstadoParede::Parede : EstadoParede::Livre;

    return resultado;
  }
} // namespace micromouse