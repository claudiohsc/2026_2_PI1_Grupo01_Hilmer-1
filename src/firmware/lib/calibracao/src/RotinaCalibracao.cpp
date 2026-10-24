#include "RotinaCalibracao.h"

namespace micromouse {

const char* nomeEstadoRotina(EstadoRotinaCalibracao estado) {
  switch (estado) {
    case EstadoRotinaCalibracao::NaoIniciada:
      return "nao_iniciada";
    case EstadoRotinaCalibracao::EmAndamento:
      return "em_andamento";
    case EstadoRotinaCalibracao::Sucesso:
      return "sucesso";
    case EstadoRotinaCalibracao::RecusadaPorMovimento:
      return "recusada_por_movimento";
    case EstadoRotinaCalibracao::TempoExcedido:
      return "tempo_excedido";
    case EstadoRotinaCalibracao::FalhaSensor:
      return "falha_sensor";
    case EstadoRotinaCalibracao::DesvioForaDaFaixa:
      return "desvio_fora_da_faixa";
  }
  return "?";
}

RotinaCalibracao::RotinaCalibracao(const ConfiguracaoRotinaCalibracao& config)
    : config_(config), calibradorTof_(config.tof), calibradorGiro_(config.giro) {}

void RotinaCalibracao::iniciar() {
  iniciar(config_.tof.referencias);
}

void RotinaCalibracao::iniciar(const ReferenciasTof& referencias) {
  calibradorTof_.iniciar(referencias);
  calibradorGiro_.iniciar();
  duracaoMedidaMs_ = 0;
  estado_ = EstadoRotinaCalibracao::EmAndamento;
}

EstadoRotinaCalibracao RotinaCalibracao::atualizar(const DistanciasLaterais& distancias,
                                                  bool distanciasValidas,
                                                  int16_t taxaGiroDecimosDps,
                                                  bool giroValido,
                                                  uint32_t dtMs) {
  if (estado_ != EstadoRotinaCalibracao::EmAndamento) {
    return estado_;
  }

  duracaoMedidaMs_ += dtMs;

  // Enforce RNF-08: não pode ultrapassar o orçamento de 1 s
  if (duracaoMedidaMs_ > config_.orcamentoTotalMs) {
    estado_ = EstadoRotinaCalibracao::TempoExcedido;
    return estado_;
  }

  // Se o giro ainda precisa de amostras e a leitura falhou
  if (calibradorGiro_.estado() == EstadoCalibracaoGiro::Calibrando) {
    if (!giroValido) {
      estado_ = EstadoRotinaCalibracao::FalhaSensor;
      return estado_;
    }
    calibradorGiro_.adicionarAmostra(taxaGiroDecimosDps);
  }

  // Se os ToF ainda precisam de amostras
  if (calibradorTof_.estado() == EstadoCalibracaoTof::Calibrando && distanciasValidas) {
    calibradorTof_.adicionarAmostra(distancias);
  }

  // Se algum sensor recusou (robô se mexeu)
  if (calibradorTof_.estado() == EstadoCalibracaoTof::Recusada ||
      calibradorGiro_.estado() == EstadoCalibracaoGiro::Recusada) {
    estado_ = EstadoRotinaCalibracao::RecusadaPorMovimento;
    return estado_;
  }

  // Se um desvio de ToF passou do máximo, a referência está errada: descarta a calibração
  if (calibradorTof_.estado() == EstadoCalibracaoTof::ForaDaFaixa) {
    estado_ = EstadoRotinaCalibracao::DesvioForaDaFaixa;
    return estado_;
  }

  // Se ambos concluíram com sucesso
  if (calibradorTof_.estado() == EstadoCalibracaoTof::Calibrado &&
      calibradorGiro_.estado() == EstadoCalibracaoGiro::Calibrado) {
    estado_ = EstadoRotinaCalibracao::Sucesso;
    return estado_;
  }

  return estado_;
}

EstadoRotinaCalibracao RotinaCalibracao::executar(ISensorDistancia& sensorDistancia, IImu& imu,
                                                 uint32_t dtPassoMs) {
  iniciar();
  uint32_t acumuladorTofMs = 0;

  while (estado_ == EstadoRotinaCalibracao::EmAndamento) {
    int16_t taxaGiro = 0;
    const bool imuOk = imu.lerTaxaGiroZ(taxaGiro);

    acumuladorTofMs += dtPassoMs;
    const bool lerTofAgora = (acumuladorTofMs >= PERIODO_AMOSTRAGEM_TOF_MS) ||
                             (calibradorTof_.amostrasRecebidas() == 0);

    DistanciasLaterais dist{};
    bool tofOk = true;
    if (lerTofAgora && calibradorTof_.estado() == EstadoCalibracaoTof::Calibrando) {
      tofOk = sensorDistancia.lerDistanciasLaterais(dist);
      acumuladorTofMs = 0;
      if (!tofOk) {
        estado_ = EstadoRotinaCalibracao::FalhaSensor;
        return estado_;
      }
    }

    atualizar(dist, lerTofAgora && tofOk, taxaGiro, imuOk, dtPassoMs);
  }

  return estado_;
}

}  // namespace micromouse

