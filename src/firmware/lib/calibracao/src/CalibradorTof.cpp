#include "CalibradorTof.h"

namespace micromouse {

namespace {

void adicionarAmostraLado(uint16_t& amostras, uint32_t& soma, uint16_t& menor, uint16_t& maior,
                          uint16_t distanciaMm) {
  if (amostras == 0) {
    menor = distanciaMm;
    maior = distanciaMm;
  } else {
    if (distanciaMm < menor) {
      menor = distanciaMm;
    }
    if (distanciaMm > maior) {
      maior = distanciaMm;
    }
  }
  soma += distanciaMm;
  ++amostras;
}

}  // namespace

CalibradorTof::CalibradorTof(const ConfiguracaoCalibracaoTof& config)
    : config_(config), referencias_(config.referencias) {}

void CalibradorTof::iniciar() { iniciar(config_.referencias); }

void CalibradorTof::iniciar(const ReferenciasTof& referencias) {
  referencias_ = referencias;
  estado_ = EstadoCalibracaoTof::Calibrando;
  amostrasRecebidas_ = 0;
  reiniciarAcumuladores();
}

void CalibradorTof::reiniciarAcumuladores() {
  frente_ = {};
  esquerda_ = {};
  direita_ = {};
}

uint16_t CalibradorTof::mediaArredondada(uint32_t soma, uint16_t quantidade) {
  const uint32_t metade = quantidade / 2U;
  return static_cast<uint16_t>((soma + metade) / quantidade);
}

bool CalibradorTof::ladoVariouDemais(const AcumuladorLado& lado) const {
  const uint16_t variacao = static_cast<uint16_t>(lado.maior - lado.menor);
  return variacao > config_.variacaoMaximaMm;
}

int16_t CalibradorTof::desvioDoLado(const AcumuladorLado& lado, uint16_t referenciaMm) const {
  const int32_t media = mediaArredondada(lado.soma, lado.amostras);
  return static_cast<int16_t>(media - static_cast<int32_t>(referenciaMm));
}

uint16_t CalibradorTof::subtrairSaturado(uint16_t bruta, int16_t desvioMm) {
  const int32_t corrigida = static_cast<int32_t>(bruta) - desvioMm;
  if (corrigida < 0) {
    return 0;
  }
  if (corrigida > UINT16_MAX) {
    return UINT16_MAX;
  }
  return static_cast<uint16_t>(corrigida);
}

EstadoCalibracaoTof CalibradorTof::adicionarAmostra(const DistanciasLaterais& leitura) {
  if (estado_ != EstadoCalibracaoTof::Calibrando) {
    return estado_;
  }

  adicionarAmostraLado(frente_.amostras, frente_.soma, frente_.menor, frente_.maior, leitura.frenteMm);
  adicionarAmostraLado(esquerda_.amostras, esquerda_.soma, esquerda_.menor, esquerda_.maior,
                       leitura.esquerdaMm);
  adicionarAmostraLado(direita_.amostras, direita_.soma, direita_.menor, direita_.maior,
                       leitura.direitaMm);
  ++amostrasRecebidas_;

  if (amostrasRecebidas_ < config_.amostras) {
    return estado_;
  }

  if (ladoVariouDemais(frente_) || ladoVariouDemais(esquerda_) || ladoVariouDemais(direita_)) {
    estado_ = EstadoCalibracaoTof::Recusada;
    return estado_;
  }

  desvios_.frenteMm = desvioDoLado(frente_, referencias_.frenteMm);
  desvios_.esquerdaMm = desvioDoLado(esquerda_, referencias_.esquerdaMm);
  desvios_.direitaMm = desvioDoLado(direita_, referencias_.direitaMm);
  estado_ = EstadoCalibracaoTof::Calibrado;
  return estado_;
}

DistanciasLaterais CalibradorTof::corrigir(const DistanciasLaterais& bruta) const {
  DistanciasLaterais saida;
  saida.frenteMm = subtrairSaturado(bruta.frenteMm, desvios_.frenteMm);
  saida.esquerdaMm = subtrairSaturado(bruta.esquerdaMm, desvios_.esquerdaMm);
  saida.direitaMm = subtrairSaturado(bruta.direitaMm, desvios_.direitaMm);
  return saida;
}

}  // namespace micromouse
