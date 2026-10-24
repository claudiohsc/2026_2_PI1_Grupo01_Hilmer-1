#pragma once

/// @file TofSimulado.h
/// @brief Os 3 VL53L0X simulados num labirinto: amostras brutas pelo `CombinadorTof` real (#344).

#include <stdint.h>

#include "CombinadorTof.h"
#include "ISensorDistancia.h"
#include "LabirintoSimulado.h"

namespace micromouse {

/// Sem alvo no alcance, o VL53L0X devolve este valor, em mm.
constexpr uint16_t SEM_ALVO_TOF_SIMULADO_MM = 8190;

/// Recuo provisório de cada sensor em relação à borda do robô, em mm, o mesmo da percepção (#379):
/// a face do sensor fica 30 mm mais perto da parede do que o centro do robô.
constexpr uint16_t RECUO_SENSOR_TOF_SIMULADO_MM = 30;

/// Implementação de `ISensorDistancia` que faz o que o `TofVl53l0x` faz na placa, sem hardware:
/// cada sensor gera uma amostra bruta, o `CombinadorTof` decide a validade e monta a leitura.
///
/// A amostra bruta de cada sensor vem do `LabirintoSimulado`, na pose de `posicionar`:
/// - a distância medida do centro do robô menos `RECUO_SENSOR_TOF_SIMULADO_MM`, que é a distância
///   da face do sensor;
/// - `SEM_ALVO_TOF_SIMULADO_MM` (8190 mm) quando a parede fica além do alcance do sensor
///   (`ConfiguracaoTof::distanciaMaximaMm`), como o sensor real faz nos corredores longos.
///
/// Para cobrir os outros casos do sensor real, o teste pode forçar a próxima amostra bruta de um
/// lado (`forcarAmostra`, por exemplo um sensor encostado na parede) ou fazer um sensor parar de
/// responder (`simularFalha`). Cada leitura avança o relógio simulado em `PERIODO_LEITURA_MS`.
///
/// Exemplo:
/// @code
/// TofSimulado tof(labirinto);
/// tof.posicionar({{0, 0}, Direcao::Norte});
/// tof.forcarAmostra(Lado::Esquerda, 12);         // encostado: sai limitado a 30 mm, parede
/// ISensorDistancia& sensor = tof;                // a lógica recebe só a interface
/// @endcode
class TofSimulado : public ISensorDistancia {
 public:
  /// Intervalo do relógio simulado entre duas leituras, em ms (uma medição do VL53L0X).
  static constexpr uint32_t PERIODO_LEITURA_MS = 33;

  /// @param labirinto Labirinto de referência, já carregado. Não é copiado: precisa continuar
  ///                  existindo enquanto o sensor o usar.
  /// @param config Faixa válida e idade máxima, como no driver real.
  explicit TofSimulado(const LabirintoSimulado& labirinto, const ConfiguracaoTof& config = ConfiguracaoTof{})
      : labirinto_(labirinto), config_(config), combinador_(config) {}

  /// Define onde o robô está. Pode ser chamada a cada movimento do teste.
  void posicionar(const Pose& pose) {
    pose_ = pose;
    posicionado_ = true;
  }

  /// Faz a próxima leitura usar `distanciaMm` como amostra bruta do sensor do `lado`, no lugar
  /// da que viria do labirinto. Vale para uma leitura só.
  void forcarAmostra(Lado lado, uint16_t distanciaMm) {
    forcada_[indice(lado)] = true;
    distanciaForcada_[indice(lado)] = distanciaMm;
  }

  /// Liga ou desliga a falha do sensor do `lado` (não responde no I²C).
  void simularFalha(Lado lado, bool falha) { falha_[indice(lado)] = falha; }

  /// @copydoc ISensorDistancia::lerDistanciasLaterais
  bool lerDistanciasLaterais(DistanciasLaterais& saida) override {
    agoraMs_ += PERIODO_LEITURA_MS;
    DistanciasLaterais doCentro = {};
    if (!posicionado_ || !labirinto_.distancias(pose_, doCentro)) {
      return false;
    }
    const uint16_t doCentroPorLado[3] = {doCentro.frenteMm, doCentro.esquerdaMm, doCentro.direitaMm};
    for (uint8_t i = 0; i < 3; i++) {
      uint16_t bruta = amostraBruta(doCentroPorLado[i]);
      if (forcada_[i]) {
        bruta = distanciaForcada_[i];
        forcada_[i] = false;
      }
      combinador_.registrar(static_cast<Lado>(i), bruta, falha_[i], agoraMs_);
    }
    return combinador_.distancias(agoraMs_, saida);
  }

  /// @copydoc ISensorDistancia::idadeUltimaLeituraMs
  uint32_t idadeUltimaLeituraMs() const override { return combinador_.idadeMs(agoraMs_); }

  /// Última amostra bruta de um sensor, com o estado, como no `TofVl53l0x`.
  const AmostraTof& amostra(Lado lado) const { return combinador_.amostra(lado); }

 private:
  static uint8_t indice(Lado lado) { return static_cast<uint8_t>(lado); }

  /// O que o sensor mediria com a parede a `doCentroMm` do centro do robô.
  uint16_t amostraBruta(uint16_t doCentroMm) const {
    const uint16_t daFace =
        doCentroMm > RECUO_SENSOR_TOF_SIMULADO_MM ? doCentroMm - RECUO_SENSOR_TOF_SIMULADO_MM : 0;
    return daFace > config_.distanciaMaximaMm ? SEM_ALVO_TOF_SIMULADO_MM : daFace;
  }

  const LabirintoSimulado& labirinto_;
  ConfiguracaoTof config_;
  CombinadorTof combinador_;
  Pose pose_ = {};
  bool posicionado_ = false;
  uint32_t agoraMs_ = 0;                   ///< Relógio simulado, em ms.
  bool falha_[3] = {};                     ///< Indexado por `Lado`.
  bool forcada_[3] = {};                   ///< Se a próxima amostra do lado foi forçada.
  uint16_t distanciaForcada_[3] = {};      ///< Amostra bruta forçada de cada lado, em mm.
};

}  // namespace micromouse
