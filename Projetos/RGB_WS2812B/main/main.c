/*
 * NOME: Nome
 * DATA: 31/01/2025
 * PROJETO: Nome do projeto
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Descrição.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0 e Simulador PICSimLab 0.9.1
 * LINKS: 
*/

// ========================================================================================================
//---BIBLIOTECAS AUXILIARES---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "RGB_WS2812B.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define PIN_rgb 18
#define LEDs    3

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void) {
    //---configuração da fita de LEDs---
    ws2812_t strip;
    ws2812_init(&strip, PIN_rgb, LEDs);  // GPIO 18, 30 LEDs

    while(1) {
        //---exemplo: Acender todos os LEDs de vermelho---
        for (int i = 0; i < strip.num_leds; i++) {
            ws2812_set_color(&strip, i, 255, 0, 0);
        }
        ws2812_update(&strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ws2812_clear(&strip);

        //---exemplo: Acender todos os LEDs de verde---
        for (int i = 0; i < strip.num_leds; i++) {
            ws2812_set_color(&strip, i, 0, 255, 0);
        }
        ws2812_update(&strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ws2812_clear(&strip);

        //---exemplo: Acender todos os LEDs de azul---
        for (int i = 0; i < strip.num_leds; i++) {
            ws2812_set_color(&strip, i, 0, 0, 255);
        }
        ws2812_update(&strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        //---limpar todos os LEDs---
        ws2812_clear(&strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}