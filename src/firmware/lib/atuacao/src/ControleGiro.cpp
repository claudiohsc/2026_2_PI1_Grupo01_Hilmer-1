#include "ControleGiro.h"

namespace micromouse {

ControleGiro::ControleGiro(const ConfiguracaoGiro& config) : config_(config) {}

void ControleGiro::iniciar(int32_t anguloRelativoMiligraus, int32_t anguloAtualMiligraus) {
  anguloAlvoMiligraus_ = anguloAtualMiligraus + anguloRelativoMiligraus;
  erroMiligraus_ = anguloRelativoMiligraus;
  tempoDecorridoMs_ = 0;
  ciclosNaTolerancia_ = 0;
  estado_ = EstadoGiro::Girando;
}

PotenciaRodas ControleGiro::calcular(int32_t anguloAtualMiligraus, uint32_t dtMs) {
  if (estado_ != EstadoGiro::Girando) {
    return {0, 0};
  }

  tempoDecorridoMs_ += dtMs;
  if (tempoDecorridoMs_ >= config_.tempoMaximoGiroMs) {
    estado_ = EstadoGiro::Timeout;
    return {0, 0};
  }

  erroMiligraus_ = anguloAlvoMiligraus_ - anguloAtualMiligraus;
  const int32_t erroAbsoluto = erroMiligraus_ >= 0 ? erroMiligraus_ : -erroMiligraus_;

  if (erroAbsoluto <= config_.toleranciaMiligraus) {
    ++ciclosNaTolerancia_;
    if (ciclosNaTolerancia_ >= config_.ciclosEstabilizacao) {
      estado_ = EstadoGiro::Concluido;
      return {0, 0};
    }
  } else {
    ciclosNaTolerancia_ = 0;
  }

  // Modulação proporcional da potência:
  // erroAbsoluto está em mili-graus (1000 = 1°).
  // ganhoProporcionalDecimos está em décimos de potência por grau (ex: 40 = 4,0 pot/grau).
  // potenciaP = (erroAbsoluto * ganhoProporcionalDecimos) / 10000;
  const int32_t potenciaP = (erroAbsoluto * config_.ganhoProporcionalDecimos) / 10000;
  int16_t potenciaComandada = static_cast<int16_t>(config_.potenciaMinima + potenciaP);
  if (potenciaComandada > config_.potenciaMaxima) {
    potenciaComandada = config_.potenciaMaxima;
  }

  // Sentido do giro diferencial:
  // Se erro > 0: alvo está à esquerda (anti-horário) -> roda esquerda recua (-), roda direita avança (+)
  // Se erro < 0: alvo está à direita (horário) -> roda esquerda avança (+), roda direita recua (-)
  if (erroMiligraus_ > 0) {
    return {static_cast<int16_t>(-potenciaComandada), potenciaComandada};
  } else {
    return {potenciaComandada, static_cast<int16_t>(-potenciaComandada)};
  }
}

EstadoGiro ControleGiro::atualizar(int32_t anguloAtualMiligraus, IMotores& motores, uint32_t dtMs) {
  const PotenciaRodas potencia = calcular(anguloAtualMiligraus, dtMs);
  if (estado_ == EstadoGiro::Concluido || estado_ == EstadoGiro::Timeout) {
    motores.parar();
  } else if (estado_ == EstadoGiro::Girando) {
    motores.definirPotencia(potencia);
  }
  return estado_;
}

EstadoGiro ControleGiro::atualizar(const LeituraGiro& giro, IMotores& motores, uint32_t dtMs) {
  if (!giro.valida) {
    // Se a leitura for inválida, freia os motores e acumula tempo para timeout se persistir
    if (estado_ == EstadoGiro::Girando) {
      motores.parar();
      tempoDecorridoMs_ += dtMs;
      if (tempoDecorridoMs_ >= config_.tempoMaximoGiroMs) {
        estado_ = EstadoGiro::Timeout;
      }
    }
    return estado_;
  }
  return atualizar(giro.anguloMiligraus, motores, dtMs);
}

void ControleGiro::cancelar() {
  estado_ = EstadoGiro::Inativo;
  tempoDecorridoMs_ = 0;
  ciclosNaTolerancia_ = 0;
  erroMiligraus_ = 0;
}

}  // namespace micromouse
