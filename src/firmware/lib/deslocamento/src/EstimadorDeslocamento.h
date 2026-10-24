#pragma once

/// @file EstimadorDeslocamento.h
/// @brief Estima o deslocamento de cada roda pela potência comandada vezes o tempo (#380).

#include <stdint.h>

#include "ConfiguracaoDeslocamento.h"
#include "IMotores.h"

namespace micromouse {

/// Roda de tração.
enum class Roda : uint8_t { Esquerda, Direita };

/// Enquanto o robô não tem encoder, faz o papel deles: a cada `atualizar(dtMs)`, cada roda avança
/// a velocidade da `TABELA_VELOCIDADE` para a potência comandada vezes o tempo. O resultado sai em
/// pulsos, na mesma unidade da `Odometria`, pelos `EncoderEstimado`.
///
/// Limites (documentados no README): a velocidade não é medida, então patinagem, bateria fraca ou
/// uma tabela errada viram erro de posição; a roda para no instante do comando, sem inércia. As
/// correções pelo ToF (`corrigirAvanco`) seguram esse erro.
///
/// Exemplo:
/// @code
/// EstimadorDeslocamento estimador;
/// estimador.comandar({400, 400});        // normalmente pelo MotoresComEstimativa
/// estimador.atualizar(10);               // a cada 10 ms
/// int32_t pulsos = estimador.contagem(Roda::Esquerda);
/// @endcode
class EstimadorDeslocamento {
 public:
  explicit EstimadorDeslocamento(const ConfiguracaoDeslocamento& config = ConfiguracaoDeslocamento{});

  /// Velocidade da roda, em mm/s, para uma potência (negativa = ré), pela tabela de calibração.
  int32_t velocidadeMmPorS(int16_t potencia) const;

  /// Registra a potência comandada às rodas, que vale até o próximo comando.
  void comandar(const PotenciaRodas& potencia);

  /// Registra que os motores foram freados: as rodas param na hora.
  void parar() { comandar({0, 0}); }

  /// Avança as rodas pelo tempo decorrido desde a última chamada, com a potência atual.
  void atualizar(uint32_t dtMs);

  /// Soma `deltaUm` micrômetros às duas rodas: move a posição estimada ao longo do eixo de
  /// avanço, sem girar. É como as correções pelo ToF chegam à `Odometria`. Conta como correção
  /// para `distanciaSemCorrecaoMm()`.
  void corrigirAvanco(int32_t deltaUm);

  /// Marca que a posição foi conferida pelo ToF sem precisar de ajuste.
  void registrarConferencia() { semCorrecaoUm_ = 0; }

  /// Pulsos acumulados da roda (a contagem do `IEncoder`).
  int32_t contagem(Roda roda) const;

  /// Zera a contagem de uma roda (o `zerar()` do `IEncoder`).
  void zerar(Roda roda);

  /// Última potência comandada.
  PotenciaRodas potencia() const { return potencia_; }

  /// `true` se as rodas foram comandadas em sentidos opostos (giro no lugar). Durante um giro,
  /// as leituras do ToF não servem de referência de posição.
  bool girando() const;

  /// Quanto o robô andou, em mm, desde a última correção ou conferência pelo ToF.
  uint32_t distanciaSemCorrecaoMm() const { return static_cast<uint32_t>(semCorrecaoUm_ / 1000); }

  /// `true` enquanto o robô não andou mais que `distanciaMaximaSemCorrecaoMm` sem correção.
  bool poseConfiavel() const {
    return distanciaSemCorrecaoMm() <= config_.distanciaMaximaSemCorrecaoMm;
  }

  const ConfiguracaoDeslocamento& configuracao() const { return config_; }

 private:
  static uint8_t indice(Roda roda) { return static_cast<uint8_t>(roda); }

  ConfiguracaoDeslocamento config_;
  PotenciaRodas potencia_ = {0, 0};
  int64_t posicaoUm_[2] = {0, 0};  ///< Distância acumulada de cada roda, em µm.
  uint64_t semCorrecaoUm_ = 0;     ///< Avanço do centro desde a última correção, em µm.
};

}  // namespace micromouse
