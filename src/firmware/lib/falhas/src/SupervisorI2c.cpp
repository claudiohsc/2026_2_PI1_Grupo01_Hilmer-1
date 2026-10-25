#include "SupervisorI2c.h"

#include <string.h>

namespace micromouse {

namespace {

constexpr Dispositivo DISPOSITIVOS[QUANTIDADE_DISPOSITIVOS] = {Dispositivo::TofEsquerdo, Dispositivo::TofFrontal,
                                                              Dispositivo::TofDireito, Dispositivo::Mpu6050};

bool parado(SituacaoI2c situacao) {
  return situacao == SituacaoI2c::EmFalha || situacao == SituacaoI2c::Inoperante;
}

}  // namespace

const char* nomeSituacao(SituacaoI2c situacao) {
  switch (situacao) {
    case SituacaoI2c::Ok:
      return "ok";
    case SituacaoI2c::Tentando:
      return "tentando";
    case SituacaoI2c::EmFalha:
      return "em_falha";
    case SituacaoI2c::Inoperante:
      return "inoperante";
  }
  return "?";
}

// Zero tentativas ou zero leituras para recuperar valem como 1: a leitura é contada antes da
// comparação com o limite.
SupervisorI2c::SupervisorI2c(const ConfiguracaoFalhasI2c& config) : config_(config) {}

DecisaoI2c SupervisorI2c::registrarLeitura(Dispositivo dispositivo, LeituraI2c leitura, uint32_t agoraMs) {
  Acompanhamento& a = acompanhamentos_[static_cast<uint8_t>(dispositivo)];
  DecisaoI2c decisao;
  decisao.antes = a.situacao;

  if (leitura == LeituraI2c::Ok) {
    a.falhasSeguidas = 0;
    if (a.okSeguidas < UINT8_MAX) {
      ++a.okSeguidas;
    }
    if (a.situacao == SituacaoI2c::Tentando) {
      a.situacao = SituacaoI2c::Ok;  // falha isolada: a nova tentativa funcionou
    } else if (parado(a.situacao) && a.okSeguidas >= config_.leiturasParaRecuperar) {
      a.situacao = SituacaoI2c::Ok;
      a.reiniciosSeguidos = 0;
    }
  } else if (leitura == LeituraI2c::Falha) {
    ++a.falhas;
    a.okSeguidas = 0;
    if (a.falhasSeguidas < UINT8_MAX) {
      ++a.falhasSeguidas;
    }
    if (a.situacao != SituacaoI2c::Inoperante) {
      if (a.falhasSeguidas < config_.tentativasLeitura) {
        if (a.situacao == SituacaoI2c::Ok) {
          a.situacao = SituacaoI2c::Tentando;
        }
      } else {
        a.situacao = SituacaoI2c::EmFalha;
        decisao.acao = pedirReinicio(a, agoraMs);
      }
    }
  }

  decisao.depois = a.situacao;
  return decisao;
}

AcaoI2c SupervisorI2c::pedirReinicio(Acompanhamento& a, uint32_t agoraMs) {
  if (a.reiniciosSeguidos >= config_.reiniciosMaximos) {
    a.situacao = SituacaoI2c::Inoperante;
    return AcaoI2c::Nenhuma;
  }
  // Subtração sem sinal: continua certa quando o `millis()` dá a volta.
  if (a.reiniciou && agoraMs - a.ultimoReinicioMs < config_.esperaEntreReiniciosMs) {
    return AcaoI2c::Nenhuma;
  }
  ++a.reiniciosSeguidos;
  ++a.reinicios;
  a.reiniciou = true;
  a.ultimoReinicioMs = agoraMs;
  a.falhasSeguidas = 0;  // depois do reinício, o dispositivo tem de novo todas as tentativas
  return AcaoI2c::Reiniciar;
}

void SupervisorI2c::registrarReinicio(Dispositivo dispositivo, bool sucesso) {
  if (!sucesso) {
    ++acompanhamentos_[static_cast<uint8_t>(dispositivo)].reiniciosSemSucesso;
  }
}

SituacaoI2c SupervisorI2c::situacao(Dispositivo dispositivo) const { return acompanhamento(dispositivo).situacao; }

bool SupervisorI2c::motoresDevemParar() const {
  for (const Acompanhamento& a : acompanhamentos_) {
    if (parado(a.situacao)) {
      return true;
    }
  }
  return false;
}

uint32_t SupervisorI2c::falhas(Dispositivo dispositivo) const { return acompanhamento(dispositivo).falhas; }

uint8_t SupervisorI2c::falhasSeguidas(Dispositivo dispositivo) const {
  return acompanhamento(dispositivo).falhasSeguidas;
}

uint32_t SupervisorI2c::reinicios(Dispositivo dispositivo) const { return acompanhamento(dispositivo).reinicios; }

uint32_t SupervisorI2c::reiniciosSemSucesso(Dispositivo dispositivo) const {
  return acompanhamento(dispositivo).reiniciosSemSucesso;
}

uint32_t SupervisorI2c::reiniciosTotais() const {
  uint32_t total = 0;
  for (const Acompanhamento& a : acompanhamentos_) {
    total += a.reinicios;
  }
  return total;
}

size_t SupervisorI2c::descreverFalhas(char* saida, size_t capacidade) const {
  if (saida == nullptr || capacidade == 0) {
    return 0;
  }
  saida[0] = '\0';
  size_t escritos = 0;
  for (Dispositivo dispositivo : DISPOSITIVOS) {
    if (!parado(situacao(dispositivo))) {
      continue;
    }
    const char* nome = nomeDispositivo(dispositivo);
    const size_t tamanho = strlen(nome) + (escritos > 0 ? 1 : 0);
    if (escritos + tamanho + 1 > capacidade) {
      saida[0] = '\0';
      return 0;
    }
    if (escritos > 0) {
      saida[escritos++] = ',';
    }
    memcpy(saida + escritos, nome, strlen(nome) + 1);
    escritos += strlen(nome);
  }
  return escritos;
}

}  // namespace micromouse
