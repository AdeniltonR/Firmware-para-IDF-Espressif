/*
 * NOME: Adenilton Ribeiro
 * DATA: 25/04/2025
 * PROJETO: Pulsein IDF
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca para leitura de pulso digital utilizando o periférico RMT com suporte a múltiplos pinos.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/rmt.html
 */

// ========================================================================================================
/**
 * @brief pulsein_idf.h
 * 
 */
#ifndef  PULSEIN_IDF_H
#define  PULSEIN_IDF_H

// ========================================================================================================
// ---BIBLIOTECA---

#include <stdint.h>
#include "driver/rmt_rx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

#define MAX_CHANNELS 4  // Número máximo de pinos suportados simultaneamente

// ========================================================================================================
//---CONSTANTS---

typedef struct {
    rmt_channel_handle_t channel;  // Conexão com o hardware RMT
    gpio_num_t gpio;               // Pino físico usado (ex: GPIO4)
    uint32_t resolution_hz;        // Precisão da medição (ex: 1.000.000 = 1μs)
    bool active;                   // Se o canal está pronto para uso
    bool receiving;                // Se está no meio de uma leitura
    uint8_t missing_pulses;        // Quantos pulsos seguintes falharam
    rmt_symbol_word_t symbols[64]; // Onde ficam armazenados os pulsos medidos
} rmt_pulsein_channel_t;

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

esp_err_t rmt_pulsein_init(gpio_num_t gpio, int index, uint32_t resolution_hz);
esp_err_t rmt_pulsein_deinit(int index);
int64_t rmt_pulsein_read_us(int index, uint32_t timeout_us);

#endif //pulsein_idf.h