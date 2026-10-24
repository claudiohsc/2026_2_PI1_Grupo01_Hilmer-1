#pragma once

/// @file ClassificadorParede.h
/// @brief Classificação de distâncias dos sensores ToF em estados de parede (HU-02, #379).

#include <cstddef>
#include <cstdint>

#include "Configuracao.h"
#include "ConfiguracaoPercepcao.h"
#include "Tipos.h"

namespace micromouse {

/// @brief Classe responsável por filtrar e classificar leituras de distância em estados (parede/livre).
class ClassificadorParede {
 public:
  /// @brief Estrutura que armazena a classificação das 3 paredes ao redor do robô.
  struct ResultadoClassificacao {
    EstadoParede frente   = EstadoParede::Desconhecido;
    EstadoParede esquerda = EstadoParede::Desconhecido;
    EstadoParede direita  = EstadoParede::Desconhecido;
  };

  /// @brief Construtor padrão com as configurações da camada de percepção ToF.
  ClassificadorParede();

  /// @brief Construtor com limiar único para todos os lados (compatibilidade).
  /// @param limiarMm Limiar de distância em milímetros para frente, esquerda e direita.
  explicit ClassificadorParede(uint16_t limiarMm);

  /// @brief Construtor com estrutura completa de configuração.
  /// @param config Configuração da percepção (limiares por lado, limites e tipo de filtro).
  explicit ClassificadorParede(const ConfiguracaoPercepcao& config);

  /// @brief Construtor com limiares específicos por lado.
  /// @param limiarFrenteMm Limiar da parede frontal em mm.
  /// @param limiarEsquerdaMm Limiar da parede esquerda em mm.
  /// @param limiarDireitaMm Limiar da parede direita em mm.
  ClassificadorParede(uint16_t limiarFrenteMm, uint16_t limiarEsquerdaMm, uint16_t limiarDireitaMm);

  /// @brief Altera o limiar de detecção de parede para todas as direções.
  /// @param novoLimiarMm Novo valor de limiar em milímetros (mm).
  void definirLimiar(uint16_t novoLimiarMm);

  /// @brief Altera o limiar de detecção para um lado específico.
  /// @param lado Lado do robô (Frente, Esquerda ou Direita).
  /// @param novoLimiarMm Novo valor de limiar em milímetros (mm).
  void definirLimiar(Lado lado, uint16_t novoLimiarMm);

  /// @brief Altera os limiares de detecção para cada lado individualmente.
  /// @param frenteMm Limiar frontal em mm.
  /// @param esquerdaMm Limiar esquerdo em mm.
  /// @param direitaMm Limiar direito em mm.
  void definirLimiares(uint16_t frenteMm, uint16_t esquerdaMm, uint16_t direitaMm);

  /// @brief Obtém o limiar frontal de detecção atualmente configurado.
  /// @return Limiar atual em milímetros (mm).
  uint16_t obterLimiar() const;

  /// @brief Obtém o limiar de detecção de um lado específico.
  /// @param lado Lado do robô (Frente, Esquerda ou Direita).
  /// @return Limiar em milímetros (mm).
  uint16_t obterLimiar(Lado lado) const;

  /// @brief Obtém a configuração atual da percepção.
  const ConfiguracaoPercepcao& obterConfiguracao() const;

  /// @brief Atualiza a configuração da percepção.
  void definirConfiguracao(const ConfiguracaoPercepcao& config);

  /// @brief Trata distâncias brutas aplicando saturação defensiva aos limites operacionais.
  /// Leituras abaixo de `distanciaMinimaMm` são limitadas ao mínimo (parede muito próxima).
  /// Leituras acima de `distanciaMaximaMm` são limitadas ao máximo (fora de alcance / livre).
  /// @param distanciaBrutaMm Leitura bruta do sensor em mm.
  /// @return Leitura tratada e limitada dentro dos limites [distanciaMinima, distanciaMaxima].
  uint16_t tratarDistancia(uint16_t distanciaBrutaMm) const;

  /// @brief Verifica se a distância informada está abaixo do mínimo operacional confiável.
  /// @param distanciaMm Distância medida em mm.
  /// @return `true` se abaixo do mínimo confiável.
  bool ehAbaixoDoMinimo(uint16_t distanciaMm) const;

  /// @brief Verifica se a distância informada está fora de alcance (sem obstáculo no raio de alcance).
  /// @param distanciaMm Distância medida em mm.
  /// @return `true` se fora de alcance (ou código de erro 8190+).
  bool ehForaDeAlcance(uint16_t distanciaMm) const;

  /// @brief Classifica uma única distância para um determinado lado.
  /// Aplica `tratarDistancia()` e compara com o limiar correspondente.
  /// @param lado Direção da leitura (Frente, Esquerda, Direita).
  /// @param distanciaMm Distância medida em mm.
  /// @return EstadoParede::Parede se <= limiar, EstadoParede::Livre se > limiar.
  EstadoParede classificarDistancia(Lado lado, uint16_t distanciaMm) const;

  /// @brief Classifica uma única leitura com as 3 direções.
  /// @param leitura Estrutura com as distâncias frente, esquerda e direita.
  /// @return Resultado da classificação.
  ResultadoClassificacao classificar(const DistanciasLaterais& leitura) const;

  /// @brief Classifica um conjunto de amostras de distância aplicando filtragem digital.
  /// @param amostras Ponteiro para o array com as leituras dos sensores.
  /// @param quantidade Número de amostras contidas no array.
  /// @return Estrutura ResultadoClassificacao com os estados (Parede, Livre ou Desconhecido).
  ResultadoClassificacao classificar(const DistanciasLaterais* amostras, size_t quantidade) const;

  /// @brief Filtra um conjunto de amostras numéricas usando o filtro configurado (mediana ou média).
  /// @param valores Array com os valores numéricos brutos em mm.
  /// @param quantidade Número de amostras no array.
  /// @return Valor filtrado em mm após tratamento de limites.
  uint16_t filtrarAmostras(const uint16_t* valores, size_t quantidade) const;

 private:
  ConfiguracaoPercepcao config_;

  static uint16_t calcularMediana(uint16_t* valores, size_t qtd);
  static uint16_t calcularMedia(const uint16_t* valores, size_t qtd);
};

}  // namespace micromouse