#pragma once

/// @file LidarUart.h
/// @brief Driver do LiDAR 360° lido por UART: implementa `ILidar` sem depender do Arduino.

#include <stdint.h>

#include "AgregadorSetores.h"
#include "ConfiguracaoLidar.h"
#include "IDecodificadorLidar.h"
#include "IFonteBytes.h"
#include "ILidar.h"

namespace micromouse {

/// Driver do LiDAR: lê bytes, decodifica, agrega por setor e entrega a última volta completa.
///
/// Nunca bloqueia: a cada chamada de `lerDistanciasLaterais()` ele só processa os bytes que
/// já chegaram (no máximo `maxBytesPorChamada`). O LiDAR gira a 5-12 Hz, bem mais devagar que
/// o laço de navegação (10 ms), então várias chamadas seguidas devolvem a mesma volta.
///
/// A leitura falha (`false`) quando:
/// - nenhuma volta completa chegou ainda (sensor desligado, sem fio, sem resposta);
/// - a última volta completa tem mais de `idadeMaximaLeituraMs`;
/// - a última volta não teve pontos válidos suficientes em algum setor.
///
/// Este driver só lê. O manual do LD06 diz que o sensor gira e transmite sozinho ao ligar, mas o
/// anúncio do ST-L50B2 diz que ele só inicia após receber comandos. Se, na bancada, nenhum byte
/// chegar depois de ligar o sensor, falta o envio do comando de partida, que exige escrever na
/// UART (o fio TX do ESP32 já está previsto no esquemático) e ainda não existe aqui.
///
/// Exemplo (a montagem real fica em `main.cpp`):
/// @code
/// FonteBytesSerial fonte(Serial1);
/// DecodificadorLd06 decodificador;
/// fonte.iniciar(DecodificadorLd06::BAUD_PADRAO, PINO_RX, PINO_TX);  // pinos de Pinos.h, após resolver o conflito com o Serial
/// LidarUart lidar(fonte, decodificador, agoraMs);
/// ILidar& sensor = lidar;
/// @endcode
class LidarUart : public ILidar {
 public:
  /// Função que devolve o tempo em ms desde a partida (no ESP32, um invólucro de `millis()`).
  using FuncaoRelogioMs = uint32_t (*)();

  /// @param fonte Origem dos bytes do sensor.
  /// @param decodificador Interpreta o protocolo do sensor.
  /// @param agoraMs Relógio em ms. Se for `nullptr`, a idade da leitura não é verificada.
  /// @param config Parâmetros de calibração.
  LidarUart(IFonteBytes& fonte, IDecodificadorLidar& decodificador, FuncaoRelogioMs agoraMs,
            const ConfiguracaoLidar& config = ConfiguracaoLidar{});

  /// @copydoc ILidar::lerDistanciasLaterais
  bool lerDistanciasLaterais(DistanciasLaterais& saida) override;

 private:
  void processarBytesDisponiveis();
  uint32_t agora() const { return agoraMs_ != nullptr ? agoraMs_() : 0; }

  IFonteBytes& fonte_;
  IDecodificadorLidar& decodificador_;
  FuncaoRelogioMs agoraMs_;
  ConfiguracaoLidar config_;
  AgregadorSetores agregador_;
  bool temVolta_ = false;
  bool ultimaVoltaValida_ = false;
  uint32_t instanteUltimaVoltaMs_ = 0;
  DistanciasLaterais ultimaLeitura_ = {0, 0, 0};
};

}  // namespace micromouse
