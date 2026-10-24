#include "ClassificadorParede.h"

namespace micromouse {

ClassificadorParede::ClassificadorParede() : config_() {}

ClassificadorParede::ClassificadorParede(uint16_t limiarMm) : config_() {
  definirLimiar(limiarMm);
}

ClassificadorParede::ClassificadorParede(const ConfiguracaoPercepcao& config) : config_(config) {}

ClassificadorParede::ClassificadorParede(uint16_t limiarFrenteMm,
                                         uint16_t limiarEsquerdaMm,
                                         uint16_t limiarDireitaMm)
    : config_() {
  definirLimiares(limiarFrenteMm, limiarEsquerdaMm, limiarDireitaMm);
}

void ClassificadorParede::definirLimiar(uint16_t novoLimiarMm) {
  config_.limiarFrenteMm = novoLimiarMm;
  config_.limiarEsquerdaMm = novoLimiarMm;
  config_.limiarDireitaMm = novoLimiarMm;
}

void ClassificadorParede::definirLimiar(Lado lado, uint16_t novoLimiarMm) {
  switch (lado) {
    case Lado::Frente:
      config_.limiarFrenteMm = novoLimiarMm;
      break;
    case Lado::Esquerda:
      config_.limiarEsquerdaMm = novoLimiarMm;
      break;
    case Lado::Direita:
      config_.limiarDireitaMm = novoLimiarMm;
      break;
  }
}

void ClassificadorParede::definirLimiares(uint16_t frenteMm,
                                          uint16_t esquerdaMm,
                                          uint16_t direitaMm) {
  config_.limiarFrenteMm = frenteMm;
  config_.limiarEsquerdaMm = esquerdaMm;
  config_.limiarDireitaMm = direitaMm;
}

uint16_t ClassificadorParede::obterLimiar() const {
  return config_.limiarFrenteMm;
}

uint16_t ClassificadorParede::obterLimiar(Lado lado) const {
  switch (lado) {
    case Lado::Frente:
      return config_.limiarFrenteMm;
    case Lado::Esquerda:
      return config_.limiarEsquerdaMm;
    case Lado::Direita:
      return config_.limiarDireitaMm;
  }
  return config_.limiarFrenteMm;
}

const ConfiguracaoPercepcao& ClassificadorParede::obterConfiguracao() const {
  return config_;
}

void ClassificadorParede::definirConfiguracao(const ConfiguracaoPercepcao& config) {
  config_ = config;
}

uint16_t ClassificadorParede::tratarDistancia(uint16_t distanciaBrutaMm) const {
  if (distanciaBrutaMm < config_.distanciaMinimaMm) {
    return config_.distanciaMinimaMm;
  }
  if (distanciaBrutaMm > config_.distanciaMaximaMm) {
    return config_.distanciaMaximaMm;
  }
  return distanciaBrutaMm;
}

bool ClassificadorParede::ehAbaixoDoMinimo(uint16_t distanciaMm) const {
  return distanciaMm < config_.distanciaMinimaMm;
}

bool ClassificadorParede::ehForaDeAlcance(uint16_t distanciaMm) const {
  return distanciaMm > config_.distanciaMaximaMm;
}

EstadoParede ClassificadorParede::classificarDistancia(Lado lado, uint16_t distanciaMm) const {
  const uint16_t tratada = tratarDistancia(distanciaMm);
  const uint16_t limiar = obterLimiar(lado);
  return (tratada <= limiar) ? EstadoParede::Parede : EstadoParede::Livre;
}

ClassificadorParede::ResultadoClassificacao ClassificadorParede::classificar(
    const DistanciasLaterais& leitura) const {
  return classificar(&leitura, 1);
}

uint16_t ClassificadorParede::calcularMediana(uint16_t* valores, size_t qtd) {
  if (qtd == 0) {
    return 0;
  }
  for (size_t i = 1; i < qtd; i++) {
    const uint16_t chave = valores[i];
    size_t j = i;
    while (j > 0 && valores[j - 1] > chave) {
      valores[j] = valores[j - 1];
      j--;
    }
    valores[j] = chave;
  }
  if (qtd % 2 != 0) {
    return valores[qtd / 2];
  }
  return static_cast<uint16_t>(
      (static_cast<uint32_t>(valores[qtd / 2 - 1]) + valores[qtd / 2]) / 2);
}

uint16_t ClassificadorParede::calcularMedia(const uint16_t* valores, size_t qtd) {
  if (qtd == 0) {
    return 0;
  }
  uint32_t soma = 0;
  for (size_t i = 0; i < qtd; i++) {
    soma += valores[i];
  }
  return static_cast<uint16_t>(soma / qtd);
}

uint16_t ClassificadorParede::filtrarAmostras(const uint16_t* valores, size_t quantidade) const {
  if (quantidade == 0 || valores == nullptr) {
    return 0;
  }
  const size_t n = (quantidade > MAX_AMOSTRAS_FILTRO_PERCEPCAO)
                       ? MAX_AMOSTRAS_FILTRO_PERCEPCAO
                       : quantidade;
  const size_t offsetInicio = quantidade - n;

  uint16_t buffer[MAX_AMOSTRAS_FILTRO_PERCEPCAO];
  for (size_t i = 0; i < n; i++) {
    buffer[i] = tratarDistancia(valores[offsetInicio + i]);
  }

  if (config_.tipoFiltro == TipoFiltroPercepcao::Mediana) {
    return calcularMediana(buffer, n);
  }
  return calcularMedia(buffer, n);
}

ClassificadorParede::ResultadoClassificacao ClassificadorParede::classificar(
    const DistanciasLaterais* amostras, size_t quantidade) const {
  ResultadoClassificacao resultado;
  if (quantidade == 0 || amostras == nullptr) {
    resultado.frente = EstadoParede::Desconhecido;
    resultado.direita = EstadoParede::Desconhecido;
    resultado.esquerda = EstadoParede::Desconhecido;
    return resultado;
  }

  const size_t n = (quantidade > MAX_AMOSTRAS_FILTRO_PERCEPCAO)
                       ? MAX_AMOSTRAS_FILTRO_PERCEPCAO
                       : quantidade;
  const size_t offsetInicio = quantidade - n;

  uint16_t bufferFrente[MAX_AMOSTRAS_FILTRO_PERCEPCAO];
  uint16_t bufferEsquerda[MAX_AMOSTRAS_FILTRO_PERCEPCAO];
  uint16_t bufferDireita[MAX_AMOSTRAS_FILTRO_PERCEPCAO];

  for (size_t i = 0; i < n; i++) {
    bufferFrente[i] = amostras[offsetInicio + i].frenteMm;
    bufferEsquerda[i] = amostras[offsetInicio + i].esquerdaMm;
    bufferDireita[i] = amostras[offsetInicio + i].direitaMm;
  }

  const uint16_t frenteFiltrada = filtrarAmostras(bufferFrente, n);
  const uint16_t esquerdaFiltrada = filtrarAmostras(bufferEsquerda, n);
  const uint16_t direitaFiltrada = filtrarAmostras(bufferDireita, n);

  resultado.frente = (frenteFiltrada <= config_.limiarFrenteMm)
                         ? EstadoParede::Parede
                         : EstadoParede::Livre;
  resultado.esquerda = (esquerdaFiltrada <= config_.limiarEsquerdaMm)
                           ? EstadoParede::Parede
                           : EstadoParede::Livre;
  resultado.direita = (direitaFiltrada <= config_.limiarDireitaMm)
                          ? EstadoParede::Parede
                          : EstadoParede::Livre;

  return resultado;
}

}  // namespace micromouse
