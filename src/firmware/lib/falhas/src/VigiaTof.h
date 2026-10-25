#pragma once

/// @file VigiaTof.h
/// @brief Transforma as amostras dos 3 VL53L0X no resultado de cada leitura para o `SupervisorI2c`
/// (#241).

#include <stdint.h>

#include "AutotesteBoot.h"
#include "CombinadorTof.h"
#include "ConfiguracaoTof.h"
#include "SupervisorI2c.h"
#include "Tipos.h"

namespace micromouse {

/// Dispositivo do I²C de cada ToF.
constexpr Dispositivo dispositivoDoTof(Lado lado) {
  return lado == Lado::Frente ? Dispositivo::TofFrontal
         : lado == Lado::Esquerda ? Dispositivo::TofEsquerdo
                                  : Dispositivo::TofDireito;
}

/// Decide, a cada ciclo da navegação, se cada ToF respondeu, falhou ou não tem novidade.
///
/// O `TofVl53l0x` registra no `CombinadorTof` cada medida lida e, quando o sensor não responde no
/// I²C, uma amostra em `Falha` a cada leitura. Entre duas medidas (cerca de 33 ms), ele não
/// registra nada. Por isso, para cada sensor:
///
/// - **amostra nova** (instante diferente da última vista): `Falha` se ela está em `Falha`; senão
///   `Ok`, inclusive fora de alcance ou abaixo do mínimo, que são medidas de um sensor que respondeu;
/// - **sem amostra nova** há mais de `idadeMaximaMs`: `Falha` (o sensor parou de medir), uma vez a
///   cada `idadeMaximaMs`;
/// - **sem amostra nova** há menos que isso: `SemNovidade`.
///
/// Depois de um reinício (`sensorReiniciado`), as amostras de antes dele são ignoradas, e o sensor
/// tem `idadeMaximaMs` para entregar a primeira medida.
///
/// Não usa Arduino: testado no computador.
class VigiaTof {
 public:
  /// @param idadeMaximaMs Tempo sem medida nova que conta como falha. O padrão é a idade máxima da
  /// leitura do `CombinadorTof` (`IDADE_MAXIMA_LEITURA_TOF_MS`).
  explicit VigiaTof(uint32_t idadeMaximaMs = IDADE_MAXIMA_LEITURA_TOF_MS) : idadeMaximaMs_(idadeMaximaMs) {}

  /// Avalia a última amostra do sensor do `lado` (`TofVl53l0x::amostra`).
  /// @param agoraMs Relógio em ms (`millis()`), lido depois da leitura dos sensores.
  LeituraI2c avaliar(Lado lado, const AmostraTof& amostra, uint32_t agoraMs);

  /// Avisa que o sensor do `lado` acabou de ser reiniciado.
  void sensorReiniciado(Lado lado, uint32_t agoraMs);

 private:
  struct Acompanhamento {
    bool comecou = false;      ///< `true` depois da primeira avaliação.
    bool viuAmostra = false;   ///< `true` depois da primeira amostra nova (vale `ultimoInstanteMs`).
    bool reiniciado = false;   ///< `true` depois de um reinício (vale `reinicioMs`).
    uint32_t ultimoInstanteMs = 0;
    uint32_t reinicioMs = 0;
    uint32_t referenciaMs = 0;  ///< Última novidade, ou o início da contagem do tempo sem medida.
  };

  uint32_t idadeMaximaMs_;
  Acompanhamento acompanhamentos_[3];  ///< Indexado por `Lado`: frente, esquerda, direita.
};

}  // namespace micromouse
