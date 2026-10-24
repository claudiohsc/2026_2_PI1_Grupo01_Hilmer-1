#pragma once

/// @file RotinaCalibracao.h
/// @brief Orquestração da autocalibração dos VL53L0X e do giroscópio dentro de 1 s (#240, HU-13 / RF-13 / RNF-08).

#include <stdint.h>

#include "CalibradorGiro.h"
#include "CalibradorTof.h"
#include "ConfiguracaoCalibracao.h"
#include "IImu.h"
#include "ISensorDistancia.h"
#include "Tipos.h"

namespace micromouse {

/// Estado da rotina unificada de autocalibração.
enum class EstadoRotinaCalibracao : uint8_t {
  NaoIniciada,           ///< Rotina ainda não foi iniciada.
  EmAndamento,           ///< Coletando amostras dos sensores dentro do orçamento.
  Sucesso,               ///< Calibração concluída com sucesso dentro de 1 s (RNF-08).
  RecusadaPorMovimento,  ///< Robô se mexeu durante a calibração (variação excessiva no ToF ou giro).
  TempoExcedido,         ///< Duração estourou o orçamento de 1 s (RNF-08).
  FalhaSensor            ///< Sensor não respondeu ou leitura inválida durante a calibração.
};

/// Orquestra a autocalibração simultânea dos sensores de distância (VL53L0X) e inerciais (MPU6050).
///
/// Atende aos critérios da issue #240:
/// 1. Viés do giroscópio calculado por média de amostras; aborta se o robô se mover.
/// 2. Desvio por sensor ToF guardado e aplicado nas leituras.
/// 3. Duração medida e estritamente contida no orçamento de 1 s do RNF-08.
/// 4. Testável sem hardware com `ImuSimulado` e leituras simuladas de ToF.
class RotinaCalibracao {
 public:
  explicit RotinaCalibracao(const ConfiguracaoRotinaCalibracao& config = ConfiguracaoRotinaCalibracao{});

  /// Inicia a rotina unificada com as referências nominais padrão da configuração.
  void iniciar();

  /// Inicia a rotina com distâncias de referência explícitas para os ToF.
  void iniciar(const ReferenciasTof& referencias);

  /// Atualiza a rotina com uma iteração de tempo e amostras dos sensores.
  /// @param distancias Leitura atual dos ToF (se `distanciasValidas` for true).
  /// @param distanciasValidas `true` se houve nova leitura válida de distância neste passo.
  /// @param taxaGiroDecimosDps Leitura atual da taxa de giro (eixo Z).
  /// @param giroValido `true` se a leitura da IMU foi válida.
  /// @param dtMs Intervalo de tempo decorrido neste passo, em ms.
  /// @return Situação da rotina após o passo.
  EstadoRotinaCalibracao atualizar(const DistanciasLaterais& distancias, bool distanciasValidas,
                                  int16_t taxaGiroDecimosDps, bool giroValido, uint32_t dtMs);

  /// Executa o laço de calibração alimentando-se das interfaces de hardware/simulação.
  /// @param sensorDistancia Fonte das leituras ToF (`ISensorDistancia` / `SensorDistanciaSimulado`).
  /// @param imu Fonte da taxa de giro (`IImu` / `ImuSimulado`).
  /// @param dtPassoMs Passo de tempo simulado entre leituras da IMU (padrão 10 ms).
  /// @return Situação final da calibração.
  EstadoRotinaCalibracao executar(ISensorDistancia& sensorDistancia, IImu& imu,
                                 uint32_t dtPassoMs = PERIODO_AMOSTRAGEM_GIRO_MS);

  /// Estado atual da rotina.
  EstadoRotinaCalibracao estado() const { return estado_; }

  /// Duração total medida da rotina, em ms.
  uint32_t duracaoMedidaMs() const { return duracaoMedidaMs_; }

  /// Acesso ao calibrador dos sensores ToF.
  const CalibradorTof& calibradorTof() const { return calibradorTof_; }

  /// Acesso ao calibrador do giroscópio.
  const CalibradorGiro& calibradorGiro() const { return calibradorGiro_; }

  /// Desvios dos sensores ToF em mm.
  DesviosTof desviosTof() const { return calibradorTof_.desvios(); }

  /// Viés do giroscópio em décimos de °/s.
  int16_t viesGiroDecimosDps() const { return calibradorGiro_.viesDecimosDps(); }

  /// Aplica a correção de desvio a uma leitura bruta de ToF.
  DistanciasLaterais corrigirTof(const DistanciasLaterais& bruta) const {
    return calibradorTof_.corrigir(bruta);
  }

  /// Aplica a correção de viés à taxa de giro bruta.
  int16_t corrigirGiro(int16_t taxaBrutaDecimosDps) const {
    return calibradorGiro_.corrigir(taxaBrutaDecimosDps);
  }

 private:
  ConfiguracaoRotinaCalibracao config_;
  EstadoRotinaCalibracao estado_ = EstadoRotinaCalibracao::NaoIniciada;
  CalibradorTof calibradorTof_;
  CalibradorGiro calibradorGiro_;
  uint32_t duracaoMedidaMs_ = 0;
};

}  // namespace micromouse

