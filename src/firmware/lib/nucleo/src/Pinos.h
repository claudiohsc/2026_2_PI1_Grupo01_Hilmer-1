#pragma once

/// @file Pinos.h
/// @brief Pinagem do ESP32-C3, conforme os diagramas de hardware
/// (`docs/04-projeto-conceitual/hardware-diagramas.md`).
///
/// Qualquer mudança no esquemático elétrico deve ser refletida aqui.

namespace micromouse {
namespace pinos {

/// @name Driver de motores DRV8833 (ponte H dupla, sinais PWM)
/// @{
constexpr int MOTOR_A_IN1 = 4;  ///< GPIO4 → AIN1 (motor M1).
constexpr int MOTOR_A_IN2 = 5;  ///< GPIO5 → AIN2 (motor M1).
constexpr int MOTOR_B_IN1 = 6;  ///< GPIO6 → BIN1 (motor M2).
constexpr int MOTOR_B_IN2 = 7;  ///< GPIO7 → BIN2 (motor M2).
/// @}

/// @name Encoders de quadratura (canais A e B de cada roda)
/// @{
constexpr int ENCODER_ESQ_A = 0;   ///< GPIO0 ← Enc_L_A (roda esquerda).
constexpr int ENCODER_ESQ_B = 1;   ///< GPIO1 ← Enc_L_B (roda esquerda).
constexpr int ENCODER_DIR_A = 3;   ///< GPIO3 ← Enc_R_A (roda direita).
constexpr int ENCODER_DIR_B = 10;  ///< GPIO10 ← Enc_R_B (roda direita).
/// @}

/// @name LiDAR (UART)
/// Atenção: GPIO20 e GPIO21 são os pinos padrão da UART0 do ESP32-C3, a mesma
/// usada pelo `Serial` (log e gravação). Validar com a frente de hardware antes
/// dos testes de bancada.
/// @{
constexpr int LIDAR_UART_RX = 20;  ///< GPIO20 ← TX do LiDAR (recepção no ESP32).
constexpr int LIDAR_UART_TX = 21;  ///< GPIO21 → RX do LiDAR (transmissão do ESP32).
/// @}

}  // namespace pinos
}  // namespace micromouse
