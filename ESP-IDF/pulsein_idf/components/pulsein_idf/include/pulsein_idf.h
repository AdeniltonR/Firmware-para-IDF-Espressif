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
#include <string.h>
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

#define MAX_CHANNELS 4  // Número máximo de pinos suportados simultaneamente

// ========================================================================================================
//---CONSTANTS---

typedef struct {
    rmt_channel_handle_t channel;  // Handle do canal RMT
    gpio_num_t gpio;               // GPIO utilizado
    uint32_t resolution_hz;        // Resolução do canal
    bool active;                   // Status do canal
} rmt_pulsein_channel_t;

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

esp_err_t rmt_pulsein_init(gpio_num_t gpio, int index, uint32_t resolution_hz);
esp_err_t rmt_pulsein_deinit(int index);
int64_t rmt_pulsein_read_us(int index, uint32_t timeout_us);

#endif //pulsein_idf.h