#include "AtualizadorMapa.h"

namespace micromouse {

ResultadoAtualizacao AtualizadorMapa::atualizar(
  Mapa &mapa,
  const Pose &pose,
  const ClassificadorParede::ResultadoClassificacao &classificacao) {
  ResultadoAtualizacao res;

  Direcao direcaoAtual = pose.direcao;
  PosicaoCelula posicaoAtual = pose.celula;

  // 1. Registra Frente
  if (classificacao.frente != EstadoParede::Desconhecido) {
    Direcao dirFrente = direcaoAbsoluta(direcaoAtual, Lado::Frente);
    res.frente = mapa.registrarParede(posicaoAtual, dirFrente, classificacao.frente);
  }

  // 2. Registra Esquerda
  if (classificacao.esquerda != EstadoParede::Desconhecido) {
    Direcao dirEsquerda = direcaoAbsoluta(direcaoAtual, Lado::Esquerda);
    res.esquerda = mapa.registrarParede(posicaoAtual, dirEsquerda, classificacao.esquerda);
  }

  // 3. Registra Direita
  if (classificacao.direita != EstadoParede::Desconhecido) {
    Direcao dirDireita = direcaoAbsoluta(direcaoAtual, Lado::Direita);
    res.direita = mapa.registrarParede(posicaoAtual, dirDireita, classificacao.direita);
  }

  // 4. Verifica conflitos
  res.temConflito = (res.frente == ResultadoRegistro::Conflito ||
                     res.esquerda == ResultadoRegistro::Conflito ||
                     res.direita == ResultadoRegistro::Conflito);

  return res;
}

ResultadoAtualizacao AtualizadorMapa::atualizar(
  Mapa &mapa, const Pose &pose,
  const ClassificadorParede &classificador,
  const DistanciasLaterais *amostras, size_t quantidadeAmostras) {
    return atualizar(mapa, pose, classificador.classificar(amostras, quantidadeAmostras));
}

} // namespace micromouse
