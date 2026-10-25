#pragma once

/// @file ReconexaoWifi.h
/// @brief Conecta e reconecta o Wi-Fi com espera crescente, sem travar a tarefa de rede (#238).

#include <stdint.h>

#include "ConfiguracaoRede.h"
#include "IRedeWifi.h"
#include "PoliticaReenvio.h"

namespace micromouse {

/// Situação da conexão depois do último `ciclo()`.
enum class EstadoWifi : uint8_t {
  Aguardando,  ///< Desconectado, esperando a hora da próxima tentativa.
  Conectando,  ///< Tentativa em andamento (até `tempoLimiteConexaoMs`).
  Conectado    ///< Conectado: pode chamar o servidor.
};

/// Máquina de estados da conexão Wi-Fi, chamada a cada ciclo da tarefa de rede.
///
/// Desconectada, começa uma tentativa (`iniciarConexao()`) e volta na hora. Nos ciclos seguintes,
/// confere se conectou. Se passar `tempoLimiteConexaoMs` sem conectar, desiste dessa tentativa e
/// espera antes da próxima: 1 s, 2 s, 4 s... até 30 s (`PoliticaReenvio`). Conectar volta a espera
/// para 1 s. Se a conexão cair, a próxima tentativa começa no ciclo seguinte.
class ReconexaoWifi {
 public:
  /// @param rede Wi-Fi da placa (ou a rede falsa dos testes).
  /// @param tempoLimiteConexaoMs Tempo máximo de uma tentativa.
  /// @param esperas Espera mínima, máxima e fator entre tentativas.
  explicit ReconexaoWifi(IRedeWifi& rede, uint32_t tempoLimiteConexaoMs = TEMPO_LIMITE_CONEXAO_WIFI_MS,
                         const ConfiguracaoReenvio& esperas = configuracaoEsperaWifi());

  /// Avança a máquina de estados. Nunca espera.
  /// @param agoraMs Relógio em ms (`millis()`), com a volta do contador tratada.
  /// @return `true` se está conectado.
  bool ciclo(uint32_t agoraMs);

  EstadoWifi estado() const { return estado_; }
  /// Tentativas de conexão começadas.
  uint32_t tentativas() const { return tentativas_; }
  /// Vezes em que a conexão caiu depois de feita.
  uint32_t quedas() const { return quedas_; }
  /// Espera que a próxima falha vai impor, em ms.
  uint32_t proximaEsperaMs() const { return esperas_.proximaEsperaMs(); }

 private:
  void comecarTentativa(uint32_t agoraMs);

  IRedeWifi& rede_;
  uint32_t tempoLimiteConexaoMs_;
  PoliticaReenvio esperas_;
  EstadoWifi estado_ = EstadoWifi::Aguardando;
  uint32_t instanteMs_ = 0;  ///< Início da tentativa (Conectando) ou hora da próxima (Aguardando).
  bool primeiraVez_ = true;
  uint32_t tentativas_ = 0;
  uint32_t quedas_ = 0;
};

}  // namespace micromouse
