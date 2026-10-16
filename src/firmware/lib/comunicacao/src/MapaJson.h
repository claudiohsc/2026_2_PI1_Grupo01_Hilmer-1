#pragma once

/// @file MapaJson.h
/// @brief Serialização do `Mapa` em JSON para a telemetria.
///
/// Não depende do Arduino (usa a biblioteca ArduinoJson, que também compila no computador),
/// então a serialização é testada no ambiente `native`.

#include <stddef.h>

#include "Mapa.h"

namespace micromouse {

/// Maior tamanho, em bytes e sem o terminador `\0`, do JSON de um mapa 12×4 com as 48 células
/// totalmente conhecidas (o pior caso, porque `false` ocupa mais caracteres que `null`).
/// Um buffer de `TAMANHO_MAX_JSON_MAPA + 1` bytes comporta qualquer mapa da competição. Esse
/// buffer é maior que a pilha das tarefas (4096 bytes), então deve ser `static` ou global.
constexpr size_t TAMANHO_MAX_JSON_MAPA = 5100;

/// Converte o `Mapa` no JSON enviado na telemetria.
///
/// **Formato.** Um objeto com as dimensões e uma lista de células, da linha 0 à última e, em cada
/// linha, da coluna 0 à última. Os nomes dos campos das células são os das colunas da tabela
/// `mapa_celulas` do banco, então a lista pode ser inserida nela, acrescentando só o `corrida_id`.
///
/// @code{.json}
/// {
///   "dimensao": "4x4",
///   "linhas": 4,
///   "colunas": 4,
///   "celulas": [
///     {"linha": 0, "coluna": 0, "parede_norte": null, "parede_sul": true,
///      "parede_leste": null, "parede_oeste": true},
///     ...
///   ]
/// }
/// @endcode
///
/// | Campo | Valor |
/// |---|---|
/// | `dimensao` | `"4x4"`, `"8x4"` ou `"12x4"` (colunas × linhas, como `labirintos.tipo`) |
/// | `linhas`, `colunas` | Tamanho do labirinto |
/// | `linha`, `coluna` | Posição da célula; a partida é (0, 0), com Norte aumentando a linha |
/// | `parede_*` | `true` se há parede, `false` se está livre e `null` se ainda é desconhecida |
///
/// O texto sai sem espaços nem quebras de linha (o exemplo acima está formatado só para leitura).
/// A ArduinoJson usa a memória dinâmica durante a conversão e a libera ao final; a saída é escrita
/// num buffer do chamador, de tamanho fixo.
class ArmazenamentoMapa;

class MapaJson {
 public:
  /// Escreve o JSON do mapa em `saida`, terminado em `\0`.
  /// @param mapa Mapa a serializar.
  /// @param[out] saida Buffer do chamador.
  /// @param capacidade Tamanho de `saida` em bytes, incluindo o `\0`.
  /// @return Número de caracteres escritos, sem o `\0`; `0` se `saida` é nulo ou não comporta o
  ///         JSON inteiro (nesse caso `saida` fica com uma string vazia, quando há espaço).
  static size_t serializar(const Mapa& mapa, char* saida, size_t capacidade);

  /// Escreve o JSON do mapa final em `saida` a partir de um `ArmazenamentoMapa` congelado.
  /// @param armazenamento Armazenamento cujo mapa final será serializado.
  /// @param[out] saida Buffer do chamador.
  /// @param capacidade Tamanho de `saida` em bytes, incluindo o `\0`.
  /// @return Número de caracteres escritos, sem o `\0`; `0` se o mapa não estiver congelado,
  ///         `saida` for nulo ou não comportar o JSON inteiro.
  static size_t serializarFinal(const ArmazenamentoMapa& armazenamento, char* saida, size_t capacidade);
};

}  // namespace micromouse
