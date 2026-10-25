#include "MotoresComProtecao.h"

namespace micromouse {

MotoresComProtecao::MotoresComProtecao(IMotores& motores, const ConfiguracaoProtecaoMotor& config)
    : motores_(motores), protecao_(config) {}

void MotoresComProtecao::definirPotencia(const PotenciaRodas& potencia) {
  if (protecao_.travado()) {
    ++recusados_;
    parar();
    return;
  }
  ultima_ = {limitarPotencia(potencia.esquerda), limitarPotencia(potencia.direita)};
  motores_.definirPotencia(ultima_);
}

void MotoresComProtecao::parar() {
  ultima_ = {0, 0};
  motores_.parar();
}

EstadoProtecao MotoresComProtecao::atualizarProtecao(const MedidasProtecao& medidas, uint32_t dtMs) {
  const bool estavaTravado = protecao_.travado();
  const EstadoProtecao estado = protecao_.atualizar(ultima_, medidas, dtMs);
  if (estado == EstadoProtecao::Travado && !estavaTravado) {
    ++cortes_;
    parar();
  }
  return estado;
}

void MotoresComProtecao::liberar() { protecao_.reiniciar(); }

}  // namespace micromouse
