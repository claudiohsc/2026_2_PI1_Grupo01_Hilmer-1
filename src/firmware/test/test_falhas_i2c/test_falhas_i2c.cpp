/// @file test_falhas_i2c.cpp
/// @brief Testes da detecção de falha no I²C e das novas tentativas (#241): o `SupervisorI2c` (falha
/// em cada um dos 4 dispositivos, tentativas configuráveis, reinício, inoperante e recuperação), o
/// `VigiaTof` (amostras dos ToF → leitura), a `TravaFalhaI2c` (motores parados enquanto houver
/// falha, sem soltar a trava do boot) e a integração com ToF e IMU simulados.
/// Roda no computador: `pio test -e native -f test_falhas_i2c`.

#include <unity.h>

#include <cstring>

#include "AcompanhadorGiro.h"
#include "AutotesteBoot.h"
#include "CombinadorTof.h"
#include "ConfiguracaoFalhasI2c.h"
#include "ImuSimulado.h"
#include "MotoresComTrava.h"
#include "MotoresSimulados.h"
#include "SupervisorI2c.h"
#include "TravaFalhaI2c.h"
#include "VigiaTof.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint32_t CICLO_MS = 10;  ///< Ciclo da navegação e da tarefa da IMU.
constexpr Dispositivo TODOS[QUANTIDADE_DISPOSITIVOS] = {Dispositivo::TofEsquerdo, Dispositivo::TofFrontal,
                                                       Dispositivo::TofDireito, Dispositivo::Mpu6050};
constexpr Lado LADOS[3] = {Lado::Frente, Lado::Esquerda, Lado::Direita};

/// Registra `vezes` leituras iguais do `dispositivo`, uma a cada `CICLO_MS`, e devolve a última decisão.
DecisaoI2c registrarVarias(SupervisorI2c& supervisor, Dispositivo dispositivo, LeituraI2c leitura, int vezes,
                           uint32_t& agoraMs) {
  DecisaoI2c decisao;
  for (int i = 0; i < vezes; ++i) {
    agoraMs += CICLO_MS;
    decisao = supervisor.registrarLeitura(dispositivo, leitura, agoraMs);
  }
  return decisao;
}

AmostraTof amostraEm(EstadoAmostraTof estado, uint32_t instanteMs) {
  AmostraTof a;
  a.distanciaMm = 150;
  a.estado = estado;
  a.instanteMs = instanteMs;
  return a;
}

/// Os 3 VL53L0X simulados como o `TofVl53l0x` os registra no `CombinadorTof`: medindo, uma medida
/// a cada 30 ms; sem resposta, uma amostra em `Falha` a cada leitura; parado, nada.
enum class ModoTof : uint8_t { Medindo, SemResposta, Parado };

class TresTofSimulados {
 public:
  void definirModo(Lado lado, ModoTof modo) { modos_[indice(lado)] = modo; }
  /// Se `true`, o reinício pelo XSHUT faz o sensor voltar a medir; se `false`, ele continua sem resposta.
  void voltarNoReinicio(Lado lado, bool volta) { volta_[indice(lado)] = volta; }

  void ler(uint32_t agoraMs) {
    for (Lado lado : LADOS) {
      const uint8_t i = indice(lado);
      if (modos_[i] == ModoTof::Medindo) {
        if (!mediu_[i] || agoraMs - ultimaMedidaMs_[i] >= 30) {
          combinador_.registrar(lado, 150, false, agoraMs);
          ultimaMedidaMs_[i] = agoraMs;
          mediu_[i] = true;
        }
      } else if (modos_[i] == ModoTof::SemResposta) {
        combinador_.registrar(lado, 0, true, agoraMs);
      }
    }
  }

  /// O reinício pelo XSHUT: devolve `true` se o sensor voltou.
  bool reiniciar(Lado lado) {
    const uint8_t i = indice(lado);
    ++reinicios_[i];
    if (volta_[i]) {
      modos_[i] = ModoTof::Medindo;
      mediu_[i] = false;
      return true;
    }
    modos_[i] = ModoTof::SemResposta;
    return false;
  }

  const AmostraTof& amostra(Lado lado) const { return combinador_.amostra(lado); }
  uint32_t reinicios(Lado lado) const { return reinicios_[indice(lado)]; }

 private:
  static uint8_t indice(Lado lado) { return static_cast<uint8_t>(lado); }

  CombinadorTof combinador_;
  ModoTof modos_[3] = {ModoTof::Medindo, ModoTof::Medindo, ModoTof::Medindo};
  bool volta_[3] = {true, true, true};
  bool mediu_[3] = {false, false, false};
  uint32_t ultimaMedidaMs_[3] = {0, 0, 0};
  uint32_t reinicios_[3] = {0, 0, 0};
};

/// A tarefa de navegação simulada, na ordem do `supervisionarI2c()` do `main.cpp`: a cada 10 ms lê
/// os ToF, registra cada um no supervisor, aplica a trava, reinicia o que o supervisor pediu e manda
/// a potência de cruzeiro.
struct NavegacaoSimulada {
  TresTofSimulados tof;
  VigiaTof vigia;
  SupervisorI2c supervisor;
  MotoresSimulados driver;
  MotoresComTrava trava{driver};
  TravaFalhaI2c travaFalha;
  uint32_t agoraMs = 0;
  bool andandoEmAlgumReinicio = false;  ///< Para conferir que os motores param antes do reinício.

  explicit NavegacaoSimulada(const ConfiguracaoFalhasI2c& config = ConfiguracaoFalhasI2c{}) : supervisor(config) {
    trava.liberar();  // boot aprovado
  }

  void ciclo() {
    agoraMs += CICLO_MS;
    tof.ler(agoraMs);
    bool reiniciar[3] = {false, false, false};
    for (uint8_t i = 0; i < 3; ++i) {
      const DecisaoI2c d = supervisor.registrarLeitura(
          dispositivoDoTof(LADOS[i]), vigia.avaliar(LADOS[i], tof.amostra(LADOS[i]), agoraMs), agoraMs);
      reiniciar[i] = d.acao == AcaoI2c::Reiniciar;
    }
    travaFalha.aplicar(supervisor.motoresDevemParar(), trava);
    for (uint8_t i = 0; i < 3; ++i) {
      if (reiniciar[i]) {
        andandoEmAlgumReinicio = andandoEmAlgumReinicio || !driver.freados();
        const bool voltou = tof.reiniciar(LADOS[i]);
        vigia.sensorReiniciado(LADOS[i], agoraMs);
        supervisor.registrarReinicio(dispositivoDoTof(LADOS[i]), voltou);
      }
    }
    trava.definirPotencia({400, 400});
  }

  void ciclos(int n) {
    for (int i = 0; i < n; ++i) {
      ciclo();
    }
  }

  bool andando() const { return !driver.freados() && driver.ultimaPotencia().esquerda == 400; }
};

}  // namespace

// ---------------------------------------------------------------- SupervisorI2c

void test_sem_falha_fica_ok_e_nao_para_os_motores() {
  SupervisorI2c supervisor;
  uint32_t agora = 0;
  for (Dispositivo d : TODOS) {
    const DecisaoI2c decisao = registrarVarias(supervisor, d, LeituraI2c::Ok, 50, agora);
    TEST_ASSERT_TRUE(decisao.acao == AcaoI2c::Nenhuma);
    TEST_ASSERT_TRUE(supervisor.situacao(d) == SituacaoI2c::Ok);
  }
  TEST_ASSERT_FALSE(supervisor.motoresDevemParar());
  TEST_ASSERT_EQUAL_UINT32(0, supervisor.reiniciosTotais());
}

void test_falha_isolada_tenta_de_novo_sem_parar_os_motores() {
  SupervisorI2c supervisor;
  uint32_t agora = 0;
  DecisaoI2c d =
      registrarVarias(supervisor, Dispositivo::TofFrontal, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C - 1, agora);
  TEST_ASSERT_TRUE(d.acao == AcaoI2c::Nenhuma);
  TEST_ASSERT_TRUE(supervisor.situacao(Dispositivo::TofFrontal) == SituacaoI2c::Tentando);
  TEST_ASSERT_FALSE(supervisor.motoresDevemParar());
  // A nova tentativa funcionou: volta a Ok na hora.
  d = supervisor.registrarLeitura(Dispositivo::TofFrontal, LeituraI2c::Ok, agora += CICLO_MS);
  TEST_ASSERT_TRUE(d.mudou());
  TEST_ASSERT_TRUE(d.depois == SituacaoI2c::Ok);
  TEST_ASSERT_EQUAL_UINT8(0, supervisor.falhasSeguidas(Dispositivo::TofFrontal));
  TEST_ASSERT_EQUAL_UINT32(TENTATIVAS_LEITURA_I2C - 1, supervisor.falhas(Dispositivo::TofFrontal));
}

/// Critério 1: a falha é detectada em cada um dos 4 dispositivos, no número de tentativas configurado.
void test_tentativas_esgotadas_param_os_motores_e_pedem_reinicio_em_cada_dispositivo() {
  for (Dispositivo d : TODOS) {
    SupervisorI2c supervisor;
    uint32_t agora = 0;
    registrarVarias(supervisor, d, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C - 1, agora);
    TEST_ASSERT_FALSE(supervisor.motoresDevemParar());
    const DecisaoI2c decisao = supervisor.registrarLeitura(d, LeituraI2c::Falha, agora += CICLO_MS);
    TEST_ASSERT_TRUE(decisao.acao == AcaoI2c::Reiniciar);
    TEST_ASSERT_TRUE(decisao.antes == SituacaoI2c::Tentando);
    TEST_ASSERT_TRUE(decisao.depois == SituacaoI2c::EmFalha);
    TEST_ASSERT_TRUE(supervisor.motoresDevemParar());
    TEST_ASSERT_EQUAL_UINT32(1, supervisor.reinicios(d));
    for (Dispositivo outro : TODOS) {
      if (outro != d) {
        TEST_ASSERT_TRUE(supervisor.situacao(outro) == SituacaoI2c::Ok);
      }
    }
  }
}

void test_tentativas_vem_da_configuracao() {
  ConfiguracaoFalhasI2c rapida;
  rapida.tentativasLeitura = 2;
  SupervisorI2c supervisor(rapida);
  uint32_t agora = 0;
  TEST_ASSERT_TRUE(registrarVarias(supervisor, Dispositivo::Mpu6050, LeituraI2c::Falha, 2, agora).acao ==
                   AcaoI2c::Reiniciar);

  ConfiguracaoFalhasI2c paciente;
  paciente.tentativasLeitura = 7;
  SupervisorI2c outro(paciente);
  agora = 0;
  TEST_ASSERT_TRUE(registrarVarias(outro, Dispositivo::Mpu6050, LeituraI2c::Falha, 6, agora).acao == AcaoI2c::Nenhuma);
  TEST_ASSERT_TRUE(outro.situacao(Dispositivo::Mpu6050) == SituacaoI2c::Tentando);
  TEST_ASSERT_TRUE(outro.registrarLeitura(Dispositivo::Mpu6050, LeituraI2c::Falha, agora + CICLO_MS).acao ==
                   AcaoI2c::Reiniciar);
}

void test_configuracao_zerada_vale_como_um() {
  ConfiguracaoFalhasI2c zerada;
  zerada.tentativasLeitura = 0;
  zerada.leiturasParaRecuperar = 0;
  SupervisorI2c supervisor(zerada);
  uint32_t agora = 0;
  TEST_ASSERT_TRUE(supervisor.registrarLeitura(Dispositivo::TofDireito, LeituraI2c::Falha, agora).acao ==
                   AcaoI2c::Reiniciar);
  TEST_ASSERT_TRUE(supervisor.registrarLeitura(Dispositivo::TofDireito, LeituraI2c::Ok, agora + CICLO_MS).depois ==
                   SituacaoI2c::Ok);
}

void test_depois_do_reinicio_o_dispositivo_tem_todas_as_tentativas_de_novo() {
  SupervisorI2c supervisor;
  uint32_t agora = 0;
  registrarVarias(supervisor, Dispositivo::TofEsquerdo, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C, agora);
  TEST_ASSERT_EQUAL_UINT8(0, supervisor.falhasSeguidas(Dispositivo::TofEsquerdo));
  // Continua em falha (motores parados), mas só pede outro reinício depois de esgotar as tentativas
  // de novo e de passar a espera.
  agora += ESPERA_ENTRE_REINICIOS_I2C_MS;
  const DecisaoI2c antes =
      registrarVarias(supervisor, Dispositivo::TofEsquerdo, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C - 1, agora);
  TEST_ASSERT_TRUE(antes.acao == AcaoI2c::Nenhuma);
  TEST_ASSERT_TRUE(supervisor.situacao(Dispositivo::TofEsquerdo) == SituacaoI2c::EmFalha);
  TEST_ASSERT_TRUE(supervisor.motoresDevemParar());
  TEST_ASSERT_TRUE(supervisor.registrarLeitura(Dispositivo::TofEsquerdo, LeituraI2c::Falha, agora += CICLO_MS).acao ==
                   AcaoI2c::Reiniciar);
  TEST_ASSERT_EQUAL_UINT32(2, supervisor.reinicios(Dispositivo::TofEsquerdo));
}

void test_reinicios_respeitam_a_espera() {
  SupervisorI2c supervisor;
  uint32_t agora = 1000;
  registrarVarias(supervisor, Dispositivo::TofFrontal, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C, agora);
  const uint32_t primeiro = agora;
  // Tentativas esgotadas antes da espera: nada de reinício, e as falhas continuam contando.
  uint32_t ultimoSemReinicio = 0;
  for (int i = 0; i < 200 && agora + CICLO_MS - primeiro < ESPERA_ENTRE_REINICIOS_I2C_MS; ++i) {
    agora += CICLO_MS;
    TEST_ASSERT_TRUE(supervisor.registrarLeitura(Dispositivo::TofFrontal, LeituraI2c::Falha, agora).acao ==
                     AcaoI2c::Nenhuma);
    ultimoSemReinicio = agora;
  }
  TEST_ASSERT_EQUAL_UINT32(primeiro + ESPERA_ENTRE_REINICIOS_I2C_MS - CICLO_MS, ultimoSemReinicio);
  TEST_ASSERT_TRUE(supervisor.registrarLeitura(Dispositivo::TofFrontal, LeituraI2c::Falha, agora += CICLO_MS).acao ==
                   AcaoI2c::Reiniciar);
  TEST_ASSERT_EQUAL_UINT32(primeiro + ESPERA_ENTRE_REINICIOS_I2C_MS, agora);
}

void test_espera_entre_reinicios_com_o_relogio_dando_a_volta() {
  ConfiguracaoFalhasI2c config;
  config.tentativasLeitura = 1;
  SupervisorI2c supervisor(config);
  const uint32_t quaseNoFim = UINT32_MAX - 100;
  TEST_ASSERT_TRUE(supervisor.registrarLeitura(Dispositivo::Mpu6050, LeituraI2c::Falha, quaseNoFim).acao ==
                   AcaoI2c::Reiniciar);
  // Passou do fim do contador: 300 ms depois ainda não deu a espera de 500 ms; 500 ms depois, deu.
  TEST_ASSERT_TRUE(supervisor.registrarLeitura(Dispositivo::Mpu6050, LeituraI2c::Falha, quaseNoFim + 300).acao ==
                   AcaoI2c::Nenhuma);
  TEST_ASSERT_TRUE(supervisor.registrarLeitura(Dispositivo::Mpu6050, LeituraI2c::Falha, quaseNoFim + 500).acao ==
                   AcaoI2c::Reiniciar);
}

void test_reinicios_esgotados_deixam_o_dispositivo_inoperante() {
  SupervisorI2c supervisor;
  uint32_t agora = 0;
  uint32_t pedidos = 0;
  DecisaoI2c ultima;
  for (int i = 0; i < 400; ++i) {
    agora += CICLO_MS;
    ultima = supervisor.registrarLeitura(Dispositivo::TofDireito, LeituraI2c::Falha, agora);
    if (ultima.acao == AcaoI2c::Reiniciar) {
      ++pedidos;
      supervisor.registrarReinicio(Dispositivo::TofDireito, false);
    }
    if (ultima.depois == SituacaoI2c::Inoperante) {
      break;
    }
  }
  TEST_ASSERT_EQUAL_UINT32(REINICIOS_MAXIMOS_I2C, pedidos);
  TEST_ASSERT_TRUE(ultima.antes == SituacaoI2c::EmFalha);
  TEST_ASSERT_TRUE(ultima.depois == SituacaoI2c::Inoperante);
  TEST_ASSERT_TRUE(supervisor.motoresDevemParar());
  TEST_ASSERT_EQUAL_UINT32(REINICIOS_MAXIMOS_I2C, supervisor.reiniciosSemSucesso(Dispositivo::TofDireito));
}

void test_inoperante_nao_pede_mais_reinicio_mas_volta_com_leituras_boas() {
  ConfiguracaoFalhasI2c config;
  config.tentativasLeitura = 1;
  config.reiniciosMaximos = 1;
  config.esperaEntreReiniciosMs = 0;
  SupervisorI2c supervisor(config);
  uint32_t agora = 0;
  TEST_ASSERT_TRUE(supervisor.registrarLeitura(Dispositivo::Mpu6050, LeituraI2c::Falha, agora).acao ==
                   AcaoI2c::Reiniciar);
  TEST_ASSERT_TRUE(supervisor.registrarLeitura(Dispositivo::Mpu6050, LeituraI2c::Falha, agora += CICLO_MS).depois ==
                   SituacaoI2c::Inoperante);
  const DecisaoI2c depois = registrarVarias(supervisor, Dispositivo::Mpu6050, LeituraI2c::Falha, 100, agora);
  TEST_ASSERT_TRUE(depois.acao == AcaoI2c::Nenhuma);
  TEST_ASSERT_EQUAL_UINT32(1, supervisor.reinicios(Dispositivo::Mpu6050));
  // O dispositivo voltou sozinho (por exemplo, um mau contato): as leituras boas o tiram da falha.
  registrarVarias(supervisor, Dispositivo::Mpu6050, LeituraI2c::Ok, LEITURAS_PARA_RECUPERAR_I2C, agora);
  TEST_ASSERT_TRUE(supervisor.situacao(Dispositivo::Mpu6050) == SituacaoI2c::Ok);
  TEST_ASSERT_FALSE(supervisor.motoresDevemParar());
  // E, se falhar de novo, tem todos os reinícios outra vez.
  TEST_ASSERT_TRUE(supervisor.registrarLeitura(Dispositivo::Mpu6050, LeituraI2c::Falha, agora += CICLO_MS).acao ==
                   AcaoI2c::Reiniciar);
}

void test_recuperar_exige_leituras_boas_seguidas() {
  SupervisorI2c supervisor;
  uint32_t agora = 0;
  registrarVarias(supervisor, Dispositivo::TofFrontal, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C, agora);
  registrarVarias(supervisor, Dispositivo::TofFrontal, LeituraI2c::Ok, LEITURAS_PARA_RECUPERAR_I2C - 1, agora);
  TEST_ASSERT_TRUE(supervisor.situacao(Dispositivo::TofFrontal) == SituacaoI2c::EmFalha);
  // Uma falha no meio zera a contagem das boas: oscilar não solta os motores.
  supervisor.registrarLeitura(Dispositivo::TofFrontal, LeituraI2c::Falha, agora += CICLO_MS);
  registrarVarias(supervisor, Dispositivo::TofFrontal, LeituraI2c::Ok, LEITURAS_PARA_RECUPERAR_I2C - 1, agora);
  TEST_ASSERT_TRUE(supervisor.motoresDevemParar());
  const DecisaoI2c d = supervisor.registrarLeitura(Dispositivo::TofFrontal, LeituraI2c::Ok, agora += CICLO_MS);
  TEST_ASSERT_TRUE(d.antes == SituacaoI2c::EmFalha);
  TEST_ASSERT_TRUE(d.depois == SituacaoI2c::Ok);
  TEST_ASSERT_FALSE(supervisor.motoresDevemParar());
}

void test_motores_so_seguem_com_todos_os_dispositivos_ok() {
  SupervisorI2c supervisor;
  uint32_t agora = 0;
  registrarVarias(supervisor, Dispositivo::TofEsquerdo, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C, agora);
  registrarVarias(supervisor, Dispositivo::Mpu6050, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C, agora);
  registrarVarias(supervisor, Dispositivo::TofEsquerdo, LeituraI2c::Ok, LEITURAS_PARA_RECUPERAR_I2C, agora);
  TEST_ASSERT_TRUE(supervisor.situacao(Dispositivo::TofEsquerdo) == SituacaoI2c::Ok);
  TEST_ASSERT_TRUE(supervisor.motoresDevemParar());  // a IMU ainda está em falha
  registrarVarias(supervisor, Dispositivo::Mpu6050, LeituraI2c::Ok, LEITURAS_PARA_RECUPERAR_I2C, agora);
  TEST_ASSERT_FALSE(supervisor.motoresDevemParar());
}

void test_sem_novidade_nao_muda_nada() {
  SupervisorI2c supervisor;
  uint32_t agora = 0;
  registrarVarias(supervisor, Dispositivo::TofFrontal, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C - 1, agora);
  const DecisaoI2c d = registrarVarias(supervisor, Dispositivo::TofFrontal, LeituraI2c::SemNovidade, 100, agora);
  TEST_ASSERT_FALSE(d.mudou());
  TEST_ASSERT_TRUE(d.acao == AcaoI2c::Nenhuma);
  TEST_ASSERT_EQUAL_UINT8(TENTATIVAS_LEITURA_I2C - 1, supervisor.falhasSeguidas(Dispositivo::TofFrontal));
  TEST_ASSERT_EQUAL_UINT32(TENTATIVAS_LEITURA_I2C - 1, supervisor.falhas(Dispositivo::TofFrontal));
}

void test_descreve_os_dispositivos_em_falha() {
  SupervisorI2c supervisor;
  char texto[TAMANHO_MAX_FALHAS_I2C];
  TEST_ASSERT_EQUAL_size_t(0, supervisor.descreverFalhas(texto, sizeof(texto)));
  TEST_ASSERT_EQUAL_STRING("", texto);

  uint32_t agora = 0;
  registrarVarias(supervisor, Dispositivo::TofFrontal, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C - 1, agora);
  supervisor.descreverFalhas(texto, sizeof(texto));
  TEST_ASSERT_EQUAL_STRING("", texto);  // só tentando: ainda não é falha

  for (Dispositivo d : TODOS) {
    registrarVarias(supervisor, d, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C, agora);
  }
  const size_t n = supervisor.descreverFalhas(texto, sizeof(texto));
  TEST_ASSERT_EQUAL_STRING("tof_esquerdo,tof_frontal,tof_direito,mpu6050", texto);
  TEST_ASSERT_EQUAL_size_t(std::strlen(texto), n);
  TEST_ASSERT_TRUE(n + 1 <= TAMANHO_MAX_FALHAS_I2C);

  char curto[20];
  TEST_ASSERT_EQUAL_size_t(0, supervisor.descreverFalhas(curto, sizeof(curto)));
  // No limite: cabe com o `\0`, e não cabe com um byte a menos.
  TEST_ASSERT_EQUAL_size_t(n, supervisor.descreverFalhas(texto, n + 1));
  TEST_ASSERT_EQUAL_size_t(0, supervisor.descreverFalhas(texto, n));
  TEST_ASSERT_EQUAL_STRING("", curto);
  TEST_ASSERT_EQUAL_size_t(0, supervisor.descreverFalhas(nullptr, 10));
  TEST_ASSERT_EQUAL_size_t(0, supervisor.descreverFalhas(curto, 0));
}

void test_contadores_de_falhas_e_reinicios() {
  SupervisorI2c supervisor;
  uint32_t agora = 0;
  registrarVarias(supervisor, Dispositivo::TofDireito, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C, agora);
  supervisor.registrarReinicio(Dispositivo::TofDireito, true);
  registrarVarias(supervisor, Dispositivo::Mpu6050, LeituraI2c::Falha, TENTATIVAS_LEITURA_I2C, agora);
  supervisor.registrarReinicio(Dispositivo::Mpu6050, false);
  TEST_ASSERT_EQUAL_UINT32(TENTATIVAS_LEITURA_I2C, supervisor.falhas(Dispositivo::TofDireito));
  TEST_ASSERT_EQUAL_UINT32(1, supervisor.reinicios(Dispositivo::TofDireito));
  TEST_ASSERT_EQUAL_UINT32(0, supervisor.reiniciosSemSucesso(Dispositivo::TofDireito));
  TEST_ASSERT_EQUAL_UINT32(1, supervisor.reiniciosSemSucesso(Dispositivo::Mpu6050));
  TEST_ASSERT_EQUAL_UINT32(2, supervisor.reiniciosTotais());
}

void test_nomes_das_situacoes() {
  TEST_ASSERT_EQUAL_STRING("ok", nomeSituacao(SituacaoI2c::Ok));
  TEST_ASSERT_EQUAL_STRING("tentando", nomeSituacao(SituacaoI2c::Tentando));
  TEST_ASSERT_EQUAL_STRING("em_falha", nomeSituacao(SituacaoI2c::EmFalha));
  TEST_ASSERT_EQUAL_STRING("inoperante", nomeSituacao(SituacaoI2c::Inoperante));
  TEST_ASSERT_EQUAL_STRING("?", nomeSituacao(static_cast<SituacaoI2c>(99)));
}

void test_leitura_da_imu_com_barramento_ocupado_nao_e_falha() {
  TEST_ASSERT_TRUE(leituraDaImu(false, false) == LeituraI2c::Ok);
  TEST_ASSERT_TRUE(leituraDaImu(false, true) == LeituraI2c::Ok);
  TEST_ASSERT_TRUE(leituraDaImu(true, false) == LeituraI2c::Falha);
  TEST_ASSERT_TRUE(leituraDaImu(true, true) == LeituraI2c::SemNovidade);
}

// ---------------------------------------------------------------- VigiaTof

void test_vigia_amostra_nova_de_sensor_que_respondeu_e_ok() {
  VigiaTof vigia;
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Frente, amostraEm(EstadoAmostraTof::Valida, 10), 10) == LeituraI2c::Ok);
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Frente, amostraEm(EstadoAmostraTof::ForaDeAlcance, 40), 40) == LeituraI2c::Ok);
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Frente, amostraEm(EstadoAmostraTof::AbaixoDoMinimo, 70), 70) == LeituraI2c::Ok);
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Frente, amostraEm(EstadoAmostraTof::Falha, 80), 80) == LeituraI2c::Falha);
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Frente, amostraEm(EstadoAmostraTof::Falha, 90), 90) == LeituraI2c::Falha);
}

void test_vigia_mesma_amostra_e_sem_novidade_ate_a_idade_maxima() {
  VigiaTof vigia;
  const AmostraTof a = amostraEm(EstadoAmostraTof::Valida, 100);
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Esquerda, a, 100) == LeituraI2c::Ok);
  for (uint32_t t = 110; t <= 100 + IDADE_MAXIMA_LEITURA_TOF_MS; t += CICLO_MS) {
    TEST_ASSERT_TRUE(vigia.avaliar(Lado::Esquerda, a, t) == LeituraI2c::SemNovidade);
  }
  // Uma amostra em falha repetida (sem leitura nova) também não conta duas vezes.
  VigiaTof outra;
  const AmostraTof f = amostraEm(EstadoAmostraTof::Falha, 100);
  TEST_ASSERT_TRUE(outra.avaliar(Lado::Direita, f, 100) == LeituraI2c::Falha);
  TEST_ASSERT_TRUE(outra.avaliar(Lado::Direita, f, 110) == LeituraI2c::SemNovidade);
}

void test_vigia_sensor_parado_falha_uma_vez_por_janela() {
  VigiaTof vigia;
  const AmostraTof a = amostraEm(EstadoAmostraTof::Valida, 0);
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Frente, a, 0) == LeituraI2c::Ok);
  int falhas = 0;
  for (uint32_t t = CICLO_MS; t <= 1000; t += CICLO_MS) {
    if (vigia.avaliar(Lado::Frente, a, t) == LeituraI2c::Falha) {
      ++falhas;
    }
  }
  // 1 s sem medida nova, com idade máxima de 100 ms: uma falha a cada janela de 110 ms (9 em 1 s).
  TEST_ASSERT_EQUAL_INT(1000 / (IDADE_MAXIMA_LEITURA_TOF_MS + CICLO_MS), falhas);
}

void test_vigia_sem_nenhuma_amostra_falha_depois_da_idade_maxima() {
  VigiaTof vigia(50);
  const AmostraTof vazia;  // SemAmostra
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Frente, vazia, 1000) == LeituraI2c::SemNovidade);
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Frente, vazia, 1050) == LeituraI2c::SemNovidade);
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Frente, vazia, 1060) == LeituraI2c::Falha);
}

void test_vigia_depois_do_reinicio_ignora_as_amostras_antigas() {
  VigiaTof vigia;
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Direita, amostraEm(EstadoAmostraTof::Falha, 500), 500) == LeituraI2c::Falha);
  vigia.sensorReiniciado(Lado::Direita, 520);
  // A amostra em falha de antes do reinício ainda está no combinador: não conta.
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Direita, amostraEm(EstadoAmostraTof::Falha, 510), 530) ==
                   LeituraI2c::SemNovidade);
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Direita, amostraEm(EstadoAmostraTof::Falha, 510), 600) ==
                   LeituraI2c::SemNovidade);
  // A primeira medida depois do reinício conta, inclusive a do mesmo instante dele.
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Direita, amostraEm(EstadoAmostraTof::Valida, 520), 610) == LeituraI2c::Ok);
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Direita, amostraEm(EstadoAmostraTof::Valida, 560), 620) == LeituraI2c::Ok);
  // Sem medida nova depois do reinício por mais que a idade máxima: falha.
  VigiaTof outra;
  outra.sensorReiniciado(Lado::Frente, 1000);
  const AmostraTof antiga = amostraEm(EstadoAmostraTof::Falha, 900);
  TEST_ASSERT_TRUE(outra.avaliar(Lado::Frente, antiga, 1000 + IDADE_MAXIMA_LEITURA_TOF_MS) == LeituraI2c::SemNovidade);
  TEST_ASSERT_TRUE(outra.avaliar(Lado::Frente, antiga, 1000 + IDADE_MAXIMA_LEITURA_TOF_MS + CICLO_MS) ==
                   LeituraI2c::Falha);
}

void test_vigia_cada_lado_e_um_dispositivo() {
  TEST_ASSERT_TRUE(dispositivoDoTof(Lado::Frente) == Dispositivo::TofFrontal);
  TEST_ASSERT_TRUE(dispositivoDoTof(Lado::Esquerda) == Dispositivo::TofEsquerdo);
  TEST_ASSERT_TRUE(dispositivoDoTof(Lado::Direita) == Dispositivo::TofDireito);
  // Os lados são acompanhados em separado.
  VigiaTof vigia;
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Frente, amostraEm(EstadoAmostraTof::Falha, 10), 10) == LeituraI2c::Falha);
  TEST_ASSERT_TRUE(vigia.avaliar(Lado::Esquerda, amostraEm(EstadoAmostraTof::Valida, 10), 10) == LeituraI2c::Ok);
}

// ---------------------------------------------------------------- TravaFalhaI2c

void test_trava_para_os_motores_e_solta_quando_a_falha_acaba() {
  MotoresSimulados driver;
  MotoresComTrava trava(driver);
  trava.liberar();  // boot aprovado
  TravaFalhaI2c travaFalha;
  TEST_ASSERT_TRUE(travaFalha.aplicar(false, trava) == MudancaTrava::Nenhuma);
  TEST_ASSERT_TRUE(travaFalha.aplicar(true, trava) == MudancaTrava::Travou);
  TEST_ASSERT_TRUE(trava.travado());
  TEST_ASSERT_TRUE(driver.freados());
  TEST_ASSERT_TRUE(travaFalha.travouPorFalha());
  TEST_ASSERT_TRUE(travaFalha.aplicar(true, trava) == MudancaTrava::Nenhuma);
  trava.definirPotencia({400, 400});  // a navegação tenta andar: vira freio
  TEST_ASSERT_TRUE(driver.freados());
  TEST_ASSERT_TRUE(travaFalha.aplicar(false, trava) == MudancaTrava::Soltou);
  TEST_ASSERT_FALSE(trava.travado());
  trava.definirPotencia({400, 400});
  TEST_ASSERT_FALSE(driver.freados());
}

void test_trava_nao_solta_a_trava_do_boot() {
  MotoresSimulados driver;
  MotoresComTrava trava(driver);  // o boot reprovou: a trava ficou fechada
  TravaFalhaI2c travaFalha;
  TEST_ASSERT_TRUE(travaFalha.aplicar(true, trava) == MudancaTrava::Nenhuma);
  TEST_ASSERT_FALSE(travaFalha.travouPorFalha());
  TEST_ASSERT_TRUE(travaFalha.aplicar(false, trava) == MudancaTrava::Nenhuma);
  TEST_ASSERT_TRUE(trava.travado());
}

// ---------------------------------------------------------------- integração

/// Critérios 2 e 3: o ToF frontal para de responder com o robô andando. Os motores seguem nas
/// tentativas, param quando elas acabam, o sensor é reiniciado pelo XSHUT, volta, e os motores
/// voltam depois das leituras boas.
void test_integracao_tof_frontal_sem_resposta_e_reiniciado_pelo_xshut() {
  NavegacaoSimulada nav;
  nav.ciclos(100);
  TEST_ASSERT_TRUE(nav.andando());
  TEST_ASSERT_EQUAL_UINT32(0, nav.supervisor.falhas(Dispositivo::TofFrontal));

  nav.tof.definirModo(Lado::Frente, ModoTof::SemResposta);
  nav.ciclos(TENTATIVAS_LEITURA_I2C - 1);
  TEST_ASSERT_TRUE(nav.andando());  // ainda nas tentativas
  TEST_ASSERT_TRUE(nav.supervisor.situacao(Dispositivo::TofFrontal) == SituacaoI2c::Tentando);
  nav.ciclo();
  TEST_ASSERT_TRUE(nav.driver.freados());  // tentativas esgotadas: motores parados
  TEST_ASSERT_TRUE(nav.travaFalha.travouPorFalha());
  TEST_ASSERT_EQUAL_UINT32(1, nav.tof.reinicios(Lado::Frente));
  TEST_ASSERT_FALSE(nav.andandoEmAlgumReinicio);  // a trava veio antes do reinício
  TEST_ASSERT_TRUE(nav.supervisor.situacao(Dispositivo::TofFrontal) == SituacaoI2c::EmFalha);

  // O reinício funcionou: o sensor volta a medir a cada 30 ms; com 3 medidas boas, os motores voltam.
  int ciclosParados = 0;
  while (!nav.andando() && ciclosParados < 100) {
    nav.ciclo();
    ++ciclosParados;
  }
  TEST_ASSERT_TRUE(nav.andando());
  TEST_ASSERT_TRUE(ciclosParados <= 8);  // ~70 ms: a primeira medida e mais duas a cada 30 ms
  TEST_ASSERT_TRUE(nav.supervisor.situacao(Dispositivo::TofFrontal) == SituacaoI2c::Ok);
  TEST_ASSERT_EQUAL_UINT32(1, nav.tof.reinicios(Lado::Frente));
  TEST_ASSERT_EQUAL_UINT32(0, nav.supervisor.reiniciosSemSucesso(Dispositivo::TofFrontal));
  nav.ciclos(300);
  TEST_ASSERT_TRUE(nav.andando());
  TEST_ASSERT_EQUAL_UINT32(1, nav.supervisor.reiniciosTotais());
}

/// O ToF esquerdo não volta com o XSHUT: 3 reinícios espaçados de 500 ms, depois inoperante, com os
/// motores parados o tempo todo e os outros dispositivos intactos. Se ele voltar sozinho, os motores
/// voltam também.
void test_integracao_tof_que_nao_volta_fica_inoperante_e_os_motores_seguem_parados() {
  NavegacaoSimulada nav;
  nav.ciclos(50);
  nav.tof.voltarNoReinicio(Lado::Esquerda, false);
  nav.tof.definirModo(Lado::Esquerda, ModoTof::SemResposta);
  nav.ciclos(TENTATIVAS_LEITURA_I2C);
  const uint32_t inicioDaFalha = nav.agoraMs;
  bool andouNaFalha = false;
  while (nav.agoraMs - inicioDaFalha < 3000) {
    nav.ciclo();
    andouNaFalha = andouNaFalha || nav.andando();
  }
  TEST_ASSERT_FALSE(andouNaFalha);
  TEST_ASSERT_FALSE(nav.andandoEmAlgumReinicio);
  TEST_ASSERT_EQUAL_UINT32(REINICIOS_MAXIMOS_I2C, nav.tof.reinicios(Lado::Esquerda));
  TEST_ASSERT_EQUAL_UINT32(REINICIOS_MAXIMOS_I2C, nav.supervisor.reiniciosSemSucesso(Dispositivo::TofEsquerdo));
  TEST_ASSERT_TRUE(nav.supervisor.situacao(Dispositivo::TofEsquerdo) == SituacaoI2c::Inoperante);
  TEST_ASSERT_TRUE(nav.supervisor.situacao(Dispositivo::TofFrontal) == SituacaoI2c::Ok);
  TEST_ASSERT_TRUE(nav.supervisor.situacao(Dispositivo::TofDireito) == SituacaoI2c::Ok);
  char texto[TAMANHO_MAX_FALHAS_I2C];
  nav.supervisor.descreverFalhas(texto, sizeof(texto));
  TEST_ASSERT_EQUAL_STRING("tof_esquerdo", texto);

  nav.tof.definirModo(Lado::Esquerda, ModoTof::Medindo);  // o mau contato voltou
  nav.ciclos(20);
  TEST_ASSERT_TRUE(nav.andando());
  TEST_ASSERT_EQUAL_UINT32(REINICIOS_MAXIMOS_I2C, nav.tof.reinicios(Lado::Esquerda));
}

/// Um ToF que para de medir (responde, mas não entrega medida nova) também é falha.
void test_integracao_tof_que_para_de_medir_e_reiniciado() {
  NavegacaoSimulada nav;
  nav.ciclos(50);
  nav.tof.definirModo(Lado::Direita, ModoTof::Parado);
  int ciclos = 0;
  while (nav.tof.reinicios(Lado::Direita) == 0 && ciclos < 200) {
    nav.ciclo();
    ++ciclos;
  }
  // Uma falha a cada ~110 ms sem medida: 5 tentativas, cerca de 0,55 s.
  TEST_ASSERT_EQUAL_UINT32(1, nav.tof.reinicios(Lado::Direita));
  TEST_ASSERT_TRUE(ciclos >= 50 && ciclos <= 60);
  nav.ciclos(20);
  TEST_ASSERT_TRUE(nav.andando());
}

/// A IMU para de responder: o ângulo para de somar, os motores param nas tentativas esgotadas, o
/// MPU6050 é reiniciado, e tudo volta.
void test_integracao_imu_sem_resposta_e_reiniciada() {
  ImuSimulado imu;
  AcompanhadorGiro giro(imu);
  SupervisorI2c supervisor;
  MotoresSimulados driver;
  MotoresComTrava trava(driver);
  trava.liberar();
  TravaFalhaI2c travaFalha;
  uint32_t agora = 0;
  int reinicios = 0;
  auto ciclo = [&]() {
    agora += CICLO_MS;
    const uint32_t falhasAntes = giro.ultima().falhasLeitura;
    const LeituraGiro leitura = giro.ciclo(CICLO_MS);
    const LeituraI2c resultado = leituraDaImu(leitura.falhasLeitura != falhasAntes, false);
    const DecisaoI2c d = supervisor.registrarLeitura(Dispositivo::Mpu6050, resultado, agora);
    if (d.acao == AcaoI2c::Reiniciar) {
      ++reinicios;
      imu.simularFalha(false);  // `ImuMpu6050::iniciar()` funcionou
      supervisor.registrarReinicio(Dispositivo::Mpu6050, true);
    }
    travaFalha.aplicar(supervisor.motoresDevemParar(), trava);
    trava.definirPotencia({400, 400});
  };

  giro.iniciarCalibracao();
  for (int i = 0; i < AMOSTRAS_CALIBRACAO_GIRO; ++i) {
    ciclo();
  }
  TEST_ASSERT_TRUE(giro.ultima().valida);
  imu.definirTaxa(900);  // girando a 90 °/s
  for (int i = 0; i < 10; ++i) {
    ciclo();
  }
  const int32_t anguloAntes = giro.ultima().anguloMiligraus;
  TEST_ASSERT_EQUAL_INT32(9000, anguloAntes);

  imu.simularFalha(true);
  for (int i = 0; i < TENTATIVAS_LEITURA_I2C - 1; ++i) {
    ciclo();
  }
  TEST_ASSERT_FALSE(driver.freados());
  ciclo();
  TEST_ASSERT_EQUAL_INT(1, reinicios);
  TEST_ASSERT_TRUE(driver.freados());
  TEST_ASSERT_EQUAL_INT32(anguloAntes, giro.ultima().anguloMiligraus);  // nada somado na falha
  TEST_ASSERT_FALSE(giro.ultima().valida);

  for (int i = 0; i < LEITURAS_PARA_RECUPERAR_I2C - 1; ++i) {
    ciclo();
  }
  TEST_ASSERT_TRUE(driver.freados());
  ciclo();
  TEST_ASSERT_FALSE(driver.freados());
  TEST_ASSERT_TRUE(giro.ultima().valida);
  TEST_ASSERT_TRUE(supervisor.situacao(Dispositivo::Mpu6050) == SituacaoI2c::Ok);
  TEST_ASSERT_EQUAL_INT32(anguloAntes + 3 * 900, giro.ultima().anguloMiligraus);
}

/// Enquanto a navegação reinicia um ToF, ela segura o barramento, e a leitura da IMU falha por
/// espera esgotada. Isso não é falha da IMU: ela não é reiniciada, e os motores não param por ela.
void test_integracao_barramento_ocupado_nao_derruba_a_imu() {
  ImuSimulado imu;
  AcompanhadorGiro giro(imu);
  SupervisorI2c supervisor;
  uint32_t agora = 0;
  imu.simularFalha(true);  // o `Uso` não veio: `lerTaxaGiroZ` devolve `false`
  for (int i = 0; i < 100; ++i) {
    agora += CICLO_MS;
    const uint32_t falhasAntes = giro.ultima().falhasLeitura;
    const LeituraGiro leitura = giro.ciclo(CICLO_MS);
    const DecisaoI2c d = supervisor.registrarLeitura(
        Dispositivo::Mpu6050, leituraDaImu(leitura.falhasLeitura != falhasAntes, true), agora);
    TEST_ASSERT_TRUE(d.acao == AcaoI2c::Nenhuma);
  }
  TEST_ASSERT_TRUE(supervisor.situacao(Dispositivo::Mpu6050) == SituacaoI2c::Ok);
  TEST_ASSERT_FALSE(supervisor.motoresDevemParar());
  TEST_ASSERT_EQUAL_UINT32(0, supervisor.falhas(Dispositivo::Mpu6050));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_sem_falha_fica_ok_e_nao_para_os_motores);
  RUN_TEST(test_falha_isolada_tenta_de_novo_sem_parar_os_motores);
  RUN_TEST(test_tentativas_esgotadas_param_os_motores_e_pedem_reinicio_em_cada_dispositivo);
  RUN_TEST(test_tentativas_vem_da_configuracao);
  RUN_TEST(test_configuracao_zerada_vale_como_um);
  RUN_TEST(test_depois_do_reinicio_o_dispositivo_tem_todas_as_tentativas_de_novo);
  RUN_TEST(test_reinicios_respeitam_a_espera);
  RUN_TEST(test_espera_entre_reinicios_com_o_relogio_dando_a_volta);
  RUN_TEST(test_reinicios_esgotados_deixam_o_dispositivo_inoperante);
  RUN_TEST(test_inoperante_nao_pede_mais_reinicio_mas_volta_com_leituras_boas);
  RUN_TEST(test_recuperar_exige_leituras_boas_seguidas);
  RUN_TEST(test_motores_so_seguem_com_todos_os_dispositivos_ok);
  RUN_TEST(test_sem_novidade_nao_muda_nada);
  RUN_TEST(test_descreve_os_dispositivos_em_falha);
  RUN_TEST(test_contadores_de_falhas_e_reinicios);
  RUN_TEST(test_nomes_das_situacoes);
  RUN_TEST(test_leitura_da_imu_com_barramento_ocupado_nao_e_falha);
  RUN_TEST(test_vigia_amostra_nova_de_sensor_que_respondeu_e_ok);
  RUN_TEST(test_vigia_mesma_amostra_e_sem_novidade_ate_a_idade_maxima);
  RUN_TEST(test_vigia_sensor_parado_falha_uma_vez_por_janela);
  RUN_TEST(test_vigia_sem_nenhuma_amostra_falha_depois_da_idade_maxima);
  RUN_TEST(test_vigia_depois_do_reinicio_ignora_as_amostras_antigas);
  RUN_TEST(test_vigia_cada_lado_e_um_dispositivo);
  RUN_TEST(test_trava_para_os_motores_e_solta_quando_a_falha_acaba);
  RUN_TEST(test_trava_nao_solta_a_trava_do_boot);
  RUN_TEST(test_integracao_tof_frontal_sem_resposta_e_reiniciado_pelo_xshut);
  RUN_TEST(test_integracao_tof_que_nao_volta_fica_inoperante_e_os_motores_seguem_parados);
  RUN_TEST(test_integracao_tof_que_para_de_medir_e_reiniciado);
  RUN_TEST(test_integracao_imu_sem_resposta_e_reiniciada);
  RUN_TEST(test_integracao_barramento_ocupado_nao_derruba_a_imu);
  return UNITY_END();
}
