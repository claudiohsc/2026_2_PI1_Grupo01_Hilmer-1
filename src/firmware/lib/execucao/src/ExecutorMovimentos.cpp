#include "ExecutorMovimentos.h"

#include "Configuracao.h"

namespace micromouse {

namespace {

constexpr int64_t CELULA_UM = static_cast<int64_t>(TAMANHO_CELULA_MM) * 1000;

/// Centro de célula mais próximo de `posicaoUm` (os centros ficam nos múltiplos de `CELULA_UM`).
int64_t centroMaisProximo(int64_t posicaoUm) {
  int64_t dobro = 2 * posicaoUm + CELULA_UM;
  int64_t quociente = dobro / (2 * CELULA_UM);
  if (dobro % (2 * CELULA_UM) != 0 && dobro < 0) {
    quociente--;
  }
  return quociente * CELULA_UM;
}

/// Soma o recuo do sensor à leitura, sem passar do maior `uint16_t`.
uint16_t somarRecuo(uint16_t leituraMm, uint16_t recuoMm) {
  const uint32_t soma = static_cast<uint32_t>(leituraMm) + recuoMm;
  return soma > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(soma);
}

}  // namespace

ExecutorMovimentos::ExecutorMovimentos(const ConfiguracaoExecucao& config, const ConfiguracaoControle& controle,
                                       const ConfiguracaoGiro& giro)
    : config_(config),
      configControle_(controle),
      configGiro_(giro),
      controleMovimento_(controle),
      controleGiro_(giro) {}

bool ExecutorMovimentos::iniciar(Movimento movimento) {
  if (ocupado()) {
    return false;
  }
  movimento_ = movimento;
  estado_ = EstadoExecucao::Executando;
  falha_ = FalhaExecucao::Nenhuma;
  comecou_ = false;
  tempoMs_ = 0;
  restanteUm_ = 0;
  semLeituraMs_ = 0;
  desdeCalculoMs_ = 0;
  temPotencia_ = false;
  ultimaPotencia_ = {0, 0};
  rumoAlvoMiligraus_ += variacaoRumoMiligraus(movimento);
  return true;
}

EstadoExecucao ExecutorMovimentos::atualizar(const EntradaExecucao& entrada, IMotores& motores) {
  if (!ocupado()) {
    return estado_;
  }
  return ehGiro(movimento_) ? atualizarGiro(entrada, motores) : atualizarAvanco(entrada, motores);
}

void ExecutorMovimentos::cancelar(IMotores& motores) {
  if (!ocupado()) {
    return;
  }
  controleGiro_.cancelar();
  motores.parar();
  estado_ = EstadoExecucao::Ocioso;
}

DistanciasLaterais ExecutorMovimentos::paraOCentro(const DistanciasLaterais& leitura) const {
  return {somarRecuo(leitura.frenteMm, config_.distanciaSensorFrontalAoCentroMm),
          somarRecuo(leitura.esquerdaMm, config_.distanciaSensorLateralAoCentroMm),
          somarRecuo(leitura.direitaMm, config_.distanciaSensorLateralAoCentroMm)};
}

int16_t ExecutorMovimentos::tetoParaRestante(int64_t restanteUm) const {
  const int64_t rampaUm = static_cast<int64_t>(config_.distanciaDesaceleracaoMm) * 1000;
  if (rampaUm <= 0 || restanteUm >= rampaUm) {
    return INT16_MAX;
  }
  const int64_t faixa = configControle_.potenciaCruzeiro - configControle_.potenciaMinima;
  const int64_t restante = restanteUm < 0 ? 0 : restanteUm;
  return static_cast<int16_t>(configControle_.potenciaMinima + faixa * restante / rampaUm);
}

EstadoExecucao ExecutorMovimentos::terminar(EstadoExecucao estado, FalhaExecucao falha, IMotores& motores) {
  motores.parar();
  estado_ = estado;
  falha_ = falha;
  return estado_;
}

EstadoExecucao ExecutorMovimentos::atualizarAvanco(const EntradaExecucao& entrada, IMotores& motores) {
  if (!comecou_) {
    comecou_ = true;
    destinoUm_ = centroMaisProximo(entrada.posicaoAvancoUm) + CELULA_UM;
    controleMovimento_.reiniciar(rumoAlvoMiligraus_);
  } else {
    tempoMs_ += entrada.dtMs;
    desdeCalculoMs_ += entrada.dtMs;
  }

  restanteUm_ = destinoUm_ - entrada.posicaoAvancoUm;
  if (restanteUm_ <= static_cast<int64_t>(config_.toleranciaChegadaMm) * 1000) {
    return terminar(EstadoExecucao::Concluido, FalhaExecucao::Nenhuma, motores);
  }
  if (tempoMs_ >= config_.tempoMaximoAvancoMs) {
    return terminar(EstadoExecucao::Falha, FalhaExecucao::TempoEsgotado, motores);
  }

  if (entrada.leituraNova != nullptr) {
    semLeituraMs_ = 0;
    const DistanciasLaterais centro = paraOCentro(*entrada.leituraNova);
    controleMovimento_.definirTetoPotenciaBase(tetoParaRestante(restanteUm_));
    ultimaPotencia_ = entrada.giro.valida
                          ? controleMovimento_.calcular(&centro, desdeCalculoMs_, entrada.giro.anguloMiligraus)
                          : controleMovimento_.calcular(&centro, desdeCalculoMs_);
    desdeCalculoMs_ = 0;
    temPotencia_ = true;
    if (controleMovimento_.estado() == EstadoMovimento::ParadoNaParede) {
      const bool pertoDoDestino = restanteUm_ <= static_cast<int64_t>(config_.folgaParadaNaParedeMm) * 1000;
      return pertoDoDestino ? terminar(EstadoExecucao::Concluido, FalhaExecucao::Nenhuma, motores)
                            : terminar(EstadoExecucao::Falha, FalhaExecucao::ParedeNaFrente, motores);
    }
  } else {
    semLeituraMs_ += entrada.dtMs;
    if (semLeituraMs_ >= configControle_.tempoMaximoSemLeituraMs) {
      return terminar(EstadoExecucao::Falha, FalhaExecucao::SemLeituraTof, motores);
    }
  }

  // Entre duas leituras do ToF (cerca de 33 ms), mantém o último comando.
  if (temPotencia_) {
    motores.definirPotencia(ultimaPotencia_);
  } else {
    motores.parar();
  }
  return estado_;
}

EstadoExecucao ExecutorMovimentos::atualizarGiro(const EntradaExecucao& entrada, IMotores& motores) {
  if (!comecou_) {
    if (!entrada.giro.valida) {
      // Sem ângulo não há como começar: espera parado, dentro do mesmo tempo limite do giro.
      tempoMs_ += entrada.dtMs;
      if (tempoMs_ >= configGiro_.tempoMaximoGiroMs) {
        return terminar(EstadoExecucao::Falha, FalhaExecucao::GiroTempoEsgotado, motores);
      }
      motores.parar();
      return estado_;
    }
    comecou_ = true;
    const int32_t angulo = entrada.giro.anguloMiligraus;
    controleGiro_.iniciar(rumoAlvoMiligraus_ - angulo, angulo);
  }

  tempoMs_ += entrada.dtMs;
  const EstadoGiro estadoGiro = controleGiro_.atualizar(entrada.giro, motores, entrada.dtMs);
  if (estadoGiro == EstadoGiro::Concluido) {
    return terminar(EstadoExecucao::Concluido, FalhaExecucao::Nenhuma, motores);
  }
  if (estadoGiro == EstadoGiro::Timeout) {
    return terminar(EstadoExecucao::Falha, FalhaExecucao::GiroTempoEsgotado, motores);
  }
  return estado_;
}

const char* nomeDoEstadoExecucao(EstadoExecucao estado) {
  switch (estado) {
    case EstadoExecucao::Ocioso:
      return "ocioso";
    case EstadoExecucao::Executando:
      return "executando";
    case EstadoExecucao::Concluido:
      return "concluido";
    case EstadoExecucao::Falha:
      return "falha";
  }
  return "falha";
}

const char* nomeDaFalhaExecucao(FalhaExecucao falha) {
  switch (falha) {
    case FalhaExecucao::Nenhuma:
      return "nenhuma";
    case FalhaExecucao::ParedeNaFrente:
      return "parede_na_frente";
    case FalhaExecucao::SemLeituraTof:
      return "sem_leitura_tof";
    case FalhaExecucao::TempoEsgotado:
      return "tempo_esgotado";
    case FalhaExecucao::GiroTempoEsgotado:
      return "giro_tempo_esgotado";
  }
  return "nenhuma";
}

}  // namespace micromouse
