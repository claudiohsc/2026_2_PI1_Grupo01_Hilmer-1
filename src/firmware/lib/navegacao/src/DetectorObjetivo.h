#pragma once

/// @file DetectorObjetivo.h
/// @brief Detecção da chegada à célula objetivo do labirinto.
///
/// Identifica quando o robô alcança a célula de destino a partir da pose estimada pela odometria ou da posição na malha.

#include <stdint.h>

#include "Tipos.h"

namespace micromouse {

class ArmazenamentoMapa;
class IMotores;

/// Responsável por determinar a célula objetivo e verificar se ela foi alcançada.
///
/// O labirinto inicia na célula (0, 0) no canto inferior esquerdo. O objetivo
/// situa-se sempre no canto diametralmente oposto: (linhas - 1, colunas - 1).
class DetectorObjetivo {
 public:
  /// Calcula a célula objetivo diametralmente oposta a partir das dimensões do labirinto.
  /// @param linhas Quantidade total de linhas do labirinto.
  /// @param colunas Quantidade total de colunas do labirinto.
  /// @return Coordenada da célula objetivo (linhas - 1, colunas - 1).
  static PosicaoCelula calcularObjetivo(uint8_t linhas, uint8_t colunas);

  /// Calcula a célula objetivo a partir do enum `DimensaoMapa`.
  /// @param dimensao Tamanho do labirinto (4x4, 8x4 ou 12x4).
  /// @return Coordenada da célula objetivo.
  static PosicaoCelula calcularObjetivo(DimensaoMapa dimensao);

  /// Verifica se uma posição coincide com a célula objetivo informada.
  /// @param atual Posição a ser verificada.
  /// @param objetivo Posição alvo.
  /// @return `true` se a posição atual for idêntica à do objetivo.
  static bool alcancouObjetivo(const PosicaoCelula& atual, const PosicaoCelula& objetivo);

  /// Construtor que recebe as coordenadas explícitas da célula objetivo.
  /// @param objetivo Célula alvo a ser alcançada.
  explicit DetectorObjetivo(PosicaoCelula objetivo);

  /// Construtor que deriva a célula objetivo a partir da dimensão do labirinto.
  /// @param dimensao Dimensão do labirinto (padrão: 4x4).
  explicit DetectorObjetivo(DimensaoMapa dimensao = DimensaoMapa::Labirinto4x4);

  /// Construtor que calcula a célula objetivo a partir da quantidade de linhas e colunas.
  /// @param linhas Quantidade de linhas do labirinto.
  /// @param colunas Quantidade de colunas do labirinto.
  DetectorObjetivo(uint8_t linhas, uint8_t colunas);

  /// Retorna a célula objetivo atualmente configurada.
  PosicaoCelula celulaObjetivo() const { return objetivo_; }

  /// Atualiza a célula objetivo configurada.
  /// @param novoObjetivo Nova célula de destino.
  void definirObjetivo(PosicaoCelula novoObjetivo) { objetivo_ = novoObjetivo; }

  /// Atualiza a célula objetivo a partir da dimensão do labirinto.
  /// @param dimensao Dimensão do labirinto.
  void definirObjetivo(DimensaoMapa dimensao) { objetivo_ = calcularObjetivo(dimensao); }

  /// Atualiza a célula objetivo a partir da quantidade de linhas e colunas.
  /// @param linhas Quantidade de linhas do labirinto.
  /// @param colunas Quantidade de colunas do labirinto.
  void definirObjetivo(uint8_t linhas, uint8_t colunas) {
    objetivo_ = calcularObjetivo(linhas, colunas);
  }

  /// Verifica se a célula informada é o objetivo.
  /// @param atual Posição da célula atual do robô.
  /// @return `true` se estiver na célula objetivo.
  bool alcancouObjetivo(const PosicaoCelula& atual) const;

  /// Sobrecarga de conveniência: verifica a pose informada (ex.: retornada por `Odometria::pose()`).
  /// @param pose Pose atual estimada do robô.
  /// @return `true` se a célula da pose for o objetivo.
  bool alcancouObjetivo(const Pose& pose) const {
    return alcancouObjetivo(pose.celula);
  }

  /// Verifica a chegada ao objetivo e, caso alcançado, para os motores imediatamente
  /// e congela o mapa em ArmazenamentoMapa (HU-07 / #189).
  /// @param pose Pose atual do robô.
  /// @param motores Interface dos motores (camada HAL) a ser freada imediatamente.
  /// @param armazenamento Armazenamento do mapa (opcional) a ser congelado.
  /// @return `true` se o objetivo foi alcançado (nesta chamada ou anteriormente).
  bool processarChegada(const Pose& pose, IMotores& motores, ArmazenamentoMapa* armazenamento = nullptr);

  /// Retorna se o objetivo já foi alcançado e processado.
  bool objetivoAlcancado() const { return alcancado_; }

  /// Reseta o estado de conclusão (ex.: para nova corrida ou reinício de tentativa).
  void resetar() { alcancado_ = false; }

 private:
  PosicaoCelula objetivo_{0, 0};
  bool alcancado_ = false;
};

}  // namespace micromouse