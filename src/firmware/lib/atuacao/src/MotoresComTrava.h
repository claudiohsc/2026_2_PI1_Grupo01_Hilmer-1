#pragma once

/// @file MotoresComTrava.h
/// @brief `IMotores` que só deixa as rodas andarem depois de liberado (#50).

#include "IMotores.h"

namespace micromouse {

/// Envolve o driver dos motores e mantém o robô freado enquanto estiver travado.
///
/// Começa **travado**: o boot só chama `liberar()` depois do autoteste aprovado (#50). Travado,
/// todo `definirPotencia` vira `parar()` no driver; `parar()` sempre passa. Quem aciona as rodas
/// (controle, detector do objetivo) recebe este objeto em vez do driver, então nenhuma lógica nova
/// precisa saber da trava. A #241 (falha de I²C em corrida) e a #341 (motor travado) podem usar
/// `travar()` do mesmo jeito.
///
/// @code
/// MotoresComTrava tracao(motoresDrv8833);   // travado
/// tracao.definirPotencia({400, 400});         // driver recebe parar()
/// if (resultado.aprovado()) { tracao.liberar(); }
/// tracao.definirPotencia({400, 400});         // agora chega ao driver
/// @endcode
class MotoresComTrava : public IMotores {
 public:
  /// @param reais Driver dos motores (precisa continuar existindo).
  explicit MotoresComTrava(IMotores& reais) : reais_(reais) {}

  /// @copydoc IMotores::definirPotencia
  /// Travado, freia em vez de aplicar a potência.
  void definirPotencia(const PotenciaRodas& potencia) override {
    if (travado_) {
      reais_.parar();
      return;
    }
    reais_.definirPotencia(potencia);
  }

  /// @copydoc IMotores::parar
  void parar() override { reais_.parar(); }

  /// Freia e passa a recusar potência até `liberar()`.
  void travar() {
    travado_ = true;
    reais_.parar();
  }

  /// Volta a repassar a potência ao driver. Não aciona nada sozinho.
  void liberar() { travado_ = false; }

  /// `true` enquanto a potência for recusada.
  bool travado() const { return travado_; }

 private:
  IMotores& reais_;
  bool travado_ = true;
};

}  // namespace micromouse
