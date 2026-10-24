#pragma once

/// @file AutotesteBoot.h
/// @brief Autoteste do boot da Rev 2.0 (#50): confere os 4 dispositivos do I²C antes de liberar os motores.

#include <stddef.h>
#include <stdint.h>

#include "ConfiguracaoBoot.h"
#include "IBarramentoI2c.h"

namespace micromouse {

/// Dispositivos que o boot exige no barramento I²C.
enum class Dispositivo : uint8_t {
  TofEsquerdo,  ///< VL53L0X esquerdo, em `pinos::ENDERECO_TOF_ESQ` (0x30).
  TofFrontal,   ///< VL53L0X frontal, em `pinos::ENDERECO_TOF_FRONTAL` (0x31).
  TofDireito,   ///< VL53L0X direito, em `pinos::ENDERECO_TOF_DIR` (0x32).
  Mpu6050       ///< IMU, em `pinos::ENDERECO_MPU6050` (0x68).
};

constexpr uint8_t QUANTIDADE_DISPOSITIVOS = 4;

/// Situação de um dispositivo depois do autoteste.
enum class EstadoDispositivo : uint8_t {
  Ok,               ///< Respondeu no endereço esperado (e, no MPU6050, com o `WHO_AM_I` certo).
  Ausente,          ///< Ninguém respondeu no endereço esperado.
  NaoIniciou,       ///< ToF respondeu no endereço novo, mas o driver não conseguiu configurá-lo.
  NaoIdentificado   ///< Algo respondeu no 0x68, mas o `WHO_AM_I` não é o do MPU6050 ou não foi lido.
};

/// Nome do dispositivo no relatório e na telemetria: `tof_esquerdo`, `tof_frontal`, `tof_direito`
/// ou `mpu6050`.
const char* nomeDispositivo(Dispositivo dispositivo);

/// Nome do estado no relatório: `ok`, `ausente`, `nao_iniciou` ou `nao_identificado`.
const char* nomeEstado(EstadoDispositivo estado);

/// Quais ToF o `TofVl53l0x::iniciar()` conseguiu configurar (`sensorIniciado`).
struct InicializacaoTof {
  bool esquerdo;
  bool frontal;
  bool direito;
};

/// Nome da falha "ToF no endereço de fábrica" em `descreverFalhas`.
constexpr const char* FALHA_TOF_NO_ENDERECO_DE_FABRICA = "tof_0x29";

/// Maior texto de `AutotesteBoot::descreverFalhas`, com o `\0`: os 4 dispositivos e o `tof_0x29`.
constexpr size_t TAMANHO_MAX_FALHAS_BOOT = 64;

/// Resultado do autoteste.
struct ResultadoBoot {
  EstadoDispositivo dispositivos[QUANTIDADE_DISPOSITIVOS] = {
      EstadoDispositivo::Ausente, EstadoDispositivo::Ausente, EstadoDispositivo::Ausente,
      EstadoDispositivo::Ausente};

  /// `true` se algo respondeu no 0x29: um VL53L0X ficou no endereço de fábrica (não trocou de
  /// endereço, ou o XSHUT dele não está ligado ao pino certo). Reprova o boot.
  bool tofNoEnderecoDeFabrica = false;

  bool whoAmILido = false;     ///< `true` se o `WHO_AM_I` do 0x68 foi lido.
  uint8_t whoAmIMpu6050 = 0;   ///< Valor lido (só vale com `whoAmILido`).

  /// Endereços que responderam sem ser esperados. Só geram aviso: não reprovam o boot.
  uint8_t inesperados[MAXIMO_ENDERECOS_INESPERADOS] = {};
  /// Quantos endereços inesperados responderam (pode passar de `MAXIMO_ENDERECOS_INESPERADOS`).
  uint8_t quantidadeInesperados = 0;

  /// Estado de um dispositivo.
  EstadoDispositivo estado(Dispositivo dispositivo) const {
    return dispositivos[static_cast<uint8_t>(dispositivo)];
  }

  /// `true` se os 4 dispositivos estão `Ok` e nenhum ToF ficou no 0x29: os motores podem ser liberados.
  bool aprovado() const;
};

/// Autoteste do boot: varre o barramento I²C e diz se os motores podem ser liberados.
///
/// Roda depois de `TofVl53l0x::iniciar()`, quando os ToF já deveriam estar em 0x30, 0x31 e 0x32.
/// Os motores só são liberados com os 4 dispositivos presentes; qualquer falha os mantém freados
/// até o robô ser reiniciado (repetir as tentativas durante a corrida é da #241).
///
/// @code
/// AutotesteBoot autoteste;
/// ResultadoBoot resultado = autoteste.verificar(barramento, {tof.sensorIniciado(Lado::Esquerda), ...});
/// if (resultado.aprovado()) { motores.liberar(); }
/// @endcode
class AutotesteBoot {
 public:
  explicit AutotesteBoot(const ConfiguracaoBoot& config = ConfiguracaoBoot{});

  /// Varre os endereços de `PRIMEIRO_ENDERECO_VARRIDO` a `ULTIMO_ENDERECO_VARRIDO`, confere os 4
  /// dispositivos e lê o `WHO_AM_I` do MPU6050.
  /// @param iniciados Quais ToF o driver configurou: um ToF que responde mas não foi configurado
  /// fica `NaoIniciou`.
  ResultadoBoot verificar(IBarramentoI2c& barramento, const InicializacaoTof& iniciados) const;

  /// `true` se o boot levou até `tempoMaximoMs` (RNF-09).
  bool dentroDoPrazo(uint32_t duracaoMs) const { return duracaoMs <= config_.tempoMaximoMs; }

  /// Prazo do boot em uso, em ms.
  uint32_t tempoMaximoMs() const { return config_.tempoMaximoMs; }

  /// Escreve em `saida` os nomes do que falhou, separados por vírgula (por exemplo,
  /// `tof_frontal,mpu6050`), ou texto vazio se o boot foi aprovado.
  /// @return Bytes escritos (sem o `\0`), ou `0` se não couber (com `saida` vazia).
  static size_t descreverFalhas(const ResultadoBoot& resultado, char* saida, size_t capacidade);

 private:
  ConfiguracaoBoot config_;
};

}  // namespace micromouse
