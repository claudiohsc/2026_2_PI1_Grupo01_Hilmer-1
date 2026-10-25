#pragma once

/// @file IAcelerometro.h
/// @brief Interface de acesso ao acelerômetro da IMU, no plano do chão (camada `hal`).

#include <stdint.h>

namespace micromouse {

/// Fonte da aceleração do robô nos eixos do plano do chão (X e Y), usada para detectar impactos (#192).
///
/// Unidade: milésimos de g (1000 = 1 g). Com o robô parado e o sensor plano, a gravidade fica toda no
/// eixo Z e as duas componentes ficam perto de zero. Os eixos seguem a montagem do sensor e a
/// detecção só usa o módulo da variação, então o sinal e a orientação dos eixos não importam.
///
/// Implementações:
/// - `ImuSimulado` (`lib/simulacao`): aceleração definida pelo teste, sem hardware.
/// - `ImuMpu6050` (`lib/drivers_esp32`): o acelerômetro do MPU6050, no mesmo I²C do giroscópio.
class IAcelerometro {
 public:
  virtual ~IAcelerometro() = default;

  /// Lê a aceleração nos eixos X e Y.
  /// @param[out] axMg Recebe a aceleração em X, em mg.
  /// @param[out] ayMg Recebe a aceleração em Y, em mg.
  /// Os dois não mudam se a função retorna `false`.
  /// @return `false` se o sensor não respondeu ou a leitura é inválida; `true` caso contrário.
  virtual bool lerAceleracaoPlano(int16_t& axMg, int16_t& ayMg) = 0;
};

}  // namespace micromouse
