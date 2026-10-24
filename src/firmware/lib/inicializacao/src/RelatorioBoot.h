#pragma once

/// @file RelatorioBoot.h
/// @brief Linha JSON do relatório do boot (#50), escrita no log serial ao fim do `setup()`.

#include <stddef.h>
#include <stdint.h>

#include "AutotesteBoot.h"

namespace micromouse {

/// Tamanho de buffer que comporta o pior caso do relatório (todos os dispositivos com o nome de
/// estado mais longo e os `MAXIMO_ENDERECOS_INESPERADOS` endereços inesperados), com folga.
constexpr size_t TAMANHO_MAX_JSON_RELATORIO_BOOT = 512;

/// Relatório do boot em uma linha JSON, por exemplo:
///
/// ```json
/// {"tipo":"boot","ok":false,"motores_liberados":false,"falhas":"tof_frontal",
///  "boot_ms":412,"prazo_ms":2000,"no_prazo":true,
///  "dispositivos":{"tof_esquerdo":"ok","tof_frontal":"ausente","tof_direito":"ok","mpu6050":"ok"},
///  "tof_0x29":false,"mpu6050_who_am_i":104,"inesperados":[],"inesperados_total":0}
/// ```
///
/// `mpu6050_who_am_i` é `null` quando o registrador não foi lido.
class RelatorioBoot {
 public:
  /// Serializa o relatório em `saida`, terminado com `\0`.
  /// @param duracaoMs Tempo do boot medido pelo `setup()`.
  /// @return Bytes escritos (sem o `\0`), ou `0` se não couber ou o buffer for inválido.
  static size_t serializar(const ResultadoBoot& resultado, const AutotesteBoot& autoteste, uint32_t duracaoMs,
                           char* saida, size_t capacidade);
};

}  // namespace micromouse
