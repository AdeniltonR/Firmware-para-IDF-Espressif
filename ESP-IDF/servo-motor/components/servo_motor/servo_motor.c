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
// ---BIBLIOTECA---

#include "servo_motor.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (servo)
static const char *TAG = "servo";

#define SERVO_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

#define SERVO_LEDC_INIT_BITS LEDC_TIMER_10_BIT
#define SERVO_FREQ_MIN       50
#define SERVO_FREQ_MAX       400

static uint32_t g_full_duty = 0;
static servo_config_t g_cfg[LEDC_SPEED_MODE_MAX] = {0};

// ========================================================================================================
/**
 * @brief Calcula o duty cycle PWM para um determinado ângulo
 * @param speed_mode Modo de velocidade LEDC
 * @param angle Ângulo desejado
 * @return Valor do duty cycle
 */
static uint32_t calculate_duty(ledc_mode_t speed_mode, float angle) {
    float angle_us = angle / g_cfg[speed_mode].max_angle * (g_cfg[speed_mode].max_width_us - g_cfg[speed_mode].min_width_us) + g_cfg[speed_mode].min_width_us;
    ESP_LOGD(TAG, "Angulo por us: %f", angle_us);
    uint32_t duty = (uint32_t)((float)g_full_duty * (angle_us) * g_cfg[speed_mode].freq / (1000000.0f));
    return duty;
}

// ========================================================================================================
/**
 * @brief Calcula o ângulo atual com base no duty cycle
 * @param speed_mode Modo de velocidade LEDC
 * @param duty Valor atual do duty cycle
 * @return Ângulo calculado
 */
static float calculate_angle(ledc_mode_t speed_mode, uint32_t duty) {
    float angle_us = (float)duty * 1000000.0f / (float)g_full_duty / (float)g_cfg[speed_mode].freq;
    angle_us -= g_cfg[speed_mode].min_width_us;
    angle_us = angle_us < 0.0f ? 0.0f : angle_us;
    float angle = angle_us * g_cfg[speed_mode].max_angle / (g_cfg[speed_mode].max_width_us - g_cfg[speed_mode].min_width_us);
    return angle;
}

// ========================================================================================================
/**
  * @brief Inicializa o LEDC para controlar o servo
  *
  * @param speed_mode Seleciona o grupo de canais LEDC com modo de velocidade especificado.
  *                  Nota: Nem todos os dispositivos suportam modo de alta velocidade.
  * @param config Ponteiro para a estrutura de configuração do servo
  *
  * @return
  *     - ESP_OK Sucesso
  *     - ESP_ERR_INVALID_ARG Erro de parâmetro
  *     - ESP_FAIL Falha ao configurar LEDC
  */
esp_err_t iot_servo_init(ledc_mode_t speed_mode, const servo_config_t *config) {
    esp_err_t ret;
    SERVO_CHECK(NULL != config, "Ponteiro de configuracao invalido", ESP_ERR_INVALID_ARG);
    SERVO_CHECK(config->channel_number > 0 && config->channel_number <= LEDC_CHANNEL_MAX, "Numero de canais do servo fora do intervalo", ESP_ERR_INVALID_ARG);
    SERVO_CHECK(config->freq <= SERVO_FREQ_MAX && config->freq >= SERVO_FREQ_MIN, "Frequencia PWM do servo fora do intervalo", ESP_ERR_INVALID_ARG);
    uint64_t pin_mask = 0;
    uint32_t ch_mask = 0;
    for (size_t i = 0; i < config->channel_number; i++) {
        uint64_t _pin_mask = 1ULL << config->channels.servo_pin[i];
        uint32_t _ch_mask = 1UL << config->channels.ch[i];
        SERVO_CHECK(!(pin_mask & _pin_mask), "GPIO do servo duplicado", ESP_ERR_INVALID_ARG);
        SERVO_CHECK(!(ch_mask & _ch_mask), "Canal do servo duplicado", ESP_ERR_INVALID_ARG);
        SERVO_CHECK(GPIO_IS_VALID_OUTPUT_GPIO(config->channels.servo_pin[i]), "servo gpio invalid", ESP_ERR_INVALID_ARG);
        pin_mask |= _pin_mask;
        ch_mask |= _ch_mask;
    }

    ledc_timer_config_t ledc_timer = {
        .clk_cfg = LEDC_AUTO_CLK,
        .duty_resolution = SERVO_LEDC_INIT_BITS,     // Resolução do duty cycle PWM
        .freq_hz = config->freq,                     // Frequência do sinal PWM
        .speed_mode = speed_mode,                    // Modo do timer
        .timer_num = config->timer_number            // Índice do timer
    };
    ret = ledc_timer_config(&ledc_timer);
    SERVO_CHECK(ESP_OK == ret, "Falha na configuracao do timer LEDC", ESP_FAIL);
    for (size_t i = 0; i < config->channel_number; i++) {
        ledc_channel_config_t ledc_ch = {
            .intr_type  = LEDC_INTR_DISABLE,
            .channel    = config->channels.ch[i],
            .duty       = calculate_duty(speed_mode, 0),
            .gpio_num   = config->channels.servo_pin[i],
            .speed_mode = speed_mode,
            .timer_sel  = config->timer_number,
            .hpoint     = 0
        };
        ret = ledc_channel_config(&ledc_ch);
        SERVO_CHECK(ESP_OK == ret, "Falha na configuracao do canal LEDC", ESP_FAIL);
    }
    g_full_duty = (1 << SERVO_LEDC_INIT_BITS) - 1;
    g_cfg[speed_mode] = *config;

    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Desinicializa o LEDC para o servo
 *
 * @param speed_mode Seleciona o grupo de canais LEDC com modo de velocidade especificado.
 *
 * @return
 *     - ESP_OK Sucesso
 */
esp_err_t iot_servo_deinit(ledc_mode_t speed_mode) {
    SERVO_CHECK(speed_mode < LEDC_SPEED_MODE_MAX, "Modo de velocidade LEDC invalido", ESP_ERR_INVALID_ARG);
    for (size_t i = 0; i < g_cfg[speed_mode].channel_number; i++) {
        ledc_stop(speed_mode, g_cfg[speed_mode].channels.ch[i], 0);
    }
    ledc_timer_rst(speed_mode, g_cfg[speed_mode].timer_number);
    g_full_duty = 0;
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Define o servo motor para um determinado ângulo
 *
 * @note Esta API não é thread-safe
 *
 * @param speed_mode Seleciona o grupo de canais LEDC com modo de velocidade especificado.
 * @param channel Canal LEDC, selecionado de ledc_channel_t
 * @param angle Ângulo desejado
 *
 * @return
 *     - ESP_OK Sucesso
 *     - ESP_ERR_INVALID_ARG Erro de parâmetro
 */
esp_err_t iot_servo_write_angle(ledc_mode_t speed_mode, uint8_t channel, float angle) {
    SERVO_CHECK(speed_mode < LEDC_SPEED_MODE_MAX, "Modo de velocidade LEDC invalido", ESP_ERR_INVALID_ARG);
    SERVO_CHECK(channel < LEDC_CHANNEL_MAX, "Numero do canal LEDC muito alto", ESP_ERR_INVALID_ARG);
    SERVO_CHECK(angle >= 0.0f, "Angulo nao pode ser negativo", ESP_ERR_INVALID_ARG);
    esp_err_t ret;
    uint32_t duty = calculate_duty(speed_mode, angle);
    ret = ledc_set_duty(speed_mode, (ledc_channel_t)channel, duty);
    ret |= ledc_update_duty(speed_mode, (ledc_channel_t)channel);
    SERVO_CHECK(ESP_OK == ret, "Falha ao escrever angulo do servo", ESP_FAIL);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Lê o ângulo atual de um canal
 *
 * @param speed_mode Seleciona o grupo de canais LEDC com modo de velocidade especificado.
 * @param channel Canal LEDC, selecionado de ledc_channel_t
 * @param angle Ângulo atual do canal (retornado por referência)
 *
 * @return
 *     - ESP_OK Sucesso
 *     - ESP_ERR_INVALID_ARG Erro de parâmetro
 */
esp_err_t iot_servo_read_angle(ledc_mode_t speed_mode, uint8_t channel, float *angle) {
    SERVO_CHECK(speed_mode < LEDC_SPEED_MODE_MAX, "Modo de velocidade LEDC invalido", ESP_ERR_INVALID_ARG);
    SERVO_CHECK(channel < LEDC_CHANNEL_MAX, "Numero do canal LEDC muito alto", ESP_ERR_INVALID_ARG);
    uint32_t duty = ledc_get_duty(speed_mode, channel);
    float a = calculate_angle(speed_mode, duty);
    *angle = a;
    return ESP_OK;
}