#pragma once

/// @file ExecutorMovimentos.h
/// @brief Executa os movimentos entre células (#184): avançar uma célula, girar 90° e girar 180°.

#include <stdint.h>

#include "AcompanhadorGiro.h"
#include "ConfiguracaoControle.h"
#include "ConfiguracaoExecucao.h"
#include "ConfiguracaoGiro.h"
#include "ControleGiro.h"
#include "ControleMovimento.h"
#include "IMotores.h"
#include "Movimento.h"
#include "Tipos.h"

namespace micromouse {

/// Situação do movimento depois da última atualização.
enum class EstadoExecucao : uint8_t {
  Ocioso,      ///< Nenhum movimento iniciado (ou o último foi cancelado).
  Executando,  ///< Movimento em andamento.
  Concluido,   ///< Movimento terminado: o robô está no centro da célula de destino ou no rumo novo.
  Falha        ///< Movimento interrompido; o motivo está em `falha()`. Os motores ficam freados.
};

/// Por que o último movimento falhou.
enum class FalhaExecucao : uint8_t {
  Nenhuma,
  ParedeNaFrente,     ///< O avanço parou numa parede antes do centro de destino.
  SemLeituraTof,      ///< Sem leitura nova do ToF por `tempoMaximoSemLeituraMs` durante o avanço.
  TempoEsgotado,      ///< O avanço passou de `tempoMaximoAvancoMs`.
  GiroTempoEsgotado   ///< O giro não chegou ao rumo no tempo limite (inclusive sem giroscópio válido).
};

/// O que o executor recebe a cada ciclo da navegação.
struct EntradaExecucao {
  /// Leitura nova do ToF, medida a partir da face de cada sensor, ou `nullptr` se não houver uma
  /// neste ciclo. Cada leitura deve vir uma vez só.
  const DistanciasLaterais* leituraNova = nullptr;
  /// Última leitura do giroscópio (ângulo e validade).
  LeituraGiro giro;
  /// Posição do robô ao longo do eixo para onde ele está virado, em µm, com o centro da célula 0
  /// em 0 (`posicaoAoLongoDoAvancoUm(odometria)`).
  int64_t posicaoAvancoUm = 0;
  /// Tempo desde o ciclo anterior, em ms.
  uint32_t dtMs = 0;
};

/// Converte o comando decidido pela navegação em potência para as rodas, um movimento por vez.
///
/// - **Avançar uma célula:** o destino é o centro da célula seguinte à mais próxima da posição
///   atual (assim um erro pequeno de parada não se acumula). O `ControleMovimento` centraliza no
///   corredor e mantém o rumo pelo giroscópio; perto do destino, o teto da potência base faz a
///   rampa de desaceleração, e o avanço termina a `toleranciaChegadaMm` do centro. A parada pela
///   parede da frente continua valendo e, perto do destino, também conclui.
/// - **Girar 90° ou 180°:** o `ControleGiro` gira até o **rumo alvo absoluto** (múltiplos de 90°
///   desde a largada), e não 90° a partir do ângulo atual: o erro de um giro não passa para o
///   próximo.
///
/// O ToF mede a partir da face de cada sensor; antes de ir para o `ControleMovimento`, que usa
/// distâncias a partir do centro do robô, a leitura recebe o recuo de cada sensor.
///
/// A fonte da posição não importa: com os encoders instalados, só muda quem calcula
/// `posicaoAvancoUm`. Não depende do Arduino e não aloca memória: testado no ambiente `native`.
///
/// @code
/// ExecutorMovimentos executor;
/// executor.iniciar(Movimento::Girar90Direita);
/// // a cada 10 ms, na tarefa de navegação:
/// EntradaExecucao entrada{leituraNova, lerGiro(), posicaoAoLongoDoAvancoUm(odometria), 10};
/// if (executor.atualizar(entrada, motoresTracao) == EstadoExecucao::Concluido) { /* próximo */ }
/// @endcode
class ExecutorMovimentos {
 public:
  explicit ExecutorMovimentos(const ConfiguracaoExecucao& config = ConfiguracaoExecucao{},
                              const ConfiguracaoControle& controle = ConfiguracaoControle{},
                              const ConfiguracaoGiro& giro = ConfiguracaoGiro{});

  /// Começa um movimento. O primeiro `atualizar()` fixa o destino (avanço) ou o ângulo de partida
  /// (giro). Um giro já soma a variação ao rumo alvo aqui.
  /// @return `false` se outro movimento está em andamento (nada muda).
  bool iniciar(Movimento movimento);

  /// Um ciclo do movimento em andamento: calcula e aplica a potência nos motores.
  /// Fora de `Executando`, não faz nada e devolve o estado atual.
  EstadoExecucao atualizar(const EntradaExecucao& entrada, IMotores& motores);

  /// Interrompe o movimento em andamento, freia os motores e volta a `Ocioso`. Um giro
  /// interrompido não desfaz a mudança do rumo alvo: quem cancela ajusta com `definirRumoAlvo()`.
  void cancelar(IMotores& motores);

  /// Define o rumo alvo absoluto, em mili-graus do giroscópio (por exemplo, na largada).
  void definirRumoAlvo(int32_t rumoMiligraus) { rumoAlvoMiligraus_ = rumoMiligraus; }

  /// Rumo que o robô deve ter ao fim do giro e durante o avanço, em mili-graus.
  int32_t rumoAlvoMiligraus() const { return rumoAlvoMiligraus_; }

  EstadoExecucao estado() const { return estado_; }
  FalhaExecucao falha() const { return falha_; }
  Movimento movimento() const { return movimento_; }

  /// `true` enquanto há um movimento em andamento.
  bool ocupado() const { return estado_ == EstadoExecucao::Executando; }

  /// Quanto falta até o centro de destino, em mm (negativo se passou). Zero fora do avanço ou antes
  /// do primeiro `atualizar()`. Depois do fim, guarda o valor da parada: é o erro de chegada que
  /// a bancada registra.
  int32_t restanteMm() const { return static_cast<int32_t>(restanteUm_ / 1000); }

  /// Tempo do movimento atual (ou do último), em ms.
  uint32_t tempoMs() const { return tempoMs_; }

  /// Controles internos, para a telemetria, a bancada e os testes.
  const ControleMovimento& controleMovimento() const { return controleMovimento_; }
  const ControleGiro& controleGiro() const { return controleGiro_; }

  /// A leitura do ToF convertida para distâncias a partir do centro do robô (veja a classe).
  DistanciasLaterais paraOCentro(const DistanciasLaterais& leitura) const;

 private:
  EstadoExecucao atualizarAvanco(const EntradaExecucao& entrada, IMotores& motores);
  EstadoExecucao atualizarGiro(const EntradaExecucao& entrada, IMotores& motores);

  /// Teto da potência base para a distância que falta até o destino (a rampa de desaceleração).
  int16_t tetoParaRestante(int64_t restanteUm) const;

  EstadoExecucao terminar(EstadoExecucao estado, FalhaExecucao falha, IMotores& motores);

  ConfiguracaoExecucao config_;
  ConfiguracaoControle configControle_;
  ConfiguracaoGiro configGiro_;
  ControleMovimento controleMovimento_;
  ControleGiro controleGiro_;

  EstadoExecucao estado_ = EstadoExecucao::Ocioso;
  FalhaExecucao falha_ = FalhaExecucao::Nenhuma;
  Movimento movimento_ = Movimento::AvancarCelula;
  int32_t rumoAlvoMiligraus_ = 0;

  bool comecou_ = false;              ///< `false` até o primeiro `atualizar()` do movimento.
  uint32_t tempoMs_ = 0;
  int64_t destinoUm_ = 0;             ///< Centro da célula de destino, no eixo do avanço.
  int64_t restanteUm_ = 0;
  uint32_t semLeituraMs_ = 0;         ///< Tempo desde a última leitura nova do ToF.
  uint32_t desdeCalculoMs_ = 0;       ///< Tempo desde o último cálculo do `ControleMovimento`.
  bool temPotencia_ = false;          ///< `false` até a primeira leitura do avanço.
  PotenciaRodas ultimaPotencia_ = {0, 0};
};

/// Nome do estado para o log: `ocioso`, `executando`, `concluido` ou `falha`.
const char* nomeDoEstadoExecucao(EstadoExecucao estado);

/// Nome da falha para o log: `nenhuma`, `parede_na_frente`, `sem_leitura_tof`, `tempo_esgotado` ou
/// `giro_tempo_esgotado`.
const char* nomeDaFalhaExecucao(FalhaExecucao falha);

}  // namespace micromouse
