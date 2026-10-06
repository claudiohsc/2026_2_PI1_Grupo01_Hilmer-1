#pragma once

/// @file Mapa.h
/// @brief Mapa do labirinto: matriz de células com o estado das quatro paredes de cada uma.
///
/// Convenções, as mesmas de `Tipos.h` e `Direcao.h`: a célula de partida é (0, 0), no canto
/// inferior esquerdo; Norte aumenta a `linha` e Leste aumenta a `coluna`.
///
/// Não depende do Arduino e não aloca memória dinâmica: a matriz tem tamanho fixo
/// (`MAX_LINHAS_LABIRINTO` × `MAX_COLUNAS_LABIRINTO`).

#include <cstdint>

#include "Configuracao.h"
#include "Tipos.h"

namespace micromouse {

/// Mapa de um labirinto de até `MAX_LINHAS_LABIRINTO` × `MAX_COLUNAS_LABIRINTO` células.
///
/// Cada célula guarda o estado (`EstadoParede`) das suas quatro paredes. Paredes ainda não
/// observadas ficam `Desconhecido`.
class Mapa {
 public:
  /// Estado das quatro paredes de uma célula.
  struct Celula {
    EstadoParede norte = EstadoParede::Desconhecido;
    EstadoParede sul = EstadoParede::Desconhecido;
    EstadoParede leste = EstadoParede::Desconhecido;
    EstadoParede oeste = EstadoParede::Desconhecido;
  };

  /// Cria um mapa vazio com o tamanho informado.
  /// @param dimensao Tamanho do labirinto (padrão: 4×4).
  explicit Mapa(DimensaoMapa dimensao = DimensaoMapa::Labirinto4x4);

  /// Tamanho informado na criação.
  DimensaoMapa obterDimensao() const { return dimensao; }

  /// Número de linhas (eixo Norte-Sul).
  uint8_t obterLinhas() const { return linhas; }

  /// Número de colunas (eixo Leste-Oeste).
  uint8_t obterColunas() const { return colunas; }

  /// `true` se a célula está dentro do mapa.
  bool posValida(PosicaoCelula posicao) const;

  /// Estado das quatro paredes de uma célula; todas `Desconhecido` se a célula está fora do mapa.
  Celula obterCelula(PosicaoCelula posicao) const;

  /// Estado de uma parede; `Desconhecido` se ela não foi observada ou a célula está fora do mapa.
  EstadoParede obterParede(PosicaoCelula posicao, Direcao direcao) const;

  /// Registra o estado de uma parede observada.
  /// @param posicao Célula da parede.
  /// @param direcao Lado da célula (norte, leste, sul ou oeste).
  /// @param estado Estado observado da parede.
  void registrarParede(PosicaoCelula posicao, Direcao direcao, EstadoParede estado);

 private:
  DimensaoMapa dimensao;
  uint8_t linhas;
  uint8_t colunas;
  Celula celulas[MAX_LINHAS_LABIRINTO][MAX_COLUNAS_LABIRINTO];
};

}  // namespace micromouse
