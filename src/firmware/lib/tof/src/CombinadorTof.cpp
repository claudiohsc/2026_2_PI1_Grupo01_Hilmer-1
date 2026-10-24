#include "CombinadorTof.h"

namespace micromouse {

CombinadorTof::CombinadorTof(const ConfiguracaoTof& config) : config_(config) {}

EstadoAmostraTof CombinadorTof::classificar(uint16_t distanciaMm, bool falhou) const {
  if (falhou) {
    return EstadoAmostraTof::Falha;
  }
  if (distanciaMm < config_.distanciaMinimaMm) {
    return EstadoAmostraTof::AbaixoDoMinimo;
  }
  if (distanciaMm > config_.distanciaMaximaMm) {
    return EstadoAmostraTof::ForaDeAlcance;
  }
  return EstadoAmostraTof::Valida;
}

void CombinadorTof::registrar(Lado lado, uint16_t distanciaMm, bool falhou, uint32_t agoraMs) {
  AmostraTof& amostra = amostras_[indice(lado)];
  amostra.distanciaMm = distanciaMm;
  amostra.estado = classificar(distanciaMm, falhou);
  amostra.instanteMs = agoraMs;
}

const AmostraTof& CombinadorTof::amostra(Lado lado) const { return amostras_[indice(lado)]; }

bool CombinadorTof::distancias(uint32_t agoraMs, DistanciasLaterais& saida) const {
  for (const AmostraTof& amostra : amostras_) {
    if (amostra.estado == EstadoAmostraTof::SemAmostra || amostra.estado == EstadoAmostraTof::Falha) {
      return false;
    }
  }
  if (idadeMs(agoraMs) > config_.idadeMaximaLeituraMs) {
    return false;
  }
  saida.frenteMm = distanciaLimitada(amostra(Lado::Frente));
  saida.esquerdaMm = distanciaLimitada(amostra(Lado::Esquerda));
  saida.direitaMm = distanciaLimitada(amostra(Lado::Direita));
  return true;
}

uint16_t CombinadorTof::distanciaLimitada(const AmostraTof& amostra) const {
  if (amostra.estado == EstadoAmostraTof::AbaixoDoMinimo) {
    return config_.distanciaMinimaMm;
  }
  if (amostra.estado == EstadoAmostraTof::ForaDeAlcance) {
    return config_.distanciaMaximaMm;
  }
  return amostra.distanciaMm;
}

uint32_t CombinadorTof::idadeMs(uint32_t agoraMs) const {
  uint32_t maisVelha = 0;
  for (const AmostraTof& amostra : amostras_) {
    if (amostra.estado == EstadoAmostraTof::SemAmostra) {
      return IDADE_DESCONHECIDA;
    }
    // A subtração sem sinal continua correta quando o contador de ms dá a volta (~49 dias).
    const uint32_t idade = agoraMs - amostra.instanteMs;
    if (idade > maisVelha) {
      maisVelha = idade;
    }
  }
  return maisVelha;
}

void CombinadorTof::reiniciar() {
  for (AmostraTof& amostra : amostras_) {
    amostra = AmostraTof{};
  }
}

}  // namespace micromouse
