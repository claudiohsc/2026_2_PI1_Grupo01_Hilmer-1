#include "CicloNavegacao.h"

#include "AtualizadorMapa.h"
#include "DetectorObjetivo.h"

namespace micromouse {

namespace {

constexpr int32_t VOLTA_MILIGRAUS = 360000;

}  // namespace

CicloNavegacao::CicloNavegacao(Mapa& mapa, const ConfiguracaoExecucao& config, const ConfiguracaoControle& controle,
                               const ConfiguracaoGiro& giro)
    : mapa_(mapa), navegacao_(mapa), executor_(config, controle, giro), config_(config) {
  if (config_.amostrasPercepcao == 0) {
    config_.amostrasPercepcao = 1;
  } else if (config_.amostrasPercepcao > MAX_AMOSTRAS) {
    config_.amostrasPercepcao = MAX_AMOSTRAS;
  }
}

void CicloNavegacao::reiniciar(const Pose& inicial, IMotores& motores) {
  executor_.cancelar(motores);
  motores.parar();
  navegacao_.definirObjetivo(DetectorObjetivo::calcularObjetivo(mapa_.obterLinhas(), mapa_.obterColunas()));
  navegacao_.definirPose(inicial);
  direcao_ = inicial.direcao;
  direcaoPasso_ = inicial.direcao;
  direcaoLargada_ = inicial.direcao;
  largou_ = false;
  falhaExecucao_ = FalhaExecucao::Nenhuma;
  ultimaConfirmacao_ = ResultadoConfirmacao::Confirmada;
  celulasAvancadas_ = 0;
  girosFeitos_ = 0;
  quantidadeAmostras_ = 0;
  estado_ = EstadoCiclo::AguardandoLargada;
}

int32_t CicloNavegacao::rumoDaDirecao(Direcao d, int32_t anguloAtual) const {
  // Giros para a direita diminuem o ângulo do giroscópio (positivo = esquerda).
  const int32_t quartos = (static_cast<int32_t>(d) - static_cast<int32_t>(direcaoLargada_) + 4) % 4;
  int32_t rumo = rumoLargadaMiligraus_ - quartos * ANGULO_GIRO_90_MILIGRAUS;
  while (rumo - anguloAtual > VOLTA_MILIGRAUS / 2) {
    rumo -= VOLTA_MILIGRAUS;
  }
  while (anguloAtual - rumo > VOLTA_MILIGRAUS / 2) {
    rumo += VOLTA_MILIGRAUS;
  }
  return rumo;
}

void CicloNavegacao::parar(EstadoCiclo estado, IMotores& motores) {
  motores.parar();
  estado_ = estado;
}

void CicloNavegacao::entrarEmPercepcao() {
  quantidadeAmostras_ = 0;
  estado_ = EstadoCiclo::Percebendo;
}

EstadoCiclo CicloNavegacao::atualizar(const EntradaCiclo& entrada, IMotores& motores) {
  const bool parado = estado_ == EstadoCiclo::ObjetivoAlcancado || estado_ == EstadoCiclo::SemCaminho ||
                      estado_ == EstadoCiclo::Falha;
  if (parado) {
    return estado_;
  }

  if (!entrada.liberado) {
    // Sem permissão (largada ainda não liberada, ou retirada no meio): freia e espera. Ao voltar,
    // percebe de novo a partir da pose em que parou.
    if (estado_ != EstadoCiclo::AguardandoLargada) {
      executor_.cancelar(motores);
      navegacao_.cancelarPasso();
      parar(EstadoCiclo::AguardandoLargada, motores);
    }
    return estado_;
  }

  switch (estado_) {
    case EstadoCiclo::AguardandoLargada:
      if (!entrada.giro.valida) {
        break;  // sem giroscópio, não há rumo para seguir: espera ele voltar
      }
      if (!largou_) {
        largou_ = true;
        rumoLargadaMiligraus_ = entrada.giro.anguloMiligraus;
      }
      executor_.definirRumoAlvo(rumoDaDirecao(direcao_, entrada.giro.anguloMiligraus));
      motores.parar();
      entrarEmPercepcao();
      break;

    case EstadoCiclo::Percebendo:
      if (entrada.leituraNova != nullptr) {
        amostras_[quantidadeAmostras_++] = *entrada.leituraNova;
        if (quantidadeAmostras_ >= config_.amostrasPercepcao) {
          AtualizadorMapa::atualizar(mapa_, navegacao_.obterPose(), classificador_, amostras_,
                                     quantidadeAmostras_);
          decidir(motores);
        }
      }
      break;

    case EstadoCiclo::Girando: {
      const EntradaExecucao exec{entrada.leituraNova, entrada.giro, entrada.posicaoAvancoUm, entrada.dtMs};
      const EstadoExecucao resultado = executor_.atualizar(exec, motores);
      if (resultado == EstadoExecucao::Concluido) {
        direcao_ = direcaoPasso_;
        ++girosFeitos_;
        // O avanço começa no próximo ciclo, depois de a odometria adotar a direção nova.
        executor_.iniciar(Movimento::AvancarCelula);
        estado_ = EstadoCiclo::Avancando;
      } else if (resultado == EstadoExecucao::Falha) {
        falhaExecucao_ = executor_.falha();
        navegacao_.cancelarPasso();
        parar(EstadoCiclo::Falha, motores);
      }
      break;
    }

    case EstadoCiclo::Avancando: {
      const EntradaExecucao exec{entrada.leituraNova, entrada.giro, entrada.posicaoAvancoUm, entrada.dtMs};
      const EstadoExecucao resultado = executor_.atualizar(exec, motores);
      if (resultado == EstadoExecucao::Concluido ||
          (resultado == EstadoExecucao::Falha && executor_.falha() == FalhaExecucao::ParedeNaFrente)) {
        if (resultado == EstadoExecucao::Concluido) {
          ++celulasAvancadas_;
        } else {
          // O robô parou numa parede antes do destino: é a evidência mais forte que existe. Se a
          // percepção a tinha visto como livre, corrige, senão o planejamento voltaria por ela.
          mapa_.corrigirParede(navegacao_.obterPosicao(), direcao_, EstadoParede::Parede);
        }
        confirmarChegada(entrada);
        if (estado_ != EstadoCiclo::Falha) {
          entrarEmPercepcao();
        } else {
          motores.parar();
        }
      } else if (resultado == EstadoExecucao::Falha) {
        falhaExecucao_ = executor_.falha();
        navegacao_.cancelarPasso();
        parar(EstadoCiclo::Falha, motores);
      }
      break;
    }

    case EstadoCiclo::ObjetivoAlcancado:
    case EstadoCiclo::SemCaminho:
    case EstadoCiclo::Falha:
      break;
  }
  return estado_;
}

void CicloNavegacao::confirmarChegada(const EntradaCiclo& entrada) {
  if (!entrada.poseValida) {
    falhaExecucao_ = FalhaExecucao::Nenhuma;
    navegacao_.cancelarPasso();
    estado_ = EstadoCiclo::Falha;
    return;
  }
  ultimaConfirmacao_ = navegacao_.confirmarPose(Pose{entrada.pose.celula, direcao_});
  if (ultimaConfirmacao_ == ResultadoConfirmacao::Invalida) {
    falhaExecucao_ = FalhaExecucao::Nenhuma;
    navegacao_.cancelarPasso();
    estado_ = EstadoCiclo::Falha;
    return;
  }
  // Ainda na origem (parede no caminho, ou a estimativa não cruzou a fronteira): replaneja depois
  // de perceber de novo, em vez de esperar um passo que não vai terminar.
  navegacao_.cancelarPasso();
}

void CicloNavegacao::decidir(IMotores& motores) {
  if (navegacao_.atingiuObjetivo()) {
    parar(EstadoCiclo::ObjetivoAlcancado, motores);
    return;
  }
  Direcao proxima = direcao_;
  if (!navegacao_.darPasso(proxima)) {
    parar(EstadoCiclo::SemCaminho, motores);
    return;
  }
  direcaoPasso_ = proxima;
  Movimento giro = Movimento::AvancarCelula;
  if (giroAte(direcao_, proxima, giro)) {
    executor_.iniciar(giro);
    estado_ = EstadoCiclo::Girando;
  } else {
    executor_.iniciar(Movimento::AvancarCelula);
    estado_ = EstadoCiclo::Avancando;
  }
}

const char* nomeDoEstadoCiclo(EstadoCiclo estado) {
  switch (estado) {
    case EstadoCiclo::AguardandoLargada:
      return "aguardando_largada";
    case EstadoCiclo::Percebendo:
      return "percebendo";
    case EstadoCiclo::Girando:
      return "girando";
    case EstadoCiclo::Avancando:
      return "avancando";
    case EstadoCiclo::ObjetivoAlcancado:
      return "objetivo_alcancado";
    case EstadoCiclo::SemCaminho:
      return "sem_caminho";
    case EstadoCiclo::Falha:
      return "falha";
  }
  return "falha";
}

}  // namespace micromouse
