#include "DeslocamentoEstimado.h"

namespace micromouse {

DeslocamentoEstimado::DeslocamentoEstimado(IMotores& reais, const ConfiguracaoDeslocamento& config)
    : estimador_(config),
      motores_(reais, estimador_),
      encoderEsquerdo_(estimador_, Roda::Esquerda),
      encoderDireito_(estimador_, Roda::Direita),
      corretor_(config) {}

ResultadoCorrecao DeslocamentoEstimado::ciclo(uint32_t dtMs, Odometria& odometria,
                                              const DistanciasLaterais* leituraNova) {
  estimador_.atualizar(dtMs);
  odometria.atualizar(encoderEsquerdo_, encoderDireito_);
  if (leituraNova == nullptr) {
    return ResultadoCorrecao::SemReferencia;
  }
  const Correcao correcao = corretor_.avaliar(odometria, *leituraNova, estimador_);
  ultimaCorrecao_ = correcao;
  if (correcao.resultado == ResultadoCorrecao::PelaFrente ||
      correcao.resultado == ResultadoCorrecao::PelaLateral) {
    // Pulsos iguais nas duas rodas: a Odometria avança (ou recua) sem girar.
    estimador_.corrigirAvanco(correcao.deltaUm);
    odometria.atualizar(encoderEsquerdo_, encoderDireito_);
  }
  return correcao.resultado;
}

void DeslocamentoEstimado::reiniciar(Odometria& odometria, Pose pose) {
  encoderEsquerdo_.zerar();
  encoderDireito_.zerar();
  estimador_.registrarConferencia();
  odometria.reiniciar(pose, 0, 0);
  corretor_.esquecerLaterais();
}

}  // namespace micromouse
