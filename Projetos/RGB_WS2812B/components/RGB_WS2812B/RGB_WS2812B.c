/*
 * NOME: Nome
 * DATA: 31/01/2025
 * PROJETO: biblioteca.c
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Descrição.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0 e Simulador PICSimLab 0.9.1
 * LINKS: 
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include "RGB_WS2812B.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

static const char *TAG = "WS2812B";

// ========================================================================================================
/**
 * @brief Inicializa a fita de LEDs WS2812B.
 *
 * @note Esta função configura o canal RMT para a comunicação com a fita de LEDs e 
 * aloca memória para armazenar os dados de cores.
 *
 * @param strip Ponteiro para a estrutura de controle do WS2812B.
 * @param gpio_pin Pino GPIO usado para controle da fita de LEDs.
 * @param num_leds Número de LEDs na fita.
 */
void ws2812_init(ws2812_t *strip, uint8_t gpio_pin, uint16_t num_leds) {
    strip->gpio_pin = gpio_pin;
    strip->num_leds = num_leds;

    //---configura o canal RMT---
    rmt_tx_channel_config_t tx_chan_config = {
        .gpio_num = gpio_pin,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,  // 10 MHz
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &strip->channel));

    //---configura o codificador RMT---
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 4,  // Duração do bit 0
            .level1 = 0,
            .duration1 = 8,  // Duração do bit 1
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 8,  // Duração do bit 1
            .level1 = 0,
            .duration1 = 4,  // Duração do bit 0
        },
    };
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, &strip->encoder));

    //---aloca memória para os dados dos LEDs---
    strip->led_data = (rmt_symbol_word_t *)malloc(num_leds * 24 * sizeof(rmt_symbol_word_t));
    if (!strip->led_data) {
        ESP_LOGE(TAG, "Falha ao alocar memória para os dados dos LEDs");
        return;
    }

    //---inicializa o canal RMT---
    ESP_ERROR_CHECK(rmt_enable(strip->channel));
}

// ========================================================================================================
/**
 * @brief Limpa todos os LEDs da fita, definindo todas as cores como preto (desligado).
 *
 * @param strip Ponteiro para a estrutura de controle do WS2812B.
 */
void ws2812_clear(ws2812_t *strip) {
    for (uint16_t i = 0; i < strip->num_leds; i++) {
        ws2812_set_color(strip, i, 0, 0, 0);
    }
    ws2812_update(strip);
}

// ========================================================================================================
/**
 * @brief Define a cor de um LED específico na fita.
 *
 * @param strip Ponteiro para a estrutura de controle do WS2812B.
 * @param led_num Índice do LED na fita (começa em 0).
 * @param red Intensidade do vermelho (0-255).
 * @param green Intensidade do verde (0-255).
 * @param blue Intensidade do azul (0-255).
 */
void ws2812_set_color(ws2812_t *strip, uint16_t led_num, uint8_t red, uint8_t green, uint8_t blue) {
    if (led_num >= strip->num_leds) {
        ESP_LOGE(TAG, "Número do LED fora do intervalo");
        return;
    }

    uint32_t color = (green << 16) | (red << 8) | blue;

    for (int i = 0; i < 24; i++) {
        strip->led_data[led_num * 24 + i] = ((color >> (23 - i)) & 1) ? 
            (rmt_symbol_word_t){.level0 = 1, .duration0 = 8, .level1 = 0, .duration1 = 4} : 
            (rmt_symbol_word_t){.level0 = 1, .duration0 = 4, .level1 = 0, .duration1 = 8};
    }
}

// ========================================================================================================
/**
 * @brief Atualiza a fita de LEDs com as cores definidas.
 *
 * @note Essa função transmite os dados armazenados para os LEDs via RMT.
 *
 * @param strip Ponteiro para a estrutura de controle do WS2812B.
 */
void ws2812_update(ws2812_t *strip) {
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,  // Sem repetição
    };

    //---envia os dados dos LEDs---
    esp_err_t ret = rmt_transmit(strip->channel, strip->encoder, strip->led_data, strip->num_leds * 24 * sizeof(rmt_symbol_word_t), &tx_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao transmitir dados RMT: %s", esp_err_to_name(ret));
        return;
    }

    //---espera a transmissão ser concluída---
    ret = rmt_tx_wait_all_done(strip->channel, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao esperar transmissão RMT: %s", esp_err_to_name(ret));
    }
}