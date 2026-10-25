#pragma once

/// @file ControleGiro.h
/// @brief Controle de giro no próprio eixo (90° e 180°) em malha fechada com giroscópio (#190).
///
/// Utiliza o ângulo de guinada (yaw) integrado a partir da IMU (MPU6050) para realizar
/// rotações precisas no lugar, compensando o escorregamento dos pneus e irregularidades do piso (HU-11).

#include <stdint.h>

#include "AcompanhadorGiro.h"
#include "ConfiguracaoGiro.h"
#include "IMotores.h"
#include "Tipos.h"

namespace micromouse {

/// Estado do giro no próprio eixo.
enum class EstadoGiro : uint8_t {
  Inativo,    ///< Nenhum giro em andamento.
  Girando,    ///< Giro em malha fechada em execução.
  Concluido,  ///< Giro finalizado com sucesso dentro da tolerância especificada.
  Timeout     ///< Tempo limite excedido antes de atingir o ângulo desejado.
};

/// Constantes para ângulos típicos de manobra em mili-graus.
constexpr int32_t ANGULO_GIRO_90_MILIGRAUS = 90000;
constexpr int32_t ANGULO_GIRO_180_MILIGRAUS = 180000;

/// Controlador em malha fechada para rotação no lugar.
///
/// Calcula o erro angular entre a orientação alvo e a medição do giroscópio, modulando
/// a potência diferencial das rodas para acelerar, frear progressivamente ao se aproximar
/// do alvo e parar quando dentro da tolerância configurável.
///
/// Não depende do Arduino e não aloca memória dinâmica: testado no ambiente `native`.
class ControleGiro {
 public:
  explicit ControleGiro(const ConfiguracaoGiro& config = ConfiguracaoGiro{});

  /// Inicia um novo giro relativo ao ângulo atual.
  /// @param anguloRelativoMiligraus Deslocamento angular desejado (+ para esquerda, - para direita).
  /// @param anguloAtualMiligraus Leitura angular atual da IMU em mili-graus.
  void iniciar(int32_t anguloRelativoMiligraus, int32_t anguloAtualMiligraus);

  /// Inicia um giro de 90° para a esquerda (anti-horário).
  void iniciarGiro90Esquerda(int32_t anguloAtualMiligraus) {
    iniciar(ANGULO_GIRO_90_MILIGRAUS, anguloAtualMiligraus);
  }

  /// Inicia um giro de 90° para a direita (horário).
  void iniciarGiro90Direita(int32_t anguloAtualMiligraus) {
    iniciar(-ANGULO_GIRO_90_MILIGRAUS, anguloAtualMiligraus);
  }

  /// Inicia um giro de 180° (meia volta).
  /// @param anguloAtualMiligraus Leitura angular atual da IMU.
  /// @param paraEsquerda `true` para rotação pela esquerda (+180°), `false` para direita (-180°).
  void iniciarGiro180(int32_t anguloAtualMiligraus, bool paraEsquerda = true) {
    iniciar(paraEsquerda ? ANGULO_GIRO_180_MILIGRAUS : -ANGULO_GIRO_180_MILIGRAUS, anguloAtualMiligraus);
  }

  /// Calcula a potência diferencial para as rodas neste ciclo.
  /// @param anguloAtualMiligraus Leitura atual do giroscópio em mili-graus.
  /// @param dtMs Intervalo de tempo decorrido desde o ciclo anterior.
  /// @return Potências `{esquerda, direita}` calculadas. `{0, 0}` se inativo, concluído ou timeout.
  PotenciaRodas calcular(int32_t anguloAtualMiligraus, uint32_t dtMs);

  /// Atualiza o controle a partir do ângulo numérico e aplica o comando aos atuadores.
  /// @param anguloAtualMiligraus Leitura atual do giroscópio.
  /// @param motores Interface de controle dos motores.
  /// @param dtMs Intervalo de tempo em milissegundos.
  /// @return Estado atual do giro.
  EstadoGiro atualizar(int32_t anguloAtualMiligraus, IMotores& motores, uint32_t dtMs);

  /// Atualiza o controle a partir da leitura completa do giroscópio.
  /// Se a leitura estiver inválida (`!giro.valida`), mantém o avanço do tempo limite mas não
  /// aplica potências para evitar comandos descontrolados.
  /// @param giro Estrutura com leitura do giroscópio e indicador de validade.
  /// @param motores Interface de controle dos motores.
  /// @param dtMs Intervalo de tempo em milissegundos.
  /// @return Estado atual do giro.
  EstadoGiro atualizar(const LeituraGiro& giro, IMotores& motores, uint32_t dtMs);

  /// Cancela o giro imediatamente e reseta o controlador para Inativo.
  void cancelar();

  /// Estado atual do controlador.
  EstadoGiro estado() const { return estado_; }

  /// `true` se o giro foi concluído com sucesso dentro da tolerância.
  bool concluido() const { return estado_ == EstadoGiro::Concluido; }

  /// `true` se há um giro ativo em andamento.
  bool emAndamento() const { return estado_ == EstadoGiro::Girando; }

  /// Ângulo alvo absoluto em mili-graus.
  int32_t anguloAlvoMiligraus() const { return anguloAlvoMiligraus_; }

  /// Erro angular instantâneo em mili-graus (alvo - atual).
  int32_t erroMiligraus() const { return erroMiligraus_; }

  /// Tempo total decorrido desde o início do giro atual em ms.
  uint32_t tempoDecorridoMs() const { return tempoDecorridoMs_; }

  /// Configuração ativa do controle de giro.
  const ConfiguracaoGiro& configuracao() const { return config_; }

 private:
  ConfiguracaoGiro config_;
  EstadoGiro estado_ = EstadoGiro::Inativo;
  int32_t anguloAlvoMiligraus_ = 0;
  int32_t erroMiligraus_ = 0;
  uint32_t tempoDecorridoMs_ = 0;
  uint8_t ciclosNaTolerancia_ = 0;
};

}  // namespace micromouse

