#pragma once

/// @file BarramentoI2cSimulado.h
/// @brief Implementação falsa de `IBarramentoI2c` para testar o boot sem hardware (#50).

#include <stdint.h>

#include "IBarramentoI2c.h"

namespace micromouse {

/// Barramento I²C falso: o teste liga e desliga dispositivos e define o valor de registradores.
///
/// Exemplo:
/// @code
/// BarramentoI2cSimulado barramento;
/// barramento.conectar(0x68);
/// barramento.definirRegistrador(0x68, 0x75, 0x68);   // WHO_AM_I do MPU6050
/// barramento.responde(0x68);                         // true
/// @endcode
class BarramentoI2cSimulado : public IBarramentoI2c {
 public:
  /// Quantos registradores com valor definido o simulador guarda (somando todos os endereços).
  static constexpr uint8_t CAPACIDADE_REGISTRADORES = 8;

  /// Liga um dispositivo no `endereco` (7 bits; acima de 0x7F é ignorado).
  void conectar(uint8_t endereco) {
    if (endereco <= 0x7F) {
      presentes_[endereco] = true;
    }
  }

  /// Desliga o dispositivo do `endereco`.
  void desconectar(uint8_t endereco) {
    if (endereco <= 0x7F) {
      presentes_[endereco] = false;
    }
  }

  /// Define o que `lerRegistrador(endereco, registrador, ...)` devolve. Sem espaço, é ignorado.
  void definirRegistrador(uint8_t endereco, uint8_t registrador, uint8_t valor) {
    for (uint8_t i = 0; i < quantidadeRegistradores_; i++) {
      if (registradores_[i].endereco == endereco && registradores_[i].registrador == registrador) {
        registradores_[i].valor = valor;
        return;
      }
    }
    if (quantidadeRegistradores_ < CAPACIDADE_REGISTRADORES) {
      registradores_[quantidadeRegistradores_++] = {endereco, registrador, valor};
    }
  }

  /// Com `true`, o dispositivo continua respondendo ao endereço, mas toda leitura falha.
  void falharLeituras(bool falhar) { falharLeituras_ = falhar; }

  /// @copydoc IBarramentoI2c::responde
  bool responde(uint8_t endereco) override {
    ++consultas_;
    return endereco <= 0x7F && presentes_[endereco];
  }

  /// @copydoc IBarramentoI2c::lerRegistrador
  /// Registrador sem valor definido devolve 0, como um dispositivo recém-ligado.
  bool lerRegistrador(uint8_t endereco, uint8_t registrador, uint8_t& valor) override {
    if (falharLeituras_ || endereco > 0x7F || !presentes_[endereco]) {
      return false;
    }
    valor = 0;
    for (uint8_t i = 0; i < quantidadeRegistradores_; i++) {
      if (registradores_[i].endereco == endereco && registradores_[i].registrador == registrador) {
        valor = registradores_[i].valor;
      }
    }
    return true;
  }

  /// Quantas vezes `responde` foi chamado (para conferir a faixa varrida).
  uint32_t consultas() const { return consultas_; }

 private:
  struct Registrador {
    uint8_t endereco;
    uint8_t registrador;
    uint8_t valor;
  };

  bool presentes_[0x80] = {};
  Registrador registradores_[CAPACIDADE_REGISTRADORES] = {};
  uint8_t quantidadeRegistradores_ = 0;
  bool falharLeituras_ = false;
  uint32_t consultas_ = 0;
};

}  // namespace micromouse
