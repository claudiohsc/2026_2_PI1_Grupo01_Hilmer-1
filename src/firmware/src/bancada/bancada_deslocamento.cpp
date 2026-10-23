/// @file bancada_deslocamento.cpp
/// @brief Firmware de bancada do deslocamento sem encoder (#380): aciona os motores por um tempo,
/// roda o mesmo `DeslocamentoEstimado` do robô e manda pela serial o log de cada ciclo, para a
/// equipe comparar a estimativa com a régua.
///
/// Não é o firmware do robô: só entra no ambiente `bancada_deslocamento` do `platformio.ini`. O menu
/// `scripts/bancada_deslocamento.py` grava este programa e conduz o roteiro; também dá para usar
/// direto pelo `pio device monitor -e bancada_deslocamento` (comando `AJUDA`).
///
/// SEGURANÇA: os motores só giram durante `ANDAR` e `GIRAR`, por no máximo `MAX_DURACAO_MS`, e
/// qualquer byte recebido pela serial durante o movimento freia os motores na hora.
///
/// Protocolo: um comando por linha; cada resposta é uma linha JSON com o campo `tipo`. Linhas que
/// começam com `#` são texto para quem lê o monitor.

#include <Arduino.h>
#include <Wire.h>

#if !defined(ARDUINO_USB_CDC_ON_BOOT) || !ARDUINO_USB_CDC_ON_BOOT
#error "Serial pela USB nativa: defina ARDUINO_USB_MODE=1 e ARDUINO_USB_CDC_ON_BOOT=1 no platformio.ini (Pinos.h)"
#endif

#include "ConfiguracaoDeslocamento.h"
#include "CorretorDeslocamentoTof.h"
#include "DeslocamentoEstimado.h"
#include "MotoresDrv8833.h"
#include "Odometria.h"
#include "Pinos.h"
#include "TofVl53l0x.h"

using namespace micromouse;

namespace {

/// Ciclo da estimativa: o mesmo da tarefa de navegação.
constexpr uint32_t PERIODO_CICLO_MS = 10;

/// Intervalo entre linhas de log durante um movimento.
constexpr uint32_t PERIODO_LOG_MS = 50;

/// Maior duração aceita para `ANDAR` e `GIRAR`.
constexpr long MAX_DURACAO_MS = 10000;

MotoresDrv8833 motoresDrv(pinos::MOTOR_ESQ_IN1, pinos::MOTOR_ESQ_IN2, pinos::MOTOR_DIR_IN3,
                          pinos::MOTOR_DIR_IN4, false, true);
DeslocamentoEstimado deslocamento(motoresDrv);
Odometria odometria;
TofVl53l0x tof(Wire, {pinos::XSHUT_FRONTAL, pinos::ENDERECO_TOF_FRONTAL},
               {pinos::XSHUT_ESQ, pinos::ENDERECO_TOF_ESQ}, {pinos::XSHUT_DIR, pinos::ENDERECO_TOF_DIR});

/// Se as leituras do ToF corrigem a posição (`CORRECAO ON|OFF`).
bool correcaoLigada = true;

/// Leitura mais recente do ToF e instante da amostra mais antiga dela (para saber se é nova).
DistanciasLaterais ultimaLeitura = {0, 0, 0};
bool ultimaLeituraValida = false;
uint32_t instanteUltimaLeituraUsada = 0;

const char* nomeResultado(ResultadoCorrecao resultado) {
  switch (resultado) {
    case ResultadoCorrecao::SemReferencia:
      return "sem_referencia";
    case ResultadoCorrecao::PelaFrente:
      return "pela_frente";
    case ResultadoCorrecao::PelaLateral:
      return "pela_lateral";
    case ResultadoCorrecao::Descartada:
      return "descartada";
    case ResultadoCorrecao::Girando:
      return "girando";
  }
  return "?";
}

/// `true` se chegou algum byte pela serial (descarta o que chegou).
bool interrompido() {
  if (Serial.available() == 0) {
    return false;
  }
  while (Serial.available() > 0) {
    Serial.read();
  }
  return true;
}

void imprimirAjuda() {
  Serial.println("# Bancada do deslocamento sem encoder (#380). Comandos (um por linha):");
  Serial.println("#   INFO            configuracao em uso (tabela, limites, encoders)");
  Serial.println("#   VEL p           velocidade que a tabela supoe para a potencia p, em mm/s");
  Serial.println("#   ZERAR           robo no centro de (0, 0) virado para o Norte, pose confiavel");
  Serial.println("#   CORRECAO ON|OFF liga ou desliga a correcao pelo ToF");
  Serial.println("#   ANDAR p ms      as duas rodas com potencia p por ms (qualquer tecla para)");
  Serial.println("#   GIRAR p ms      esquerda +p e direita -p por ms (giro para a direita)");
  Serial.println("#   LER             uma linha de estado, sem mover");
  Serial.println("#   AJUDA           esta lista");
  Serial.println("# ATENCAO: ANDAR e GIRAR acionam os motores. Robo no chao, com espaco livre.");
}

void imprimirInfo() {
  const ConfiguracaoDeslocamento config;
  Serial.printf("{\"tipo\":\"info\",\"programa\":\"bancada_deslocamento\",\"encoders_instalados\":%s,"
                "\"tabela\":[",
                ENCODERS_INSTALADOS ? "true" : "false");
  for (uint8_t i = 0; i < config.pontosTabela; i++) {
    Serial.printf(i == 0 ? "[%d,%u]" : ",[%d,%u]", config.tabela[i].potencia,
                  config.tabela[i].velocidadeMmPorS);
  }
  Serial.printf("],\"sensor_frontal_ao_centro_mm\":%u,\"avanco_sensores_laterais_mm\":%d,"
                "\"frontal_maxima_para_corrigir_mm\":%u,\"limiar_parede_lateral_mm\":%u,"
                "\"histerese_parede_lateral_mm\":%u,\"correcao_maxima_mm\":%u,"
                "\"maxima_sem_correcao_mm\":%u,\"correcao_ligada\":%s,\"tof_ok\":%s}\n",
                config.distanciaSensorFrontalAoCentroMm, config.avancoSensoresLateraisMm,
                config.distanciaFrontalMaximaParaCorrigirMm, config.limiarParedeLateralMm,
                config.histereseParedeLateralMm, config.correcaoMaximaMm,
                config.distanciaMaximaSemCorrecaoMm, correcaoLigada ? "true" : "false",
                tof.sensorIniciado(Lado::Frente) && tof.sensorIniciado(Lado::Esquerda) &&
                        tof.sensorIniciado(Lado::Direita)
                    ? "true"
                    : "false");
}

/// Lê o ToF e devolve a leitura se ela for nova (ainda não usada pela correção).
const DistanciasLaterais* lerTofNovo() {
  ultimaLeituraValida = tof.lerDistanciasLaterais(ultimaLeitura);
  if (!ultimaLeituraValida) {
    return nullptr;
  }
  const uint32_t instante = millis() - tof.idadeUltimaLeituraMs();
  if (instante == instanteUltimaLeituraUsada) {
    return nullptr;
  }
  instanteUltimaLeituraUsada = instante;
  return correcaoLigada ? &ultimaLeitura : nullptr;
}

/// Uma linha de estado: potência, pulsos, posição, pose, confiança e ToF.
void imprimirEstado(const char* tipo, uint32_t tMs) {
  const PotenciaRodas p = deslocamento.estimador().potencia();
  const Pose pose = odometria.pose();
  Serial.printf("{\"tipo\":\"%s\",\"t_ms\":%lu,\"pot_esq\":%d,\"pot_dir\":%d,\"pulsos_esq\":%ld,"
                "\"pulsos_dir\":%ld,\"norte_mm\":%.1f,\"leste_mm\":%.1f,\"linha\":%u,\"coluna\":%u,"
                "\"orientacao\":\"%c\",\"pose_valida\":%s,\"sem_correcao_mm\":%lu,"
                "\"tof_frente_mm\":%u,\"tof_esq_mm\":%u,\"tof_dir_mm\":%u,\"tof_valida\":%s}\n",
                tipo, static_cast<unsigned long>(tMs), p.esquerda, p.direita,
                static_cast<long>(deslocamento.encoderEsquerdo().lerContagem()),
                static_cast<long>(deslocamento.encoderDireito().lerContagem()),
                odometria.posicaoNorteMicrometros() / 1000.0, odometria.posicaoLesteMicrometros() / 1000.0,
                pose.celula.linha, pose.celula.coluna, letraDaDirecao(pose.direcao),
                odometria.posicaoValida() && deslocamento.poseConfiavel() ? "true" : "false",
                static_cast<unsigned long>(deslocamento.distanciaSemCorrecaoMm()), ultimaLeitura.frenteMm,
                ultimaLeitura.esquerdaMm, ultimaLeitura.direitaMm, ultimaLeituraValida ? "true" : "false");
}

/// Aciona as rodas com `potencia` por `duracaoMs`, rodando o ciclo da estimativa a cada 10 ms.
void mover(const PotenciaRodas& potencia, long duracaoMs, const char* nome) {
  uint32_t correcoes = 0;
  uint32_t descartadas = 0;
  int32_t somaCorrecoesUm = 0;
  const double inicioNorteMm = odometria.posicaoNorteMicrometros() / 1000.0;
  const double inicioLesteMm = odometria.posicaoLesteMicrometros() / 1000.0;
  bool parou = false;

  deslocamento.motores().definirPotencia(potencia);
  const uint32_t inicioMs = millis();
  uint32_t ultimoCicloMs = inicioMs;
  uint32_t ultimoLogMs = 0;
  while (millis() - inicioMs < static_cast<uint32_t>(duracaoMs)) {
    if (interrompido()) {
      parou = true;
      break;
    }
    const uint32_t agoraMs = millis();
    if (agoraMs - ultimoCicloMs < PERIODO_CICLO_MS) {
      delay(1);
      continue;
    }
    const uint32_t dtMs = agoraMs - ultimoCicloMs;
    ultimoCicloMs = agoraMs;
    const ResultadoCorrecao r = deslocamento.ciclo(dtMs, odometria, lerTofNovo());
    if (r == ResultadoCorrecao::PelaFrente || r == ResultadoCorrecao::PelaLateral ||
        r == ResultadoCorrecao::Descartada) {
      const Correcao& c = deslocamento.ultimaCorrecao();
      if (r == ResultadoCorrecao::Descartada) {
        descartadas++;
      } else {
        correcoes++;
        somaCorrecoesUm += c.deltaUm;
      }
      Serial.printf("{\"tipo\":\"correcao\",\"t_ms\":%lu,\"resultado\":\"%s\",\"delta_mm\":%.1f,"
                    "\"norte_mm\":%.1f,\"leste_mm\":%.1f,\"tof_frente_mm\":%u,\"tof_esq_mm\":%u,"
                    "\"tof_dir_mm\":%u}\n",
                    static_cast<unsigned long>(agoraMs - inicioMs), nomeResultado(r), c.deltaUm / 1000.0,
                    odometria.posicaoNorteMicrometros() / 1000.0,
                    odometria.posicaoLesteMicrometros() / 1000.0, ultimaLeitura.frenteMm,
                    ultimaLeitura.esquerdaMm, ultimaLeitura.direitaMm);
    }
    if (agoraMs - ultimoLogMs >= PERIODO_LOG_MS) {
      ultimoLogMs = agoraMs;
      imprimirEstado("log", agoraMs - inicioMs);
    }
  }
  deslocamento.motores().parar();
  const uint32_t duracaoReal = millis() - inicioMs;
  // Um último ciclo, já parado, para a estimativa terminar no instante da parada.
  deslocamento.ciclo(0, odometria, nullptr);

  Serial.printf("{\"tipo\":\"fim_movimento\",\"movimento\":\"%s\",\"interrompido\":%s,"
                "\"duracao_ms\":%lu,\"avanco_norte_mm\":%.1f,\"avanco_leste_mm\":%.1f,"
                "\"correcoes\":%lu,\"descartadas\":%lu,\"soma_correcoes_mm\":%.1f}\n",
                nome, parou ? "true" : "false", static_cast<unsigned long>(duracaoReal),
                odometria.posicaoNorteMicrometros() / 1000.0 - inicioNorteMm,
                odometria.posicaoLesteMicrometros() / 1000.0 - inicioLesteMm,
                static_cast<unsigned long>(correcoes), static_cast<unsigned long>(descartadas),
                somaCorrecoesUm / 1000.0);
  imprimirEstado("estado", duracaoReal);
}

/// Separa "COMANDO a b" em até dois números inteiros.
void lerArgumentos(const String& resto, long& a, long& b) {
  const int espaco = resto.indexOf(' ');
  a = resto.toInt();
  b = espaco < 0 ? 0 : resto.substring(espaco + 1).toInt();
}

long limitar(long valor, long minimo, long maximo) {
  return valor < minimo ? minimo : (valor > maximo ? maximo : valor);
}

void executar(String linha) {
  linha.trim();
  linha.toUpperCase();
  if (linha.length() == 0) {
    return;
  }
  const int espaco = linha.indexOf(' ');
  const String comando = espaco < 0 ? linha : linha.substring(0, espaco);
  String resto = espaco < 0 ? String() : linha.substring(espaco + 1);
  resto.trim();
  long a = 0;
  long b = 0;
  lerArgumentos(resto, a, b);

  if (comando == "INFO") {
    imprimirInfo();
  } else if (comando == "VEL") {
    const int16_t potencia = static_cast<int16_t>(limitar(a, -POTENCIA_MAXIMA, POTENCIA_MAXIMA));
    Serial.printf("{\"tipo\":\"vel\",\"potencia\":%d,\"mm_por_s\":%ld}\n", potencia,
                  static_cast<long>(deslocamento.estimador().velocidadeMmPorS(potencia)));
  } else if (comando == "ZERAR") {
    deslocamento.motores().parar();
    deslocamento.reiniciar(odometria, {{0, 0}, Direcao::Norte});
    imprimirEstado("estado", 0);
  } else if (comando == "CORRECAO") {
    correcaoLigada = resto != "OFF";
    Serial.printf("{\"tipo\":\"correcao_ligada\",\"valor\":%s}\n", correcaoLigada ? "true" : "false");
  } else if (comando == "ANDAR" || comando == "GIRAR") {
    const int16_t potencia = static_cast<int16_t>(limitar(a, -POTENCIA_MAXIMA, POTENCIA_MAXIMA));
    const long duracao = limitar(b, 0, MAX_DURACAO_MS);
    const PotenciaRodas rodas = comando == "ANDAR" ? PotenciaRodas{potencia, potencia}
                                                   : PotenciaRodas{potencia, static_cast<int16_t>(-potencia)};
    mover(rodas, duracao, comando == "ANDAR" ? "andar" : "girar");
  } else if (comando == "LER") {
    lerTofNovo();
    imprimirEstado("estado", 0);
  } else if (comando == "AJUDA" || comando == "?") {
    imprimirAjuda();
  } else {
    Serial.printf("{\"tipo\":\"erro\",\"mensagem\":\"comando desconhecido: %s\"}\n", comando.c_str());
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  motoresDrv.iniciar();  // começa freado
  deslocamento.motores().parar();
  Wire.begin(pinos::I2C_SDA, pinos::I2C_SCL);
  const bool tofOk = tof.iniciar();
  delay(500);  // tempo para o monitor serial abrir a porta USB
  imprimirAjuda();
  if (!tofOk) {
    Serial.println("# ATENCAO: algum ToF nao iniciou; as correcoes nao vao acontecer.");
  }
  deslocamento.reiniciar(odometria, {{0, 0}, Direcao::Norte});
  Serial.println("{\"tipo\":\"pronto\",\"programa\":\"bancada_deslocamento\"}");
}

void loop() {
  static String linha;
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      executar(linha);
      linha = "";
    } else if (linha.length() < 64) {
      linha += c;
    }
  }
  delay(1);
}
