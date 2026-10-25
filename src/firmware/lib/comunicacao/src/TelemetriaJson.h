#pragma once

/// @file TelemetriaJson.h
/// @brief Serialização dos pacotes de telemetria periódicos em JSON para a transmissão serial/Wi-Fi e banco de dados.
///
/// Mapeamento dos campos JSON com a tabela `telemetria` do Supabase:
/// | Campo JSON          | Coluna na tabela `telemetria` | Descrição |
/// |---------------------|-------------------------------|-----------|
/// | `numero_sequencia`  | `numero_sequencia` (INT)      | Sequencial monotônico incremental por corrida |
/// | `posicao_linha`     | `posicao_linha` (INT)         | Linha atual na malha do labirinto (0 a N-1) |
/// | `posicao_coluna`    | `posicao_coluna` (INT)        | Coluna atual na malha do labirinto (0 a M-1) |
/// | `orientacao`        | `orientacao` (VARCHAR(2))     | Letra da direção: "N", "S", "L", "O" |
/// | `velocidade`        | `velocidade` (FLOAT)          | Velocidade linear instantânea estimada (m/s) |
/// | `tensao_bateria`    | `tensao_bateria` (FLOAT)      | Tensão instantânea da bateria em Volts (V) |
/// | `evento`            | `evento` (VARCHAR(30))        | Evento pontual ("objetivo_alcancado", "colisao" ou nulo) |
///
/// Campos adicionais de diagnóstico local (encoders e bancada, não persistidos na tabela padrão):
/// - `esq_pulsos`, `dir_pulsos`: contagem acumulada dos encoders ópticos
/// - `esq_mm`, `dir_mm`, `medio_mm`: deslocamento linear percorrido em milímetros
/// - `pose_valida`: flag de integridade da pose na malha discreta
/// - `tof_frente_mm`, `tof_esq_mm`, `tof_dir_mm`: última leitura dos 3 VL53L0X, em mm (#344)
/// - `tof_valida`: se a leitura dos 3 sensores é válida (`ISensorDistancia::lerDistanciasLaterais`)
/// - `tof_idade_ms`: idade da amostra mais antiga da leitura (`ISensorDistancia::idadeUltimaLeituraMs`)
/// - `deslocamento`: fonte do deslocamento, `"encoder"` ou `"estimado"` (sem encoder, #380)
/// - `sem_correcao_mm`: distância andada desde a última correção pelo ToF no modo estimado (#380)
/// - `boot_ok`, `boot_falhas`, `boot_ms`: resultado do autoteste do boot (#50), repetido em todo pacote
///   porque a linha do relatório do boot se perde se o computador ainda não estiver lendo a USB
/// - `i2c_falhas`: dispositivos do I²C em falha agora, separados por vírgula (`""` sem falha), e
///   `i2c_reinicios`: reinícios de dispositivo pedidos desde o boot (#241)

#include <stddef.h>
#include <stdint.h>

#include "Direcao.h"
#include "Tipos.h"

namespace micromouse {

/// Tamanho de buffer que comporta o pior caso do JSON de telemetria (todos os números no maior
/// valor, com o evento `objetivo_alcancado`, todas as falhas do boot, os 4 dispositivos do I²C em
/// falha e a fase mais longa), com folga. O pior caso tem 689 caracteres; com um buffer menor,
/// `serializar` devolve `0` e a linha se perde. Ao acrescentar campos, confira o teste
/// `test_pior_caso_cabe_em_tamanho_max_json_telemetria`.
constexpr size_t TAMANHO_MAX_JSON_TELEMETRIA = 768;

/// Dados reportados na telemetria periódica do robô.
struct PacoteTelemetria {
  // Campos diretamente alinhados com a tabela `telemetria` do Supabase:
  uint32_t numeroSequencia = 0;   ///< Sequencial monotônico incremental por corrida (`numero_sequencia`)
  Pose pose = {{0, 0}, Direcao::Norte};  ///< Célula (`posicao_linha`, `posicao_coluna`) e orientação ("N", "S", "L", "O")
  float velocidade = 0.0f;        ///< Velocidade linear instantânea em m/s (`velocidade`)
  float tensaoBateria = 0.0f;     ///< Tensão instantânea da bateria em Volts (`tensao_bateria`)
  const char* evento = nullptr;   ///< Evento pontual ("objetivo_alcancado", "colisao" ou nullptr)

  // Campos de diagnóstico de encoders e calibração de bancada:
  int32_t pulsosEsq = 0;          ///< Pulsos contados no encoder esquerdo (`esq_pulsos`)
  int32_t pulsosDir = 0;          ///< Pulsos contados no encoder direito (`dir_pulsos`)
  int32_t esqMm = 0;              ///< Deslocamento da roda esquerda em mm (`esq_mm`)
  int32_t dirMm = 0;              ///< Deslocamento da roda direita em mm (`dir_mm`)
  int32_t medioMm = 0;            ///< Deslocamento médio do robô em mm (`medio_mm`)
  bool poseValida = true;         ///< Pose estimada é válida na malha (`pose_valida`)

  // Campos de diagnóstico dos sensores de distância (#344):
  uint16_t tofFrenteMm = 0;       ///< Sensor frontal, em mm (`tof_frente_mm`)
  uint16_t tofEsqMm = 0;          ///< Sensor esquerdo, em mm (`tof_esq_mm`)
  uint16_t tofDirMm = 0;          ///< Sensor direito, em mm (`tof_dir_mm`)
  bool tofValida = false;         ///< Leitura dos 3 sensores válida (`tof_valida`)
  uint32_t tofIdadeMs = 0;        ///< Idade da leitura, em ms (`tof_idade_ms`)

  // Campos de diagnóstico do deslocamento sem encoder (#380):
  const char* modoDeslocamento = "encoder";  ///< `"encoder"` ou `"estimado"` (`deslocamento`)
  uint32_t semCorrecaoMm = 0;     ///< Distância desde a última correção pelo ToF (`sem_correcao_mm`)

  // Resultado do autoteste do boot (#50):
  bool bootOk = true;                  ///< Autoteste aprovado e motores liberados (`boot_ok`)
  const char* falhasBoot = nullptr;    ///< O que falhou, separado por vírgula; nullptr = "" (`boot_falhas`)
  uint32_t bootMs = 0;                 ///< Duração do boot, em ms (`boot_ms`)

  /// Fase do robô na corrida (#242): `aguardando_calibracao`, `aguardando_largada`, `em_corrida` ou
  /// `encerrada` (`fase`). nullptr = "" (a fase não foi informada).
  const char* fase = nullptr;

  // Falhas do I²C durante a corrida (#241):
  const char* falhasI2c = nullptr;     ///< Dispositivos em falha, separados por vírgula; nullptr = "" (`i2c_falhas`)
  uint32_t reiniciosI2c = 0;           ///< Reinícios de dispositivo pedidos desde o boot (`i2c_reinicios`)
};

/// Serializador de dados de telemetria em formato JSON compatível com a tabela `telemetria` e o dashboard web.
class TelemetriaJson {
 public:
  /// Serializa o pacote em cadeia de caracteres JSON em `saida`, terminada com `\0`.
  /// @param pacote Dados da telemetria a serializar.
  /// @param saida Buffer de destino.
  /// @param capacidade Tamanho do buffer em bytes.
  /// @return Quantidade de bytes gravados (sem o `\0`), ou `0` se não couber ou buffer inválido.
  static size_t serializar(const PacoteTelemetria& pacote, char* saida, size_t capacidade);
};

}  // namespace micromouse
