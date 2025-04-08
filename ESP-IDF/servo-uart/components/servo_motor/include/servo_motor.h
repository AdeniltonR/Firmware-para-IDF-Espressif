/*
 * NOME: Adenilton Ribeiro
 * DATA: 03/04/2025
 * PROJETO: Servo Motor
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Controle de dois servo motores usando RTOS.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: - https://docs.espressif.com/projects/esp-iot-solution/en/latest/motor/servo.html
*/

// ========================================================================================================
/**
 * @brief servo_motor.h
 * 
 */
#ifndef SERVO_MOTOR_H
#define SERVO_MOTOR_H

// ========================================================================================================
// ---BIBLIOTECA---

#include "esp_err.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"

// ========================================================================================================
/**
 * @brief Configuração do canal do servo motor
 *
 */
typedef struct {
    gpio_num_t servo_pin[LEDC_CHANNEL_MAX];     /**< Número do pino de saída PWM */
    ledc_channel_t ch[LEDC_CHANNEL_MAX];        /**< Canal LEDC utilizado */
} servo_channel_t;

// ========================================================================================================
/**
 * @brief Configuração do servo motor
 *
 */
typedef struct {
    uint16_t max_angle;        /**< Ângulo máximo do servo */
    uint16_t min_width_us;     /**< Largura de pulso correspondente ao ângulo mínimo (normalmente 500us) */
    uint16_t max_width_us;     /**< Largura de pulso correspondente ao ângulo máximo (normalmente 2500us) */
    uint32_t freq;             /**< Frequência PWM */
    ledc_timer_t timer_number; /**< Número do timer LEDC */
    servo_channel_t channels;  /**< Canais a serem utilizados */
    uint8_t channel_number;    /**< Número total de canais */
} servo_config_t;

#ifdef __cplusplus
extern "C" {
#endif

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

esp_err_t iot_servo_init(ledc_mode_t speed_mode, const servo_config_t *config);
esp_err_t iot_servo_deinit(ledc_mode_t speed_mode);
esp_err_t iot_servo_write_angle(ledc_mode_t speed_mode, uint8_t channel, float angle);
esp_err_t iot_servo_read_angle(ledc_mode_t speed_mode, uint8_t channel, float *angle);

#ifdef __cplusplus
}
#endif

#endif //servo_motor.h