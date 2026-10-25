#include "CalibradorGiro.h"

namespace micromouse {

int16_t CalibradorGiro::mediaArredondada(int32_t soma, uint16_t quantidade) {
  if (quantidade == 0) {
    return 0;
  }
  const int32_t metade = static_cast<int32_t>(quantidade / 2U);
  return static_cast<int16_t>(soma >= 0 ? (soma + metade) / static_cast<int32_t>(quantidade)
                                        : (soma - metade) / static_cast<int32_t>(quantidade));
}

CalibradorGiro::CalibradorGiro(const ConfiguracaoCalibracaoGiro& config) : config_(config) {}

void CalibradorGiro::iniciar() {
  estado_ = EstadoCalibracaoGiro::Calibrando;
  amostrasRecebidas_ = 0;
  somaAmostras_ = 0;
  menorAmostra_ = 0;
  maiorAmostra_ = 0;
}

EstadoCalibracaoGiro CalibradorGiro::adicionarAmostra(int16_t taxaDecimosDps) {
  if (estado_ != EstadoCalibracaoGiro::Calibrando) {
    return estado_;
  }

  if (amostrasRecebidas_ == 0) {
    menorAmostra_ = taxaDecimosDps;
    maiorAmostra_ = taxaDecimosDps;
  } else {
    if (taxaDecimosDps < menorAmostra_) {
      menorAmostra_ = taxaDecimosDps;
    }
    if (taxaDecimosDps > maiorAmostra_) {
      maiorAmostra_ = taxaDecimosDps;
    }
  }

  somaAmostras_ += taxaDecimosDps;
  ++amostrasRecebidas_;

  if (amostrasRecebidas_ < config_.amostras) {
    return estado_;
  }

  const int32_t variacao = static_cast<int32_t>(maiorAmostra_) - menorAmostra_;
  if (variacao > config_.variacaoMaximaDecimosDps) {
    estado_ = EstadoCalibracaoGiro::Recusada;
  } else {
    vies_ = mediaArredondada(somaAmostras_, amostrasRecebidas_);
    estado_ = EstadoCalibracaoGiro::Calibrado;
  }

  return estado_;
}

int16_t CalibradorGiro::corrigir(int16_t taxaBrutaDecimosDps) const {
  return static_cast<int16_t>(taxaBrutaDecimosDps - vies_);
}

}  // namespace micromouse

