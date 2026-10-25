#pragma once

/// @file TravaSecaoCritica.h
/// @brief `ITrava` real: seção crítica do FreeRTOS (#238).

#include <freertos/FreeRTOS.h>

#include "ITrava.h"

namespace micromouse {

/// Trava curta entre tarefas, pela seção crítica do FreeRTOS (`portENTER_CRITICAL`).
///
/// Enquanto travada, as interrupções e a troca de tarefas ficam suspensas, então só serve para
/// cópias rápidas de memória, como a do `ControladorEnvio` (no máximo 100 amostras, cerca de
/// 2,4 KB). Nunca segure esta trava durante uma chamada de rede, um acesso ao I²C ou qualquer espera.
class TravaSecaoCritica : public ITrava {
 public:
  void travar() override { portENTER_CRITICAL(&mux_); }
  void destravar() override { portEXIT_CRITICAL(&mux_); }

 private:
  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};

}  // namespace micromouse
