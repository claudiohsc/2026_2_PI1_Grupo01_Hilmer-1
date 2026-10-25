#include "DetectorColisao.h"

#include "ConfiguracaoTof.h"

namespace micromouse {

namespace {

/// Raiz quadrada inteira por baixo (maior `r` com `r * r <= valor`).
uint32_t raizInteira(uint64_t valor) {
  uint64_t r = 0;
  uint64_t bit = static_cast<uint64_t>(1) << 62;
  while (bit > valor) {
    bit >>= 2;
  }
  while (bit != 0) {
    if (valor >= r + bit) {
      valor -= r + bit;
      r = (r >> 1) + bit;
    } else {
      r >>= 1;
    }
    bit >>= 2;
  }
  return static_cast<uint32_t>(r);
}

}  // namespace

DetectorColisao::DetectorColisao(const ConfiguracaoColisao& config, const ConfiguracaoProtecaoMotor& protecao)
    : config_(config), protecao_(protecao) {
  if (config_.amostrasConfirmacao == 0) {
    config_.amostrasConfirmacao = 1;
  }
}

void DetectorColisao::reiniciar() {
  protecao_.reiniciar();
  acimaDoLimite_ = 0;
  avisada_ = false;
}

void DetectorColisao::aprenderReferencia(const EntradaDeteccao& entrada) {
  if (!temReferencia_) {
    temReferencia_ = true;
    referenciaXMg_ = entrada.axMg;
    referenciaYMg_ = entrada.ayMg;
    return;
  }
  // Média móvel exponencial em inteiros: a referência anda `1 / 2^n` do caminho até a leitura.
  const uint8_t n = config_.deslizamentoReferencia;
  referenciaXMg_ += (static_cast<int32_t>(entrada.axMg) - referenciaXMg_) / (1 << n);
  referenciaYMg_ += (static_cast<int32_t>(entrada.ayMg) - referenciaYMg_) / (1 << n);
}

TipoColisao DetectorColisao::atualizar(const EntradaDeteccao& entrada) {
  // Com o robô sem andar, a detecção fica desligada e a próxima colisão volta a ser avisada.
  if (!entrada.ativo) {
    protecao_.reiniciar();
    acimaDoLimite_ = 0;
    avisada_ = false;
    if (entrada.aceleracaoValida) {
      aprenderReferencia(entrada);
    }
    return TipoColisao::Nenhuma;
  }

  // Impacto: o afastamento da referência de repouso, no plano.
  bool impacto = false;
  if (entrada.aceleracaoValida) {
    if (!temReferencia_) {
      aprenderReferencia(entrada);  // primeira leitura: não há com o que comparar
    }
    const int64_t dx = static_cast<int64_t>(entrada.axMg) - referenciaXMg_;
    const int64_t dy = static_cast<int64_t>(entrada.ayMg) - referenciaYMg_;
    desvioMg_ = raizInteira(static_cast<uint64_t>(dx * dx + dy * dy));
    if (desvioMg_ >= config_.limiarImpactoMg) {
      if (acimaDoLimite_ < UINT8_MAX) {
        ++acimaDoLimite_;
      }
      impacto = acimaDoLimite_ >= config_.amostrasConfirmacao;
    } else {
      acimaDoLimite_ = 0;
      // Só leituras calmas ajustam a referência: um impacto não pode virar o "novo repouso".
      if (desvioMg_ < static_cast<uint32_t>(config_.limiarImpactoMg) / 2) {
        aprenderReferencia(entrada);
      }
    }
  } else {
    acimaDoLimite_ = 0;
  }

  // Frente parada: potência alta sem nenhum sinal de movimento. Uma frente além do alcance do ToF
  // (a leitura fica no máximo, 1200 mm) não muda andando por um corredor longo: não vale como parada.
  MedidasProtecao medidas = entrada.medidas;
  medidas.frenteValida = medidas.frenteValida && medidas.frenteMm < DISTANCIA_MAXIMA_VALIDA_TOF_MM;
  const bool frenteParada = protecao_.atualizar(entrada.potencia, medidas, entrada.dtMs) == EstadoProtecao::Travado;

  if (avisada_) {
    return TipoColisao::Nenhuma;
  }
  const TipoColisao tipo = impacto ? TipoColisao::Impacto : (frenteParada ? TipoColisao::FrenteParada : TipoColisao::Nenhuma);
  if (tipo != TipoColisao::Nenhuma) {
    avisada_ = true;
    ultima_ = tipo;
    ++colisoes_;
  }
  return tipo;
}

}  // namespace micromouse
