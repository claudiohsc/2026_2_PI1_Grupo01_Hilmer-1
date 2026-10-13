#include "AgregadorSetores.h"

#include <algorithm>

namespace micromouse {

namespace {

constexpr uint16_t VOLTA_CENTI_GRAUS = 36000;
constexpr uint16_t MEIA_VOLTA_CENTI_GRAUS = 18000;
constexpr uint16_t ANGULO_DIREITA_CENTI_GRAUS = 9000;
constexpr uint16_t ANGULO_ESQUERDA_CENTI_GRAUS = 27000;
constexpr uint16_t MAX_MEIA_LARGURA_CENTI_GRAUS = 4499;

/// Diferença absoluta entre dois ângulos, sem considerar que 359° e 1° são vizinhos.
uint16_t diferencaLinear(uint16_t a, uint16_t b) { return a > b ? a - b : b - a; }

/// Menor diferença entre dois ângulos na circunferência (359° e 1° estão a 2°).
uint16_t diferencaCircular(uint16_t a, uint16_t b) {
  const uint16_t d = diferencaLinear(a, b);
  return d > MEIA_VOLTA_CENTI_GRAUS ? VOLTA_CENTI_GRAUS - d : d;
}

/// Mediana de `n` valores (n >= 1). Reordena `v`. Com `n` par, é a média dos dois centrais.
uint16_t mediana(uint16_t* v, uint16_t n) {
  std::sort(v, v + n);
  if (n % 2 == 1) {
    return v[n / 2];
  }
  return static_cast<uint16_t>((static_cast<uint32_t>(v[n / 2 - 1]) + v[n / 2]) / 2);
}

}  // namespace

AgregadorSetores::AgregadorSetores(const ConfiguracaoLidar& config) : config_(config) {
  if (config_.meiaLarguraSetorCentiGraus > MAX_MEIA_LARGURA_CENTI_GRAUS) {
    config_.meiaLarguraSetorCentiGraus = MAX_MEIA_LARGURA_CENTI_GRAUS;
  }
  if (config_.minimoPontosPorSetor == 0) {
    config_.minimoPontosPorSetor = 1;
  }
  limparAcumuladores();
}

void AgregadorSetores::reiniciar() {
  limparAcumuladores();
  temUltimoAngulo_ = false;
  descartarProximaVolta_ = true;
  voltaValida_ = false;
}

bool AgregadorSetores::adicionarPonto(const PontoLidar& ponto) {
  const uint16_t angulo = ponto.anguloCentiGraus % VOLTA_CENTI_GRAUS;
  bool fechou = false;
  if (temUltimoAngulo_ && diferencaLinear(angulo, ultimoAngulo_) > MEIA_VOLTA_CENTI_GRAUS) {
    fechou = fecharVolta();
  }
  ultimoAngulo_ = angulo;
  temUltimoAngulo_ = true;
  acumular(angulo, ponto.distanciaMm);
  return fechou;
}

int8_t AgregadorSetores::setorDoAngulo(uint16_t anguloCentiGraus) const {
  int32_t relativo = static_cast<int32_t>(anguloCentiGraus % VOLTA_CENTI_GRAUS) -
                     static_cast<int32_t>(config_.deslocamentoFrenteCentiGraus % VOLTA_CENTI_GRAUS);
  if (relativo < 0) {
    relativo += VOLTA_CENTI_GRAUS;
  }
  if (!config_.anguloCrescenteHorario) {
    relativo = (VOLTA_CENTI_GRAUS - relativo) % VOLTA_CENTI_GRAUS;
  }
  // A partir daqui, `r` é o ângulo em relação à frente do robô, crescendo no sentido horário.
  const uint16_t r = static_cast<uint16_t>(relativo);
  const uint16_t meia = config_.meiaLarguraSetorCentiGraus;
  if (diferencaCircular(r, 0) <= meia) {
    return static_cast<int8_t>(Lado::Frente);
  }
  if (diferencaCircular(r, ANGULO_DIREITA_CENTI_GRAUS) <= meia) {
    return static_cast<int8_t>(Lado::Direita);
  }
  if (diferencaCircular(r, ANGULO_ESQUERDA_CENTI_GRAUS) <= meia) {
    return static_cast<int8_t>(Lado::Esquerda);
  }
  return -1;
}

void AgregadorSetores::acumular(uint16_t anguloCentiGraus, uint16_t distanciaMm) {
  if (distanciaMm < config_.distanciaMinimaMm || distanciaMm > config_.distanciaMaximaMm) {
    return;
  }
  const int8_t setor = setorDoAngulo(anguloCentiGraus);
  if (setor < 0 || contagem_[setor] >= CAPACIDADE_SETOR) {
    return;
  }
  valores_[setor][contagem_[setor]++] = distanciaMm;
}

bool AgregadorSetores::fecharVolta() {
  if (descartarProximaVolta_) {
    descartarProximaVolta_ = false;
    limparAcumuladores();
    return false;
  }
  uint16_t resultado[NUM_SETORES] = {0, 0, 0};
  bool valida = true;
  for (uint8_t s = 0; s < NUM_SETORES; s++) {
    if (contagem_[s] < config_.minimoPontosPorSetor) {
      valida = false;
      break;
    }
    resultado[s] = mediana(valores_[s], contagem_[s]);
  }
  voltaValida_ = valida;
  if (valida) {
    distancias_.frenteMm = resultado[static_cast<uint8_t>(Lado::Frente)];
    distancias_.esquerdaMm = resultado[static_cast<uint8_t>(Lado::Esquerda)];
    distancias_.direitaMm = resultado[static_cast<uint8_t>(Lado::Direita)];
  }
  limparAcumuladores();
  return true;
}

void AgregadorSetores::limparAcumuladores() {
  for (uint8_t s = 0; s < NUM_SETORES; s++) {
    contagem_[s] = 0;
  }
}

}  // namespace micromouse
