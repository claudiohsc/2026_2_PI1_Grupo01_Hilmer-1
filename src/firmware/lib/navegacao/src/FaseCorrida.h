#pragma once

/// @file FaseCorrida.h
/// @brief As fases do robô na telemetria: calibrando, aguardando a largada, em corrida e encerrada (#242).

#include <stdint.h>

#include "AmostraTelemetria.h"
#include "ConfiguracaoLargada.h"

namespace micromouse {

/// O que fazer com a amostra de telemetria deste ciclo.
struct DecisaoColeta {
  bool registrar = false;                         ///< `true` se há amostra para guardar neste ciclo.
  EstadoCorrida estado = EstadoCorrida::Aguardando;  ///< `estado_corrida` da amostra.
  uint32_t tempoDecorridoMs = 0;                  ///< `tempo_decorrido`: zero até a largada, depois conta dela.
  bool largouAgora = false;                       ///< `true` no ciclo em que o robô largou.
  bool ultima = false;                            ///< `true` na última amostra (objetivo ou fim do tempo): encerrar a corrida.
  bool tempoEsgotado = false;                     ///< `true` se a última amostra é por fim do tempo (10 min).
};

/// Fase do robô para a telemetria e para o dashboard.
///
/// ```
/// AguardandoCalibracao --calibracaoConcluida--> AguardandoLargada --largou--> EmCorrida --objetivo ou 10 min--> Encerrada
/// ```
///
/// - **Aguardando calibração:** nada é enviado; a corrida nem é criada no servidor.
/// - **Aguardando largada (#242):** depois de uma calibração bem-sucedida, o robô envia amostras com
///   `estado_corrida = aguardando`, na célula de partida, com tempo e velocidade zerados (o dashboard
///   mostra o Frame 01_Idle). O ritmo é baixo (`periodoAmostraAguardandoMs`).
/// - **Largada:** o deslocamento desde a partida passa de `deslocamentoMinimoMm`. O cronômetro dos 10
///   minutos começa aí, e não na criação da corrida.
/// - **Em corrida:** uma amostra por ciclo, `explorando`, até o objetivo ou o fim do tempo.
///
/// Sem Arduino e sem alocação: roda igual no computador e na placa. Quem chama protege a instância
/// com a seção crítica, porque a tarefa de rede e a de telemetria a usam.
class FaseCorrida {
 public:
  enum class Fase : uint8_t { AguardandoCalibracao, AguardandoLargada, EmCorrida, Encerrada };

  explicit FaseCorrida(const ConfiguracaoLargada& config = ConfiguracaoLargada{}) : config_(config) {}

  Fase fase() const { return fase_; }

  /// `true` se a calibração já terminou bem: a corrida pode ser criada no servidor.
  bool calibrado() const { return fase_ != Fase::AguardandoCalibracao; }

  /// A calibração do boot terminou bem (ou o giroscópio calibrou sozinho): passa a aguardar a largada.
  /// Fora de `AguardandoCalibracao`, não faz nada.
  void calibracaoConcluida();

  /// Decide o que registrar neste ciclo.
  /// @param deslocamentoMm Deslocamento desde a partida (o valor absoluto é o que conta).
  /// @param agoraMs `millis()` do ciclo.
  /// @param chegouAoObjetivo `true` no ciclo em que o detector de objetivo disparou.
  DecisaoColeta decidir(int32_t deslocamentoMm, uint32_t agoraMs, bool chegouAoObjetivo);

  /// Volta a aguardar a largada (por exemplo, o robô foi recolocado na partida à mão, comando `Z`).
  /// Uma corrida encerrada continua encerrada, e sem calibração continua sem calibração.
  void reiniciar();

 private:
  ConfiguracaoLargada config_;
  Fase fase_ = Fase::AguardandoCalibracao;
  uint32_t inicioMs_ = 0;           ///< `millis()` da largada.
  uint32_t ultimaEsperaMs_ = 0;     ///< `millis()` da última amostra de espera.
  bool temUltimaEspera_ = false;
};

/// Nome da fase para o log: `aguardando_calibracao`, `aguardando_largada`, `em_corrida` ou `encerrada`.
const char* nomeDaFase(FaseCorrida::Fase fase);

}  // namespace micromouse
