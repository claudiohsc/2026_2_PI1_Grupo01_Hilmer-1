#pragma once

/// @file EncoderEstimado.h
/// @brief `IEncoder` provisório (#380): a contagem vem do `EstimadorDeslocamento`, não de pulsos.

#include "EstimadorDeslocamento.h"
#include "IEncoder.h"

namespace micromouse {

/// Encoder de uma roda cuja contagem é estimada pela potência comandada (veja
/// `EstimadorDeslocamento`). Segue a convenção do `IEncoder`: a contagem aumenta quando a roda
/// avança. Quando os encoders forem instalados, o `EncoderEsp32` ocupa o lugar deste, e a
/// `Odometria` não muda (`ENCODERS_INSTALADOS` em `ConfiguracaoDeslocamento.h`).
class EncoderEstimado : public IEncoder {
 public:
  /// @param estimador Fonte das contagens; precisa continuar existindo enquanto o encoder for usado.
  /// @param roda Roda que este encoder representa.
  EncoderEstimado(EstimadorDeslocamento& estimador, Roda roda) : estimador_(estimador), roda_(roda) {}

  /// @copydoc IEncoder::lerContagem
  int32_t lerContagem() const override { return estimador_.contagem(roda_); }

  /// @copydoc IEncoder::zerar
  void zerar() override { estimador_.zerar(roda_); }

 private:
  EstimadorDeslocamento& estimador_;
  Roda roda_;
};

}  // namespace micromouse
