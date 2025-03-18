/*
* NOME: Adenilton
* DATA: 18/03/2025
* PROJETO: RGB WS2812B
* VERSAO: 1.0.0
* DESCRICAO: - feat: Controle de LEDs WS2812B via SPI.
*            - docs: ESP32-S3 - ESP-IDF v5.4.0
* LINKS: Link de referencia - https://github.com/okhsunrog/esp_ws28xx/tree/main
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ws2812b.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define LED_GPIO 48

#define LED_NUM 1

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

static const char *TAG = "example";
CRGB* ws2812_buffer;

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void set_led_color(CRGB color);

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(ws28xx_init(LED_GPIO, WS2812B, LED_NUM, &ws2812_buffer));
    
    CRGB colors[] = {
        (CRGB){.r=255, .g=0, .b=0},     // Vermelho
        (CRGB){.r=0, .g=255, .b=0},     // Verde
        (CRGB){.r=0, .g=0, .b=255},     // Azul
        (CRGB){.r=255, .g=255, .b=0},   // Amarelo
        (CRGB){.r=255, .g=0, .b=255}    // Roxo
    };
    
    int color_count = sizeof(colors) / sizeof(colors[0]);
    int current_color = 0;

    while (1) {
        ESP_LOGI(TAG, "Setting LED color to %d", current_color);
        set_led_color(colors[current_color]);
        current_color = (current_color + 1) % color_count;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// ========================================================================================================
/**
 * @brief Define a cor de todos os LEDs na tira.
 * 
 * @param color A cor a ser aplicada aos LEDs, representada por uma estrutura CRGB.
 * 
 */
void set_led_color(CRGB color) {
    //---percorre todos os LEDs no buffer---
    for(int i = 0; i < LED_NUM; i++) {
        //---define a cor do LED atual no buffer---
        ws2812_buffer[i] = color;
    }

    //----atualiza a tira de LEDs com as novas cores---
    ESP_ERROR_CHECK_WITHOUT_ABORT(ws28xx_update());
}