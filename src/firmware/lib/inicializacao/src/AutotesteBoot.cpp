#include "AutotesteBoot.h"

#include <string.h>

#include "Pinos.h"

namespace micromouse {

namespace {

constexpr uint8_t ENDERECOS[QUANTIDADE_DISPOSITIVOS] = {
    pinos::ENDERECO_TOF_ESQ, pinos::ENDERECO_TOF_FRONTAL, pinos::ENDERECO_TOF_DIR,
    pinos::ENDERECO_MPU6050};

bool esperado(uint8_t endereco) {
  for (uint8_t esperadoNoBarramento : ENDERECOS) {
    if (endereco == esperadoNoBarramento) {
      return true;
    }
  }
  return endereco == pinos::ENDERECO_FABRICA_TOF;
}

/// Acrescenta `nome` em `saida`, com vírgula antes se já houver texto. `false` se não couber.
bool acrescentar(char* saida, size_t capacidade, size_t& usados, const char* nome) {
  const size_t separador = usados > 0 ? 1 : 0;
  const size_t tamanho = strlen(nome);
  if (usados + separador + tamanho + 1 > capacidade) {
    return false;
  }
  if (separador > 0) {
    saida[usados++] = ',';
  }
  memcpy(saida + usados, nome, tamanho);
  usados += tamanho;
  saida[usados] = '\0';
  return true;
}

}  // namespace

const char* nomeDispositivo(Dispositivo dispositivo) {
  switch (dispositivo) {
    case Dispositivo::TofEsquerdo:
      return "tof_esquerdo";
    case Dispositivo::TofFrontal:
      return "tof_frontal";
    case Dispositivo::TofDireito:
      return "tof_direito";
    case Dispositivo::Mpu6050:
      return "mpu6050";
  }
  return "?";
}

const char* nomeEstado(EstadoDispositivo estado) {
  switch (estado) {
    case EstadoDispositivo::Ok:
      return "ok";
    case EstadoDispositivo::Ausente:
      return "ausente";
    case EstadoDispositivo::NaoIniciou:
      return "nao_iniciou";
    case EstadoDispositivo::NaoIdentificado:
      return "nao_identificado";
  }
  return "?";
}

bool ResultadoBoot::aprovado() const {
  for (EstadoDispositivo estadoDispositivo : dispositivos) {
    if (estadoDispositivo != EstadoDispositivo::Ok) {
      return false;
    }
  }
  return !tofNoEnderecoDeFabrica;
}

AutotesteBoot::AutotesteBoot(const ConfiguracaoBoot& config) : config_(config) {}

ResultadoBoot AutotesteBoot::verificar(IBarramentoI2c& barramento, const InicializacaoTof& iniciados) const {
  ResultadoBoot resultado;
  bool presentes[QUANTIDADE_DISPOSITIVOS] = {};

  for (uint8_t endereco = PRIMEIRO_ENDERECO_VARRIDO; endereco <= ULTIMO_ENDERECO_VARRIDO; endereco++) {
    if (!barramento.responde(endereco)) {
      continue;
    }
    for (uint8_t i = 0; i < QUANTIDADE_DISPOSITIVOS; i++) {
      if (endereco == ENDERECOS[i]) {
        presentes[i] = true;
      }
    }
    if (endereco == pinos::ENDERECO_FABRICA_TOF) {
      resultado.tofNoEnderecoDeFabrica = true;
    }
    if (!esperado(endereco)) {
      if (resultado.quantidadeInesperados < MAXIMO_ENDERECOS_INESPERADOS) {
        resultado.inesperados[resultado.quantidadeInesperados] = endereco;
      }
      if (resultado.quantidadeInesperados < UINT8_MAX) {
        resultado.quantidadeInesperados++;
      }
    }
  }

  // ToF: presente no endereço novo e configurado pelo driver.
  const bool tofIniciados[3] = {iniciados.esquerdo, iniciados.frontal, iniciados.direito};
  for (uint8_t i = 0; i < 3; i++) {
    if (!presentes[i]) {
      resultado.dispositivos[i] = EstadoDispositivo::Ausente;
    } else {
      resultado.dispositivos[i] = tofIniciados[i] ? EstadoDispositivo::Ok : EstadoDispositivo::NaoIniciou;
    }
  }

  // MPU6050: presente e com o WHO_AM_I esperado, para outro chip no 0x68 não passar por ele.
  const uint8_t mpu = static_cast<uint8_t>(Dispositivo::Mpu6050);
  if (!presentes[mpu]) {
    resultado.dispositivos[mpu] = EstadoDispositivo::Ausente;
  } else {
    resultado.whoAmILido =
        barramento.lerRegistrador(pinos::ENDERECO_MPU6050, REGISTRADOR_WHO_AM_I_MPU6050, resultado.whoAmIMpu6050);
    const bool identificado = resultado.whoAmILido && resultado.whoAmIMpu6050 == config_.whoAmIMpu6050;
    resultado.dispositivos[mpu] = identificado ? EstadoDispositivo::Ok : EstadoDispositivo::NaoIdentificado;
  }
  return resultado;
}

size_t AutotesteBoot::descreverFalhas(const ResultadoBoot& resultado, char* saida, size_t capacidade) {
  if (saida == nullptr || capacidade == 0) {
    return 0;
  }
  saida[0] = '\0';
  size_t usados = 0;
  for (uint8_t i = 0; i < QUANTIDADE_DISPOSITIVOS; i++) {
    if (resultado.dispositivos[i] != EstadoDispositivo::Ok &&
        !acrescentar(saida, capacidade, usados, nomeDispositivo(static_cast<Dispositivo>(i)))) {
      saida[0] = '\0';
      return 0;
    }
  }
  if (resultado.tofNoEnderecoDeFabrica &&
      !acrescentar(saida, capacidade, usados, FALHA_TOF_NO_ENDERECO_DE_FABRICA)) {
    saida[0] = '\0';
    return 0;
  }
  return usados;
}

}  // namespace micromouse
