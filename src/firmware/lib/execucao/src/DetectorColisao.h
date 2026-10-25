#pragma once

/// @file DetectorColisao.h
/// @brief Detecta colisões pelo pico de aceleração da IMU e pelo ToF frontal parado (#192).

#include <stdint.h>

#include "ConfiguracaoColisao.h"
#include "ConfiguracaoProtecaoMotor.h"
#include "IMotores.h"
#include "ProtecaoMotorTravado.h"

namespace micromouse {

/// O que o detector recebe a cada ciclo da tarefa de navegação.
struct EntradaDeteccao {
  /// `true` enquanto o robô está sendo conduzido (avançando, girando ou realinhando). Parado, em
  /// recuperação ou sem permissão para andar, a detecção fica desligada: um robô erguido ou
  /// empurrado à mão não é uma colisão.
  bool ativo = false;
  /// `false` se o acelerômetro não respondeu neste ciclo (a detecção pelo impacto fica cega).
  bool aceleracaoValida = false;
  int16_t axMg = 0;  ///< Aceleração em X, em mg.
  int16_t ayMg = 0;  ///< Aceleração em Y, em mg.
  /// Potência que a navegação aplicou às rodas no ciclo anterior.
  PotenciaRodas potencia = {0, 0};
  /// ToF frontal e giroscópio do ciclo, os mesmos da proteção contra motor travado.
  MedidasProtecao medidas = {false, 0, 0};
  uint32_t dtMs = 0;  ///< Tempo desde o ciclo anterior, em ms.
};

/// Diz se o robô bateu, por duas evidências independentes:
///
/// 1. **Impacto (IMU):** a aceleração no plano (X e Y) se afasta, em módulo, da referência de repouso
///    por pelo menos `limiarImpactoMg` por `amostrasConfirmacao` leituras seguidas. A referência
///    é uma média lenta das leituras calmas, então a inclinação do sensor e o zero dele não contam.
/// 2. **Frente parada (ToF):** o robô recebe potência alta por `tempoLimiteMs` e nem o ToF frontal
///    nem o giroscópio mostram movimento (`ProtecaoMotorTravado`). Uma frente além do alcance do ToF
///    (leitura no máximo, 1200 mm) não conta: ela também não muda andando por um corredor longo.
///    Pega a batida que o acelerômetro não viu, como um encosto devagar, e o robô preso depois
///    dela. A detecção por encoder parado fica para quando os encoders existirem.
///
/// Cada colisão é avisada **uma vez**: o detector devolve o tipo no ciclo em que ela é percebida
/// e fica quieto até `ativo` voltar a `false` (a recuperação) ou `reiniciar()`.
///
/// Não depende do Arduino e não aloca memória: testado no ambiente `native`.
///
/// @code
/// DetectorColisao detector;
/// // a cada 10 ms, na tarefa de navegação:
/// EntradaDeteccao e;
/// e.ativo = cicloNavegacao.emMovimento();
/// e.aceleracaoValida = acelerometro.lerAceleracaoPlano(e.axMg, e.ayMg);
/// e.potencia = registrador.ultimaPotencia();
/// e.medidas = {tofValido, frenteMm, taxaGiro};
/// e.dtMs = 10;
/// entradaCiclo.colisao = detector.atualizar(e);
/// @endcode
class DetectorColisao {
 public:
  explicit DetectorColisao(const ConfiguracaoColisao& config = ConfiguracaoColisao{},
                           const ConfiguracaoProtecaoMotor& protecao = ConfiguracaoProtecaoMotor{});

  /// Avança a detecção em um ciclo e devolve o tipo da colisão percebida neste ciclo, ou `Nenhuma`.
  TipoColisao atualizar(const EntradaDeteccao& entrada);

  /// Esquece a colisão, a contagem do impacto e a janela do ToF (a referência de repouso fica).
  void reiniciar();

  /// Colisões percebidas desde o início.
  uint32_t colisoes() const { return colisoes_; }

  /// Tipo da última colisão percebida (`Nenhuma` se nunca houve).
  TipoColisao ultima() const { return ultima_; }

  /// Afastamento da última leitura em relação à referência de repouso, em mg (para o log e a
  /// bancada: é o número que se compara com `limiarImpactoMg`).
  uint32_t desvioMg() const { return desvioMg_; }

 private:
  void aprenderReferencia(const EntradaDeteccao& entrada);

  ConfiguracaoColisao config_;
  ProtecaoMotorTravado protecao_;
  bool temReferencia_ = false;
  int32_t referenciaXMg_ = 0;
  int32_t referenciaYMg_ = 0;
  uint8_t acimaDoLimite_ = 0;
  bool avisada_ = false;
  uint32_t colisoes_ = 0;
  TipoColisao ultima_ = TipoColisao::Nenhuma;
  uint32_t desvioMg_ = 0;
};

}  // namespace micromouse
