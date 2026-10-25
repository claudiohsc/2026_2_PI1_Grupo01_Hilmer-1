#pragma once

/// @file MotoresComProtecao.h
/// @brief Motores com a proteção contra motor travado no caminho de todo comando (#341).

#include <stdint.h>

#include "ConfiguracaoProtecaoMotor.h"
#include "IMotores.h"
#include "ProtecaoMotorTravado.h"

namespace micromouse {

/// Decorador de `IMotores`: repassa os comandos ao driver real e, a cada ciclo da navegação, passa
/// a potência comandada pela `ProtecaoMotorTravado`.
///
/// Quem comanda os motores (controle de movimento, detector do objetivo) recebe este objeto como
/// `IMotores&` e não muda nada. A navegação chama `atualizarProtecao()` uma vez por ciclo, com o
/// ToF frontal e a taxa do giroscópio. Quando a proteção conclui que o robô está travado:
///
/// - os motores são freados na hora (`parar()` no driver);
/// - todo `definirPotencia()` seguinte é **recusado** e vira freio, até `liberar()`. Assim o
///   controle não religa o motor preso no ciclo seguinte.
///
/// Uso típico, no ciclo da navegação:
/// @code
/// MotoresComProtecao motores(drv8833);
/// controle.atualizar(sensor, motores, dtMs);          // comanda como antes
/// if (motores.atualizarProtecao(medidas, dtMs) == EstadoProtecao::Travado) {
///   // decidir: recuar, avisar a telemetria (#192) e depois motores.liberar()
/// }
/// @endcode
class MotoresComProtecao : public IMotores {
 public:
  /// @param motores Driver real (ou `MotoresSimulados` nos testes).
  /// @param config Limites da proteção (`ConfiguracaoProtecaoMotor.h`).
  explicit MotoresComProtecao(IMotores& motores, const ConfiguracaoProtecaoMotor& config = ConfiguracaoProtecaoMotor{});

  /// Repassa a potência ao driver, já limitada. Com a proteção travada, freia em vez de repassar.
  void definirPotencia(const PotenciaRodas& potencia) override;

  /// Repassa o freio ao driver.
  void parar() override;

  /// Avança a proteção em um ciclo, com a última potência repassada. Se ela concluir que o robô
  /// está travado, freia os motores.
  /// @param medidas ToF frontal e taxa do giroscópio medidos neste ciclo.
  /// @param dtMs Tempo desde o ciclo anterior, em ms.
  EstadoProtecao atualizarProtecao(const MedidasProtecao& medidas, uint32_t dtMs);

  /// Destrava a proteção: os comandos voltam a chegar ao driver. Chame depois de a navegação
  /// decidir o que fazer com o robô preso.
  void liberar();

  /// `true` enquanto a proteção estiver travada (motores freados, comandos recusados).
  bool travado() const { return protecao_.travado(); }

  /// Estado da proteção depois do último `atualizarProtecao()`.
  EstadoProtecao estado() const { return protecao_.estado(); }

  /// Última potência que chegou ao driver (zero depois de um freio).
  PotenciaRodas ultimaPotencia() const { return ultima_; }

  /// Quantas vezes a proteção freou o robô por motor travado.
  uint32_t cortes() const { return cortes_; }

  /// Comandos de potência recusados porque a proteção estava travada.
  uint32_t comandosRecusados() const { return recusados_; }

 private:
  IMotores& motores_;
  ProtecaoMotorTravado protecao_;
  PotenciaRodas ultima_ = {0, 0};
  uint32_t cortes_ = 0;
  uint32_t recusados_ = 0;
};

}  // namespace micromouse
