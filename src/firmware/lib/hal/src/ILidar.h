#pragma once

/// @file ILidar.h
/// @brief Interface de acesso ao sensor LiDAR (camada `hal`).
///
/// A lógica do firmware depende só desta interface, nunca do driver concreto.
/// Assim, a percepção e o mapeamento podem ser testados no computador com o
/// `LidarSimulado` e, na placa, usar o driver UART real (em `drivers_esp32`).

#include "Tipos.h"

namespace micromouse {

/// Fonte de leituras de distância nas três direções relativas ao robô.
///
/// Implementações:
/// - `LidarSimulado` (`lib/simulacao`): leituras configuradas pelo teste, sem hardware.
/// - Driver do LiDAR por UART (`lib/drivers_esp32`): a ser criado.
class ILidar {
 public:
  virtual ~ILidar() = default;

  /// Lê as distâncias à frente, à esquerda e à direita do robô.
  ///
  /// @param[out] saida Recebe as distâncias, em milímetros, quando a leitura é válida.
  /// @return `true` se houve leitura válida das três direções; `false` se o sensor
  ///         não respondeu ou a leitura é inválida. Quando retorna `false`, o conteúdo
  ///         de `saida` não deve ser usado.
  virtual bool lerDistanciasLaterais(DistanciasLaterais& saida) = 0;
};

}  // namespace micromouse
