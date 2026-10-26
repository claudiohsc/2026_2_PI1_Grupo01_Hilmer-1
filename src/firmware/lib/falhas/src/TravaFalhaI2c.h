#pragma once

/// @file TravaFalhaI2c.h
/// @brief Trava os motores enquanto houver falha no I²C, sem soltar a trava do boot (#241).

#include <stdint.h>

#include "MotoresComTrava.h"

namespace micromouse {

/// O que `TravaFalhaI2c::aplicar` mudou na trava, para o log.
enum class MudancaTrava : uint8_t {
  Nenhuma,
  Travou,  ///< A falha do I²C travou os motores.
  Soltou   ///< A falha acabou, e a trava que ela pôs foi solta.
};

/// Usa a mesma `MotoresComTrava` do autoteste do boot (#50) para parar os motores enquanto o
/// `SupervisorI2c` pedir (`motoresDevemParar()`).
///
/// Só solta a trava que ela mesma pôs: se o boot reprovou, os motores já estão travados, e
/// continuam travados até o robô ser reiniciado, mesmo depois de a falha do I²C acabar.
///
/// @code
/// MudancaTrava m = travaFalha.aplicar(supervisor.motoresDevemParar(), motoresComTrava);
/// if (m == MudancaTrava::Travou) { Serial.println("{\"status\":\"motores_travados_i2c\"}"); }
/// @endcode
class TravaFalhaI2c {
 public:
  /// Trava, se `deveParar` e a trava ainda está solta; solta, se não precisa mais e foi ela que travou.
  MudancaTrava aplicar(bool deveParar, MotoresComTrava& trava) {
    if (deveParar) {
      if (travouPorFalha_ || trava.travado()) {
        return MudancaTrava::Nenhuma;
      }
      trava.travar();
      travouPorFalha_ = true;
      return MudancaTrava::Travou;
    }
    if (!travouPorFalha_) {
      return MudancaTrava::Nenhuma;
    }
    trava.liberar();
    travouPorFalha_ = false;
    return MudancaTrava::Soltou;
  }

  /// `true` enquanto a trava que está valendo foi posta por uma falha do I²C.
  bool travouPorFalha() const { return travouPorFalha_; }

 private:
  bool travouPorFalha_ = false;
};

}  // namespace micromouse
