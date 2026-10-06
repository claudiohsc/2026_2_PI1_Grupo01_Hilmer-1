#pragma once

#include <cstddef>
#include <cstdint>
#include "Configuracao.h"
#include "Tipos.h"

namespace micromouse {

/// @brief Estrutura que armazena a classificação das 3 paredes ao redor do robô.

/// @brief Classe responsável por classificar distâncias de sensores em estados de parede.
class ClassificadorParede {
  public:
  struct ResultadoClassificacao {
    EstadoParede frente   = EstadoParede::Desconhecido;
    EstadoParede esquerda = EstadoParede::Desconhecido;
    EstadoParede direita  = EstadoParede::Desconhecido;
  };
  private:
  uint16_t limiarDistanciaMm;

 public:
  /// @brief Construtor do classificador de paredes.
  /// @param limiarMm Limiar de distância em milímetros (padrão: TAMANHO_CELULA_MM).
  explicit ClassificadorParede(uint16_t limiarMm = TAMANHO_CELULA_MM);

  /// @brief Altera o limiar de detecção de parede em tempo de execução.
  /// @param novoLimiarMm Novo valor de limiar em milímetros (mm).
  void definirLimiar(uint16_t novoLimiarMm);

  /// @brief Obtém o limiar de detecção atualmente configurado.
  /// @return Limiar atual em milímetros (mm).
  uint16_t obterLimiar() const;

  /// @brief Classifica um conjunto de amostras de distância em estados de parede (Frente, Esquerda, Direita).
  /// @param amostras Ponteiro para o array com as leituras dos sensores.
  /// @param quantidade Número de amostras contidas no array.
  /// @return Estrutura ResultadoClassificacao com os estados (Parede, Livre ou Desconhecido).
  ResultadoClassificacao classificar(const DistanciasLaterais* amostras, size_t quantidade) const;
};

}  // namespace micromouse