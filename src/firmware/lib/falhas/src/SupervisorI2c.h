#pragma once

/// @file SupervisorI2c.h
/// @brief Detecção de falha nos 4 dispositivos do I²C durante a corrida e decisão das novas
/// tentativas (#241).

#include <stddef.h>
#include <stdint.h>

#include "AutotesteBoot.h"
#include "ConfiguracaoFalhasI2c.h"

namespace micromouse {

/// Resultado de uma leitura de um dispositivo, do ponto de vista do I²C.
enum class LeituraI2c : uint8_t {
  Ok,          ///< O dispositivo respondeu e entregou dado novo.
  Falha,       ///< Não respondeu, estourou o tempo ou parou de entregar dado.
  SemNovidade  ///< Nada a concluir neste ciclo: o ToF ainda está medindo, ou o barramento estava
               ///< com a outra tarefa.
};

/// Situação de um dispositivo para o supervisor.
enum class SituacaoI2c : uint8_t {
  Ok,         ///< Respondendo.
  Tentando,   ///< Falhou há pouco; a leitura é repetida no próximo ciclo. Os motores seguem.
  EmFalha,    ///< Esgotou as tentativas: motores parados e o dispositivo é reiniciado.
  Inoperante  ///< Esgotou os reinícios: motores parados, sem novos reinícios.
};

/// O que a tarefa dona do dispositivo deve fazer depois de registrar a leitura.
enum class AcaoI2c : uint8_t {
  Nenhuma,
  Reiniciar  ///< ToF: pelo XSHUT (`TofVl53l0x::reiniciarSensor`); IMU: `ImuMpu6050::iniciar`.
};

/// Resposta de `SupervisorI2c::registrarLeitura`: a ação pedida e a situação antes e depois da
/// leitura, para o log anunciar só as mudanças.
struct DecisaoI2c {
  AcaoI2c acao = AcaoI2c::Nenhuma;
  SituacaoI2c antes = SituacaoI2c::Ok;
  SituacaoI2c depois = SituacaoI2c::Ok;
  bool mudou() const { return antes != depois; }
};

/// Nome da situação no log: `ok`, `tentando`, `em_falha` ou `inoperante`.
const char* nomeSituacao(SituacaoI2c situacao);

/// Leitura da IMU para o supervisor: `falhou` vem da contagem de falhas do `AcompanhadorGiro` (o
/// giroscópio) e da leitura do acelerômetro (#192), que é o mesmo MPU6050, e `barramentoOcupado` da
/// contagem de esperas esgotadas do `BarramentoI2c`. Se a leitura falhou só porque a outra tarefa
/// estava com o barramento (por exemplo, reiniciando um ToF), não é falha da IMU.
constexpr LeituraI2c leituraDaImu(bool falhou, bool barramentoOcupado) {
  return !falhou ? LeituraI2c::Ok : barramentoOcupado ? LeituraI2c::SemNovidade : LeituraI2c::Falha;
}

/// Maior texto de `SupervisorI2c::descreverFalhas`, com o `\0`: os 4 dispositivos separados por
/// vírgula.
constexpr size_t TAMANHO_MAX_FALHAS_I2C = 48;

/// Acompanha os 4 dispositivos do I²C (3 VL53L0X e o MPU6050) durante a corrida.
///
/// Cada tarefa registra o resultado de cada leitura do dispositivo que ela usa. Para cada um:
///
/// 1. Uma leitura com falha deixa o dispositivo `Tentando`: a leitura é repetida no próximo ciclo,
///    e os motores seguem.
/// 2. Com `tentativasLeitura` falhas seguidas, ele fica `EmFalha`: os motores devem parar
///    (`motoresDevemParar()`), e a tarefa recebe `AcaoI2c::Reiniciar`. Depois de cada reinício, o
///    dispositivo tem de novo `tentativasLeitura` leituras para voltar; os reinícios respeitam
///    `esperaEntreReiniciosMs`.
/// 3. Depois de `reiniciosMaximos` reinícios sem voltar, ele fica `Inoperante` e não é mais
///    reiniciado. Os motores continuam parados.
/// 4. Em qualquer situação, `leiturasParaRecuperar` leituras boas seguidas o devolvem a `Ok`.
///
/// Não usa Arduino nem aloca memória. Não é seguro entre tarefas: no `main.cpp`, toda chamada fica
/// numa seção crítica curta, e a ação (o reinício) é feita fora dela, pela tarefa dona do
/// dispositivo.
///
/// @code
/// SupervisorI2c supervisor;
/// DecisaoI2c d = supervisor.registrarLeitura(Dispositivo::TofFrontal, LeituraI2c::Falha, millis());
/// if (d.acao == AcaoI2c::Reiniciar) {
///   supervisor.registrarReinicio(Dispositivo::TofFrontal, tof.reiniciarSensor(Lado::Frente));
/// }
/// if (supervisor.motoresDevemParar()) { /* trava os motores */ }
/// @endcode
class SupervisorI2c {
 public:
  explicit SupervisorI2c(const ConfiguracaoFalhasI2c& config = ConfiguracaoFalhasI2c{});

  /// Registra o resultado de uma leitura do `dispositivo`.
  /// @param agoraMs Relógio em ms (`millis()`), com a volta do contador tratada.
  /// @return A ação que a tarefa dona do dispositivo deve fazer e a situação antes e depois.
  DecisaoI2c registrarLeitura(Dispositivo dispositivo, LeituraI2c leitura, uint32_t agoraMs);

  /// Informa o resultado de um reinício pedido por `registrarLeitura`. Só entra nos contadores: quem
  /// decide se o dispositivo voltou são as leituras seguintes.
  void registrarReinicio(Dispositivo dispositivo, bool sucesso);

  /// Situação atual do `dispositivo`.
  SituacaoI2c situacao(Dispositivo dispositivo) const;

  /// `true` se algum dispositivo está `EmFalha` ou `Inoperante`.
  bool motoresDevemParar() const;

  /// Leituras com falha do `dispositivo` desde o início.
  uint32_t falhas(Dispositivo dispositivo) const;

  /// Falhas seguidas do `dispositivo` desde a última leitura boa ou o último reinício.
  uint8_t falhasSeguidas(Dispositivo dispositivo) const;

  /// Reinícios pedidos para o `dispositivo` desde o início.
  uint32_t reinicios(Dispositivo dispositivo) const;

  /// Reinícios do `dispositivo` que o driver informou como malsucedidos.
  uint32_t reiniciosSemSucesso(Dispositivo dispositivo) const;

  /// Reinícios pedidos para todos os dispositivos, para a telemetria.
  uint32_t reiniciosTotais() const;

  /// Escreve em `saida` os nomes dos dispositivos `EmFalha` ou `Inoperante`, separados por vírgula
  /// (por exemplo, `tof_frontal,mpu6050`), ou texto vazio se nenhum estiver.
  /// @return Bytes escritos (sem o `\0`), ou `0` se não couber (com `saida` vazia, se couber o `\0`).
  size_t descreverFalhas(char* saida, size_t capacidade) const;

 private:
  struct Acompanhamento {
    SituacaoI2c situacao = SituacaoI2c::Ok;
    uint8_t falhasSeguidas = 0;
    uint8_t okSeguidas = 0;
    uint8_t reiniciosSeguidos = 0;
    bool reiniciou = false;          ///< `true` depois do primeiro reinício (vale `ultimoReinicioMs`).
    uint32_t ultimoReinicioMs = 0;
    uint32_t falhas = 0;
    uint32_t reinicios = 0;
    uint32_t reiniciosSemSucesso = 0;
  };

  /// Pede um reinício, se ainda houver e se já passou a espera; senão, `Nenhuma` (e, sem reinícios,
  /// o dispositivo fica `Inoperante`).
  AcaoI2c pedirReinicio(Acompanhamento& a, uint32_t agoraMs);

  const Acompanhamento& acompanhamento(Dispositivo dispositivo) const {
    return acompanhamentos_[static_cast<uint8_t>(dispositivo)];
  }

  ConfiguracaoFalhasI2c config_;
  Acompanhamento acompanhamentos_[QUANTIDADE_DISPOSITIVOS];
};

}  // namespace micromouse
