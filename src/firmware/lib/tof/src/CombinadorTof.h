#pragma once

/// @file CombinadorTof.h
/// @brief Regra de validade das leituras dos 3 VL53L0X, sem depender do Arduino.
///
/// O driver (`TofVl53l0x`, em `drivers_esp32`) só fala com os sensores pelo I²C e entrega cada
/// medida bruta a esta classe, que decide se ela vale e monta a leitura das três direções.
/// Assim a regra é testada no computador (`test_tof`).

#include <stdint.h>

#include "ConfiguracaoTof.h"
#include "Tipos.h"

namespace micromouse {

/// Situação da última amostra de um sensor.
enum class EstadoAmostraTof : uint8_t {
  SemAmostra,      ///< O sensor ainda não entregou nenhuma medida.
  Valida,          ///< Distância dentro da faixa válida.
  AbaixoDoMinimo,  ///< Distância abaixo de `distanciaMinimaMm`: obstáculo perto demais.
  ForaDeAlcance,   ///< Distância acima de `distanciaMaximaMm` ou sem alvo (8190/8191 mm).
  Falha            ///< O sensor não respondeu no I²C ou estourou o tempo de espera.
};

/// Última amostra de um sensor, com a distância bruta, para quem precisa tratar cada caso
/// (por exemplo, "fora de alcance" como lado livre, na percepção, issue #379).
struct AmostraTof {
  uint16_t distanciaMm = 0;                           ///< Distância bruta, em mm.
  EstadoAmostraTof estado = EstadoAmostraTof::SemAmostra;
  uint32_t instanteMs = 0;                            ///< Quando a amostra foi registrada.
};

/// Guarda a última amostra de cada sensor e monta a leitura das três direções.
///
/// A leitura só vale (`distancias()` devolve `true`) quando:
/// - os 3 sensores têm amostra no estado `Valida`;
/// - a amostra mais antiga tem no máximo `idadeMaximaLeituraMs`.
///
/// Exemplo:
/// @code
/// CombinadorTof combinador;
/// combinador.registrar(Lado::Frente, 250, false, agora);
/// combinador.registrar(Lado::Esquerda, 40, false, agora);
/// combinador.registrar(Lado::Direita, 45, false, agora);
/// DistanciasLaterais leitura;
/// if (combinador.distancias(agora, leitura)) { /* usa a leitura */ }
/// @endcode
class CombinadorTof {
 public:
  /// Idade devolvida por `idadeMs()` quando falta amostra de algum sensor.
  static constexpr uint32_t IDADE_DESCONHECIDA = UINT32_MAX;

  explicit CombinadorTof(const ConfiguracaoTof& config = ConfiguracaoTof{});

  /// Classifica uma medida bruta conforme a faixa válida da configuração.
  /// @param distanciaMm Distância devolvida pelo sensor, em mm.
  /// @param falhou `true` se o sensor não respondeu ou estourou o tempo.
  EstadoAmostraTof classificar(uint16_t distanciaMm, bool falhou) const;

  /// Registra uma nova medida de um sensor, substituindo a anterior dele.
  /// @param lado Sensor que mediu.
  /// @param distanciaMm Distância devolvida pelo sensor, em mm.
  /// @param falhou `true` se o sensor não respondeu ou estourou o tempo.
  /// @param agoraMs Instante da medida, em ms.
  void registrar(Lado lado, uint16_t distanciaMm, bool falhou, uint32_t agoraMs);

  /// Última amostra registrada de um sensor.
  const AmostraTof& amostra(Lado lado) const;

  /// Monta a leitura das três direções.
  /// @param agoraMs Instante atual, em ms, para conferir a idade das amostras.
  /// @param[out] saida Recebe as distâncias, em mm, quando a leitura vale.
  /// @return `true` se a leitura vale; `false` caso contrário (veja a regra na classe).
  bool distancias(uint32_t agoraMs, DistanciasLaterais& saida) const;

  /// Idade, em ms, da amostra mais antiga dos 3 sensores, ou `IDADE_DESCONHECIDA` se algum
  /// ainda não entregou amostra. É a idade da leitura que `distancias()` monta.
  uint32_t idadeMs(uint32_t agoraMs) const;

  /// Esquece todas as amostras (por exemplo, depois de reiniciar os sensores).
  void reiniciar();

 private:
  static uint8_t indice(Lado lado) { return static_cast<uint8_t>(lado); }

  ConfiguracaoTof config_;
  AmostraTof amostras_[3];  ///< Indexado por `Lado`: frente, esquerda, direita.
};

}  // namespace micromouse
