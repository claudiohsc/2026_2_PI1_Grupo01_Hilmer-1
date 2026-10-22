#pragma once

/// @file ILidar.h
/// @brief Interface de acesso aos sensores de distância (camada `hal`).
///
/// O nome vem do LiDAR da primeira versão do hardware; na Rev 2.0 são 3 sensores VL53L0X.
/// A lógica do firmware depende só desta interface, nunca do driver concreto.
/// Assim, a percepção e o mapeamento podem ser testados no computador com o
/// `LidarSimulado` e, na placa, usar o driver real (em `drivers_esp32`).

#include "Tipos.h"

namespace micromouse {

/// Fonte de leituras de distância nas três direções relativas ao robô.
///
/// Implementações:
/// - `LidarSimulado` (`lib/simulacao`): leituras configuradas pelo teste, sem hardware.
/// - `TofVl53l0x` (`lib/drivers_esp32`): os 3 VL53L0X da Rev 2.0, pelo I²C.
/// - `LidarUart` (`lib/lidar`): LiDAR 360° por UART, sem uso desde a Rev 2.0.
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

  /// Idade, em ms, da amostra mais antiga que compõe a última leitura, medida agora.
  /// Serve para a navegação não reagir duas vezes ao mesmo dado: se a idade não diminuiu
  /// desde a chamada anterior, a leitura é a mesma.
  ///
  /// A implementação padrão devolve 0 (leitura sempre nova), o que vale para fontes sem
  /// relógio, como o `LidarSimulado`.
  virtual uint32_t idadeUltimaLeituraMs() const { return 0; }
};

}  // namespace micromouse
