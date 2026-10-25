#include "FaseCorrida.h"

namespace micromouse {

const char* nomeDaFase(FaseCorrida::Fase fase) {
  switch (fase) {
    case FaseCorrida::Fase::AguardandoCalibracao:
      return "aguardando_calibracao";
    case FaseCorrida::Fase::AguardandoLargada:
      return "aguardando_largada";
    case FaseCorrida::Fase::EmCorrida:
      return "em_corrida";
    case FaseCorrida::Fase::Encerrada:
      return "encerrada";
  }
  return "?";
}

void FaseCorrida::calibracaoConcluida() {
  if (fase_ == Fase::AguardandoCalibracao) {
    fase_ = Fase::AguardandoLargada;
    temUltimaEspera_ = false;
  }
}

void FaseCorrida::reiniciar() {
  if (fase_ == Fase::EmCorrida) {
    fase_ = Fase::AguardandoLargada;
    temUltimaEspera_ = false;
  }
}

DecisaoColeta FaseCorrida::decidir(int32_t deslocamentoMm, uint32_t agoraMs, bool chegouAoObjetivo) {
  DecisaoColeta decisao;
  if (fase_ == Fase::AguardandoCalibracao || fase_ == Fase::Encerrada) {
    return decisao;
  }

  if (fase_ == Fase::AguardandoLargada) {
    const uint32_t deslocamentoAbsoluto =
        static_cast<uint32_t>(deslocamentoMm >= 0 ? deslocamentoMm : -static_cast<int64_t>(deslocamentoMm));
    if (deslocamentoAbsoluto < config_.deslocamentoMinimoMm) {
      // Ainda na partida: uma amostra logo ao entrar no estado e depois uma a cada período.
      const bool vencido = !temUltimaEspera_ || (agoraMs - ultimaEsperaMs_) >= config_.periodoAmostraAguardandoMs;
      if (vencido) {
        temUltimaEspera_ = true;
        ultimaEsperaMs_ = agoraMs;
        decisao.registrar = true;
        decisao.estado = EstadoCorrida::Aguardando;
        decisao.tempoDecorridoMs = 0;
      }
      return decisao;
    }
    // Largou: o cronômetro da corrida começa agora.
    fase_ = Fase::EmCorrida;
    inicioMs_ = agoraMs;
    decisao.largouAgora = true;
  }

  // Em corrida: uma amostra por ciclo.
  const uint32_t decorridoMs = agoraMs - inicioMs_;
  decisao.tempoEsgotado = decorridoMs >= config_.limiteTempoMs;
  decisao.ultima = chegouAoObjetivo || decisao.tempoEsgotado;
  decisao.registrar = true;
  decisao.tempoDecorridoMs = decorridoMs;
  decisao.estado = decisao.ultima ? EstadoCorrida::Parado : EstadoCorrida::Explorando;
  if (decisao.ultima) {
    fase_ = Fase::Encerrada;
  }
  return decisao;
}

}  // namespace micromouse
