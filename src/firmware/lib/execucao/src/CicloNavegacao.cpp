#include "CicloNavegacao.h"

#include "AtualizadorMapa.h"
#include "ConfiguracaoTof.h"
#include "DetectorObjetivo.h"

namespace micromouse {

namespace {

constexpr int32_t VOLTA_MILIGRAUS = 360000;
constexpr int64_t CELULA_UM = static_cast<int64_t>(TAMANHO_CELULA_MM) * 1000;

/// Quanto a posição ao longo do eixo está além do centro de célula mais próximo, em µm (negativo =
/// antes dele). Os centros estão nos múltiplos de `CELULA_UM`.
int64_t excessoSobreOCentroUm(int64_t posicaoUm) {
  int64_t resto = posicaoUm % CELULA_UM;
  if (resto > CELULA_UM / 2) {
    resto -= CELULA_UM;
  } else if (resto < -CELULA_UM / 2) {
    resto += CELULA_UM;
  }
  return resto;
}

}  // namespace

CicloNavegacao::CicloNavegacao(Mapa& mapa, const ConfiguracaoExecucao& config, const ConfiguracaoControle& controle,
                               const ConfiguracaoGiro& giro, const ConfiguracaoRealinhamento& realinhamento,
                               const ConfiguracaoColisao& colisao)
    : mapa_(mapa),
      navegacao_(mapa),
      executor_(config, controle, giro, realinhamento),
      config_(config),
      configRealinhamento_(realinhamento),
      configColisao_(colisao) {
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
  realinhamentos_ = 0;
  colisoes_ = 0;
  recuperacoes_ = 0;
  paredesPorColisao_ = 0;
  colisoesSeguidas_ = 0;
  celulasNaUltimaColisao_ = 0;
  colisoesNaMesmaFrente_ = 0;
  ultimaColisao_ = TipoColisao::Nenhuma;
  temFrente_ = false;
  realinhamentosNoPasso_ = 0;
  conferindoRealinhamento_ = false;
  ultimoDesvio_ = DesvioLateral{};
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
      conferindoRealinhamento_ = false;
      parar(EstadoCiclo::AguardandoLargada, motores);
    }
    return estado_;
  }

  if (entrada.leituraNova != nullptr) {
    ultimaFrenteMm_ = entrada.leituraNova->frenteMm;
    temFrente_ = true;
  }
  if (entrada.colisao != TipoColisao::Nenhuma && emMovimento()) {
    iniciarRecuperacao(entrada, motores);
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

    case EstadoCiclo::Realinhando: {
      const EntradaExecucao exec{entrada.leituraNova, entrada.giro, entrada.posicaoAvancoUm, entrada.dtMs};
      const EstadoExecucao resultado = conferindoRealinhamento_ ? EstadoExecucao::Concluido
                                                                : executor_.atualizar(exec, motores);
      if (conferindoRealinhamento_) {
        // Parado de novo: junta leituras para medir quanto falta.
        if (entrada.leituraNova != nullptr) {
          amostras_[quantidadeAmostras_++] = *entrada.leituraNova;
          if (quantidadeAmostras_ >= config_.amostrasPercepcao) {
            conferindoRealinhamento_ = false;
            realinharOuGirar(giroPendente_);
          }
        }
        break;
      }
      if (resultado == EstadoExecucao::Concluido) {
        ++realinhamentos_;
        conferindoRealinhamento_ = true;
        quantidadeAmostras_ = 0;
      } else if (resultado == EstadoExecucao::Falha) {
        falhaExecucao_ = executor_.falha();
        navegacao_.cancelarPasso();
        parar(EstadoCiclo::Falha, motores);
      }
      break;
    }

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

    case EstadoCiclo::Recuperando: {
      const EntradaExecucao exec{entrada.leituraNova, entrada.giro, entrada.posicaoAvancoUm, entrada.dtMs};
      const EstadoExecucao resultado = executor_.atualizar(exec, motores);
      if (resultado == EstadoExecucao::Concluido) {
        ++recuperacoes_;
        entrarEmPercepcao();
      } else if (resultado == EstadoExecucao::Falha) {
        falhaExecucao_ = executor_.falha();
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

void CicloNavegacao::iniciarRecuperacao(const EntradaCiclo& entrada, IMotores& motores) {
  const EstadoCiclo antes = estado_;
  ++colisoes_;
  ultimaColisao_ = entrada.colisao;
  executor_.cancelar(motores);
  motores.parar();
  navegacao_.cancelarPasso();
  conferindoRealinhamento_ = false;
  realinhamentosNoPasso_ = 0;
  quantidadeAmostras_ = 0;

  // A contagem das colisões seguidas recomeça quando o robô conclui uma célula.
  if (celulasAvancadas_ != celulasNaUltimaColisao_) {
    colisoesSeguidas_ = 0;
    colisoesNaMesmaFrente_ = 0;
  }
  celulasNaUltimaColisao_ = celulasAvancadas_;
  if (colisoesSeguidas_ < UINT8_MAX) {
    ++colisoesSeguidas_;
  }

  // Colisão repetida avançando, na mesma célula e direção: uma parede que a percepção não viu. Grava
  // só ela, e só se o ToF frontal não mede a frente livre (o impacto pode ter vindo de outro lado).
  // Um ToF que não vê nada (leitura no alcance máximo) não contradiz o impacto.
  if (antes == EstadoCiclo::Avancando) {
    const PosicaoCelula aqui = navegacao_.obterPosicao();
    const bool mesmoLugar = colisoesNaMesmaFrente_ > 0 && aqui.linha == celulaDaColisao_.linha &&
                            aqui.coluna == celulaDaColisao_.coluna && direcao_ == direcaoDaColisao_;
    colisoesNaMesmaFrente_ = mesmoLugar ? static_cast<uint8_t>(colisoesNaMesmaFrente_ + 1) : 1;
    celulaDaColisao_ = aqui;
    direcaoDaColisao_ = direcao_;
    // Só vale se o robô está onde a batida na parede da própria célula acontece: a odometria concorda com
    // a célula da navegação e o robô está a ~34 mm além do centro. Senão, a batida foi noutro lugar
    // (por exemplo, já na célula seguinte) e gravar essa parede apagaria um caminho livre.
    const int64_t excessoUm = excessoSobreOCentroUm(entrada.posicaoAvancoUm);
    const bool naParede = entrada.poseValida && entrada.pose.celula.linha == aqui.linha &&
                          entrada.pose.celula.coluna == aqui.coluna &&
                          excessoUm >= static_cast<int64_t>(configColisao_.excessoMinimoParedeMm) * 1000 &&
                          excessoUm <= static_cast<int64_t>(configColisao_.excessoMaximoParedeMm) * 1000;
    const bool frenteLivre = temFrente_ && ultimaFrenteMm_ > configColisao_.limiarFrenteLivreMm &&
                             ultimaFrenteMm_ < DISTANCIA_MAXIMA_VALIDA_TOF_MM;
    if (colisoesNaMesmaFrente_ >= 2 && naParede && !frenteLivre &&
        mapa_.obterParede(aqui, direcao_) != EstadoParede::Parede) {
      mapa_.corrigirParede(aqui, direcao_, EstadoParede::Parede);
      ++paredesPorColisao_;
    }
  } else {
    colisoesNaMesmaFrente_ = 0;
  }

  if (colisoesSeguidas_ > configColisao_.colisoesSeguidasMaximas) {
    falhaExecucao_ = FalhaExecucao::ColisoesSeguidas;
    estado_ = EstadoCiclo::Falha;
    return;
  }

  // O recuo mantém o rumo da direção atual (o giro interrompido já tinha mudado o rumo alvo).
  if (entrada.giro.valida) {
    executor_.definirRumoAlvo(rumoDaDirecao(direcao_, entrada.giro.anguloMiligraus));
  } else if (antes == EstadoCiclo::Girando) {
    executor_.definirRumoAlvo(executor_.rumoNominalMiligraus() - variacaoRumoMiligraus(executor_.movimento()));
  }
  // Recua até o centro da célula (pela odometria), no mínimo `recuoMinimoMm` e no máximo `recuoMm`.
  int64_t recuoMm = entrada.poseValida ? excessoSobreOCentroUm(entrada.posicaoAvancoUm) / 1000
                                       : static_cast<int64_t>(configColisao_.recuoMm);
  recuoMm = recuoMm < configColisao_.recuoMinimoMm ? configColisao_.recuoMinimoMm : recuoMm;
  recuoMm = recuoMm > configColisao_.recuoMm ? configColisao_.recuoMm : recuoMm;
  if (!executor_.iniciarRecuo(static_cast<uint16_t>(recuoMm))) {
    falhaExecucao_ = FalhaExecucao::Nenhuma;
    estado_ = EstadoCiclo::Falha;
    return;
  }
  estado_ = EstadoCiclo::Recuperando;
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
    // Antes de girar no lugar, vai para o centro do corredor se estiver longe dele (#188).
    realinhamentosNoPasso_ = 0;
    realinharOuGirar(giro);
  } else {
    executor_.iniciar(Movimento::AvancarCelula);
    estado_ = EstadoCiclo::Avancando;
  }
}

void CicloNavegacao::realinharOuGirar(Movimento giro) {
  ultimoDesvio_ = desvioDasAmostras();
  const int32_t modulo = ultimoDesvio_.desvioUm < 0 ? -ultimoDesvio_.desvioUm : ultimoDesvio_.desvioUm;
  const bool longeDoCentro = ultimoDesvio_.referencia != ReferenciaLateral::SemParede &&
                             modulo >= static_cast<int32_t>(configRealinhamento_.desvioMinimoMm) * 1000 &&
                             modulo <= static_cast<int32_t>(configRealinhamento_.desvioMaximoMm) * 1000;
  if (longeDoCentro && realinhamentosNoPasso_ < configRealinhamento_.tentativas) {
    const PosicaoCelula aqui = navegacao_.obterPosicao();
    const bool atrasLivre = mapa_.obterParede(aqui, oposta(direcao_)) == EstadoParede::Livre;
    const bool frenteLivre = mapa_.obterParede(aqui, direcao_) == EstadoParede::Livre;
    const bool recuar = atrasLivre || !frenteLivre;
    const bool livre = atrasLivre || frenteLivre;
    const PlanoRealinhamento plano = planejarRealinhamento(ultimoDesvio_.desvioUm, recuar, livre,
                                                           configRealinhamento_.trechoMaximoLivreMm,
                                                           configRealinhamento_.folgaMinimaMm);
    if (executor_.iniciarRealinhamento(plano)) {
      giroPendente_ = giro;
      ++realinhamentosNoPasso_;
      estado_ = EstadoCiclo::Realinhando;
      return;
    }
  }
  iniciarGiro(giro);
}

void CicloNavegacao::iniciarGiro(Movimento giro) {
  executor_.iniciar(giro);
  estado_ = EstadoCiclo::Girando;
}

DesvioLateral CicloNavegacao::desvioDasAmostras() const {
  // Mediana de cada lado (a mesma ideia do filtro da percepção).
  uint16_t esquerda[MAX_AMOSTRAS];
  uint16_t direita[MAX_AMOSTRAS];
  const uint8_t n = quantidadeAmostras_;
  for (uint8_t i = 0; i < n; i++) {
    esquerda[i] = amostras_[i].esquerdaMm;
    direita[i] = amostras_[i].direitaMm;
  }
  for (uint8_t i = 1; i < n; i++) {
    for (uint8_t j = i; j > 0 && esquerda[j - 1] > esquerda[j]; j--) {
      const uint16_t t = esquerda[j];
      esquerda[j] = esquerda[j - 1];
      esquerda[j - 1] = t;
    }
    for (uint8_t j = i; j > 0 && direita[j - 1] > direita[j]; j--) {
      const uint16_t t = direita[j];
      direita[j] = direita[j - 1];
      direita[j - 1] = t;
    }
  }
  if (n == 0) {
    return DesvioLateral{};
  }
  const DistanciasLaterais mediana{0, esquerda[n / 2], direita[n / 2]};
  return medirDesvioLateral(mediana, configRealinhamento_.limiarParedeLateralMm,
                            configRealinhamento_.distanciaSensorLateralAoCentroMm);
}

const char* nomeDoEstadoCiclo(EstadoCiclo estado) {
  switch (estado) {
    case EstadoCiclo::AguardandoLargada:
      return "aguardando_largada";
    case EstadoCiclo::Percebendo:
      return "percebendo";
    case EstadoCiclo::Realinhando:
      return "realinhando";
    case EstadoCiclo::Girando:
      return "girando";
    case EstadoCiclo::Avancando:
      return "avancando";
    case EstadoCiclo::Recuperando:
      return "recuperando";
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
