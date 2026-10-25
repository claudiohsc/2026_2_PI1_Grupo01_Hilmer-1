#pragma once

/// @file RegistradorPotencia.h
/// @brief Decorador de `IMotores` que guarda a última potência comandada (#192).

#include "IMotores.h"

namespace micromouse {

/// Repassa tudo ao motor que envolve e guarda a última potência comandada, para a detecção de
/// colisão saber se o robô foi mandado andar. Fica na camada de fora (`motoresTracao`): o que a
/// navegação comandou, mesmo que as camadas de dentro tenham recusado.
///
/// Não aloca memória e não depende do Arduino. Só a tarefa de navegação o usa.
class RegistradorPotencia : public IMotores {
 public:
  explicit RegistradorPotencia(IMotores& motores) : motores_(motores) {}

  /// @copydoc IMotores::definirPotencia
  void definirPotencia(const PotenciaRodas& potencia) override {
    ultima_ = {limitarPotencia(potencia.esquerda), limitarPotencia(potencia.direita)};
    motores_.definirPotencia(potencia);
  }

  /// @copydoc IMotores::parar
  void parar() override {
    ultima_ = {0, 0};
    motores_.parar();
  }

  /// Última potência comandada (zero depois de `parar()`).
  PotenciaRodas ultimaPotencia() const { return ultima_; }

 private:
  IMotores& motores_;
  PotenciaRodas ultima_ = {0, 0};
};

}  // namespace micromouse
