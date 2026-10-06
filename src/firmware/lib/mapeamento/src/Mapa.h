#pragma once

#include <cstdint>
#include "Configuracao.h"
#include "Tipos.h"

namespace micromouse {

class Mapa {
 public:
  /// Estrutura de uma célula com suas 4 paredes
  struct Celula {
    EstadoParede norte = EstadoParede::Desconhecido;
    EstadoParede sul   = EstadoParede::Desconhecido;
    EstadoParede leste = EstadoParede::Desconhecido;
    EstadoParede oeste = EstadoParede::Desconhecido;
  };

 private:
  DimensaoMapa dimensao;
  uint8_t linhas;
  uint8_t colunas;

  // Matriz estática de tamanho máximo (4 x 12) para armazenar os estados do mapa
  Celula celulas[MAX_LINHAS_LABIRINTO][MAX_COLUNAS_LABIRINTO];

 public:
  explicit Mapa(DimensaoMapa dimensao = DimensaoMapa::Labirinto4x4);

  // Getters
  DimensaoMapa obterDimensao() const { return dimensao; }
  uint8_t obterLinhas() const { return linhas; }
  uint8_t obterColunas() const { return colunas; }

  // Retorna o estado das 4 paredes de uma célula
  Celula obterCelula(PosicaoCelula posicao) const;

  // Retorna o estado de somente uma parede de uma célula
  EstadoParede obterParede(PosicaoCelula posicao, Direcao direcao) const;

  // Métodos de modificação e validação
  void registrarParede(PosicaoCelula posicao, Direcao direcao, EstadoParede estado);
  bool posValida(PosicaoCelula posicao) const;
};

}  // namespace micromouse