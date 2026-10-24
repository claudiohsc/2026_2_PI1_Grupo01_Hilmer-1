#include "EstimadorDeslocamento.h"

namespace micromouse {

namespace {

/// Divisão inteira arredondada para baixo (a de C++ arredonda para o zero).
int64_t divisaoPiso(int64_t numerador, int64_t denominador) {
  int64_t quociente = numerador / denominador;
  if ((numerador % denominador != 0) && ((numerador < 0) != (denominador < 0))) {
    quociente--;
  }
  return quociente;
}

}  // namespace

EstimadorDeslocamento::EstimadorDeslocamento(const ConfiguracaoDeslocamento& config)
    : config_(config) {}

int32_t EstimadorDeslocamento::velocidadeMmPorS(int16_t potencia) const {
  if (config_.tabela == nullptr || config_.pontosTabela == 0) {
    return 0;
  }
  const int32_t sinal = potencia < 0 ? -1 : 1;
  const int32_t modulo = potencia < 0 ? -static_cast<int32_t>(potencia) : potencia;
  const PontoVelocidade* tabela = config_.tabela;

  if (modulo < tabela[0].potencia) {
    return 0;  // zona morta
  }
  for (uint8_t i = 1; i < config_.pontosTabela; i++) {
    if (modulo <= tabela[i].potencia) {
      const int32_t p0 = tabela[i - 1].potencia;
      const int32_t p1 = tabela[i].potencia;
      const int32_t v0 = tabela[i - 1].velocidadeMmPorS;
      const int32_t v1 = tabela[i].velocidadeMmPorS;
      return sinal * (v0 + (v1 - v0) * (modulo - p0) / (p1 - p0));
    }
  }
  return sinal * static_cast<int32_t>(tabela[config_.pontosTabela - 1].velocidadeMmPorS);
}

void EstimadorDeslocamento::comandar(const PotenciaRodas& potencia) {
  potencia_ = {limitarPotencia(potencia.esquerda), limitarPotencia(potencia.direita)};
}

void EstimadorDeslocamento::atualizar(uint32_t dtMs) {
  // mm/s × ms = µm.
  const int64_t esquerdaUm = static_cast<int64_t>(velocidadeMmPorS(potencia_.esquerda)) * dtMs;
  const int64_t direitaUm = static_cast<int64_t>(velocidadeMmPorS(potencia_.direita)) * dtMs;
  posicaoUm_[indice(Roda::Esquerda)] += esquerdaUm;
  posicaoUm_[indice(Roda::Direita)] += direitaUm;

  // Só o avanço do centro conta para a confiança: um giro no lugar não desloca o robô.
  const int64_t avancoUm = (esquerdaUm + direitaUm) / 2;
  semCorrecaoUm_ += static_cast<uint64_t>(avancoUm < 0 ? -avancoUm : avancoUm);
}

void EstimadorDeslocamento::corrigirAvanco(int32_t deltaUm) {
  posicaoUm_[indice(Roda::Esquerda)] += deltaUm;
  posicaoUm_[indice(Roda::Direita)] += deltaUm;
  semCorrecaoUm_ = 0;
}

int32_t EstimadorDeslocamento::contagem(Roda roda) const {
  return static_cast<int32_t>(divisaoPiso(posicaoUm_[indice(roda)], config_.micrometrosPorPulso));
}

void EstimadorDeslocamento::zerar(Roda roda) { posicaoUm_[indice(roda)] = 0; }

bool EstimadorDeslocamento::girando() const {
  return (potencia_.esquerda > 0 && potencia_.direita < 0) ||
         (potencia_.esquerda < 0 && potencia_.direita > 0);
}

}  // namespace micromouse
