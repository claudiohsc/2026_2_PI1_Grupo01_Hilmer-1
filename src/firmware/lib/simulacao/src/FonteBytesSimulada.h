#pragma once

/// @file FonteBytesSimulada.h
/// @brief Implementação falsa de `IFonteBytes` para testes sem hardware.

#include <stddef.h>
#include <stdint.h>

#include "IFonteBytes.h"

namespace micromouse {

/// Fonte de bytes falsa: entrega, em ordem, os bytes que o teste enfileirar.
/// Sem bytes na fila, `lerByte()` devolve `false`, como uma UART sem dados (sensor mudo).
/// A fila é circular e de tamanho fixo (`CAPACIDADE_BYTES`), sem alocação dinâmica.
class FonteBytesSimulada : public IFonteBytes {
 public:
  /// Quantidade máxima de bytes na fila.
  static constexpr uint16_t CAPACIDADE_BYTES = 4096;

  /// Acrescenta bytes ao fim da fila.
  /// @return `false` se não couber tudo (nada é enfileirado); `true` caso contrário.
  bool enfileirar(const uint8_t* dados, size_t quantidade) {
    if (quantidade > static_cast<size_t>(CAPACIDADE_BYTES - quantidade_)) {
      return false;
    }
    for (size_t i = 0; i < quantidade; i++) {
      fila_[(inicio_ + quantidade_) % CAPACIDADE_BYTES] = dados[i];
      quantidade_++;
    }
    return true;
  }

  /// Quantidade de bytes ainda na fila.
  uint16_t quantidadeDisponivel() const { return quantidade_; }

  /// @copydoc IFonteBytes::lerByte
  bool lerByte(uint8_t& saida) override {
    if (quantidade_ == 0) {
      return false;
    }
    saida = fila_[inicio_];
    inicio_ = (inicio_ + 1) % CAPACIDADE_BYTES;
    quantidade_--;
    return true;
  }

 private:
  uint8_t fila_[CAPACIDADE_BYTES] = {};
  uint16_t inicio_ = 0;
  uint16_t quantidade_ = 0;
};

}  // namespace micromouse
