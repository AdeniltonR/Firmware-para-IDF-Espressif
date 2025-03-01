/*
 * NOME: Nome
 * DATA: 31/01/2025
 * PROJETO:  RGB_WS2812B.h
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Descrição.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0 e Simulador PICSimLab 0.9.1
 * LINKS: 
*/

// ========================================================================================================
/**
 * @brief  RGB_WS2812B.h
 * 
 */
#ifndef  RGB_WS2812B_H
#define   RGB_WS2812B_H

// ========================================================================================================
// ---BIBLIOTECA---

#include <stdint.h>
#include "driver/rmt_tx.h"

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

// ========================================================================================================
//---MACROS---

// ========================================================================================================
//---CONSTANTS---

// ========================================================================================================
/**
 * @brief Estrutura para armazenar a configuração da fita de LEDs WS2812B.
 *
 * @note Essa estrutura contém as configurações do pino GPIO, número de LEDs e
 * os manipuladores necessários para a comunicação via RMT.
 */
typedef struct {
    uint8_t gpio_pin;              // Pino GPIO
    uint16_t num_leds;             // Número de LEDs
    rmt_channel_handle_t channel;  // Canal RMT
    rmt_encoder_handle_t encoder;  // Codificador RMT
    rmt_symbol_word_t *led_data;   // Buffer de dados dos LEDs
} ws2812_t;

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void ws2812_init(ws2812_t *strip, uint8_t gpio_pin, uint16_t num_leds);
void ws2812_clear(ws2812_t *strip);
void ws2812_set_color(ws2812_t *strip, uint16_t led_num, uint8_t red, uint8_t green, uint8_t blue);
void ws2812_update(ws2812_t *strip);

#endif //RGB_WS2812B