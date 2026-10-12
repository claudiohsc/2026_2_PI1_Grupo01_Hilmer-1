#pragma once

#include "ClassificadorParede.h"
#include "Direcao.h"
#include "Mapa.h"
#include "Tipos.h"

namespace micromouse {

struct ResultadoAtualizacao {
  ResultadoRegistro frente = ResultadoRegistro::Invalida;
  ResultadoRegistro esquerda = ResultadoRegistro::Invalida;
  ResultadoRegistro direita = ResultadoRegistro::Invalida;
  bool temConflito = false;
};

class AtualizadorMapa {
public:
  /// Atualiza o mapa a partir das paredes já classificadas e da pose atual do
  /// robô.
  /// @param mapa Mapa a ser atualizado.
  /// @param pose Pose atual do robô.
  /// @param classificacao Resultado da classificação das paredes.
  /// @return Resultado da atualização.
  static ResultadoAtualizacao
  atualizar(Mapa &mapa, const Pose &pose,
            const ClassificadorParede::ResultadoClassificacao &classificacao);

  /// Sobrecarga de conveniência: recebe leituras do LiDAR, classifica com o
  /// classificador e atualiza o mapa.
  /// @param mapa Mapa a ser atualizado.
  /// @param pose Pose atual do robô.
  /// @param classificador Classificador de paredes.
  /// @param amostras Leituras do LiDAR.
  /// @param quantidadeAmostras Quantidade de amostras.
  /// @return Resultado da atualização.
  static ResultadoAtualizacao
  atualizar(Mapa &mapa, const Pose &pose,
            const ClassificadorParede &classificador,
            const DistanciasLaterais *amostras, size_t quantidadeAmostras);
};

} // namespace micromouse
