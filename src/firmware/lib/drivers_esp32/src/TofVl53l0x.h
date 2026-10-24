#pragma once

/// @file TofVl53l0x.h
/// @brief Driver dos 3 sensores de distância VL53L0X (esquerdo, frontal e direito) pelo I²C.
///
/// Implementa `ISensorDistancia` (camada `hal`), então mapa, percepção e navegação não mudam com a troca do
/// LiDAR pelos ToF (#344). A regra de validade da leitura fica no `CombinadorTof` (camada `tof`),
/// testado no computador; aqui só se fala com os sensores pela biblioteca `pololu/VL53L0X`.

#include <Arduino.h>
#include <VL53L0X.h>
#include <Wire.h>

#include "CombinadorTof.h"
#include "ConfiguracaoTof.h"
#include "ISensorDistancia.h"

namespace micromouse {

/// Ligação de um VL53L0X: o GPIO do XSHUT e o endereço I²C atribuído no boot.
struct ConexaoTof {
  uint8_t pinoXshut;  ///< GPIO ligado ao XSHUT do sensor (nível baixo = sensor em reset).
  uint8_t endereco;   ///< Endereço I²C (7 bits) atribuído ao sensor em `iniciar()`.
};

/// Driver dos 3 VL53L0X que compartilham o barramento I²C.
///
/// Todos os sensores saem do reset no mesmo endereço de fábrica (0x29). Por isso `iniciar()` põe
/// os três em reset (XSHUT baixo) e solta um por vez, mudando o endereço de cada um antes de soltar
/// o próximo. Depois disso, os três medem em modo contínuo.
///
/// `lerDistanciasLaterais()` nunca espera uma medida: a cada chamada, só lê os sensores que já têm
/// medida pronta e entrega a combinação das últimas amostras. A leitura falha (`false`) se algum
/// sensor falhou, está fora da faixa válida ou parou de medir (amostra mais velha que
/// `idadeMaximaLeituraMs`). As amostras brutas de cada sensor ficam em `amostra()`.
///
/// Exemplo (a montagem real fica em `main.cpp`):
/// @code
/// Wire.begin(pinos::I2C_SDA, pinos::I2C_SCL);
/// TofVl53l0x tof(Wire, {pinos::XSHUT_FRONTAL, pinos::ENDERECO_TOF_FRONTAL},
///                {pinos::XSHUT_ESQ, pinos::ENDERECO_TOF_ESQ},
///                {pinos::XSHUT_DIR, pinos::ENDERECO_TOF_DIR});
/// if (!tof.iniciar()) { /* algum sensor não respondeu: veja sensorIniciado() */ }
/// ISensorDistancia& sensor = tof;
/// @endcode
class TofVl53l0x : public ISensorDistancia {
 public:
  /// @param barramento Barramento I²C já iniciado (`Wire.begin(SDA, SCL)`), compartilhado com o
  ///        MPU6050.
  /// @param frente Ligação do sensor frontal.
  /// @param esquerda Ligação do sensor esquerdo.
  /// @param direita Ligação do sensor direito.
  /// @param config Tempo de medição, faixa válida e tempos de espera.
  TofVl53l0x(TwoWire& barramento, const ConexaoTof& frente, const ConexaoTof& esquerda,
             const ConexaoTof& direita, const ConfiguracaoTof& config = ConfiguracaoTof{});

  /// Reinicia os 3 sensores, atribui os endereços e liga o modo contínuo. Chame no `setup()`,
  /// depois de `Wire.begin()`. Pode ser chamado de novo para reiniciar os sensores.
  ///
  /// Um sensor que não responde fica em reset (XSHUT baixo), para não ocupar o endereço de fábrica
  /// e atrapalhar os seguintes.
  ///
  /// @return `true` se os 3 sensores iniciaram; `false` se algum falhou.
  bool iniciar();

  /// `true` se o sensor do `lado` iniciou na última chamada de `iniciar()`.
  bool sensorIniciado(Lado lado) const { return iniciados_[static_cast<uint8_t>(lado)]; }

  /// @copydoc ISensorDistancia::lerDistanciasLaterais
  bool lerDistanciasLaterais(DistanciasLaterais& saida) override;

  /// @copydoc ISensorDistancia::idadeUltimaLeituraMs
  /// Devolve `CombinadorTof::IDADE_DESCONHECIDA` se algum sensor ainda não entregou amostra.
  uint32_t idadeUltimaLeituraMs() const override;

  /// Última amostra bruta do sensor do `lado`, com o estado (válida, fora de alcance, etc.).
  const AmostraTof& amostra(Lado lado) const { return combinador_.amostra(lado); }

 private:
  /// Solta o XSHUT do sensor `indice`, muda o endereço e liga o modo contínuo.
  bool iniciarSensor(uint8_t indice);

  /// Lê a medida do sensor `indice`, se houver uma pronta, e a registra no combinador.
  void lerSensor(uint8_t indice, uint32_t agoraMs);

  TwoWire& barramento_;
  ConfiguracaoTof config_;
  ConexaoTof conexoes_[3];  ///< Indexado por `Lado`: frente, esquerda, direita.
  VL53L0X sensores_[3];
  bool iniciados_[3] = {false, false, false};
  CombinadorTof combinador_;
};

}  // namespace micromouse
