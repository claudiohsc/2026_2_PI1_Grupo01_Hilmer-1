#pragma once

/// @file ConfiguracaoFalhasI2c.h
/// @brief Parâmetros da detecção de falha no I²C e das novas tentativas (#241).
///
/// ATENÇÃO: os valores abaixo são PROVISÓRIOS. Nada foi medido com o robô montado: ajuste na
/// bancada, soltando um fio do I²C ou o XSHUT de um ToF com o robô ligado. Para mudar um valor,
/// altere só este arquivo.
///
/// Ritmo das tentativas: a navegação lê os 3 VL53L0X a cada 10 ms, e a tarefa da IMU lê o MPU6050
/// também a cada 10 ms. Cada leitura que falha é uma tentativa.

#include <stdint.h>

namespace micromouse {

/// Leituras seguidas com falha que o dispositivo pode ter antes de ser declarado em falha. Até
/// esse número, a leitura só é repetida no ciclo seguinte. No limite, os motores param e o
/// dispositivo é reiniciado. É o mesmo número de falhas que invalida o ângulo de giro
/// (`FALHAS_SEGUIDAS_GIRO_INVALIDO`, #378).
///
/// Com 5 tentativas, um dispositivo que não responde é declarado em falha em cerca de 50 ms. Um ToF
/// que responde mas parou de medir conta uma falha a cada `IDADE_MAXIMA_LEITURA_TOF_MS` sem medida
/// nova (`VigiaTof`), então leva cerca de 0,55 s.
constexpr uint8_t TENTATIVAS_LEITURA_I2C = 5;

/// Leituras boas seguidas que tiram o dispositivo da falha. Mais de uma, para um sensor que oscila
/// entre responder e não responder não ficar ligando e desligando os motores.
constexpr uint8_t LEITURAS_PARA_RECUPERAR_I2C = 3;

/// Reinícios seguidos sem o dispositivo voltar a responder antes de ele ser declarado inoperante.
/// Inoperante, ele não é mais reiniciado, e os motores ficam parados até ele voltar sozinho ou o
/// robô ser reiniciado.
constexpr uint8_t REINICIOS_MAXIMOS_I2C = 3;

/// Espera mínima entre dois reinícios do mesmo dispositivo, em ms. O reinício de um VL53L0X segura
/// o barramento por algumas dezenas de ms; reiniciar sem pausa tiraria o I²C da IMU.
constexpr uint32_t ESPERA_ENTRE_REINICIOS_I2C_MS = 500;

/// Parâmetros em uso pelo `SupervisorI2c`. Os valores padrão vêm das constantes acima; os testes
/// alteram os campos para cobrir outros cenários.
struct ConfiguracaoFalhasI2c {
  uint8_t tentativasLeitura = TENTATIVAS_LEITURA_I2C;
  uint8_t leiturasParaRecuperar = LEITURAS_PARA_RECUPERAR_I2C;
  uint8_t reiniciosMaximos = REINICIOS_MAXIMOS_I2C;
  uint32_t esperaEntreReiniciosMs = ESPERA_ENTRE_REINICIOS_I2C_MS;
};

}  // namespace micromouse
