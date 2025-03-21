/*
 * NOME: Adenilton Ribeiro
 * DATA: 20/03/2025
 * PROJETO: Led Strip
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Controle de dois leds RGB usando RMT, mas a biblioteca driver/rmt.h está obsoleta.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

#define LED_NUM 6  // Número de LEDs em cada tira

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define PIN_rgb_1 19
#define PIN_rgb_2 23

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

//---tag de logs---
static const char *TAG = "example";

//---declare duas instâncias do led_strip_t---
static led_strip_t *strip_1;
static led_strip_t *strip_2;

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void set_rgb_leds(void);

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void) {

    //---inicializa as tiras de LEDs---
    set_rgb_leds();

    while (true) {
        //---controla a primeira tira de LEDs---
        ESP_LOGI(TAG, "LED RED - Strip 1");
        strip_1->set_pixel(strip_1, 0, 255, 0, 0);  // LED 0: Vermelho
        strip_1->set_pixel(strip_1, 1, 255, 0, 0);  // LED 0: Vermelho
        strip_1->set_pixel(strip_1, 2, 0, 255, 0);  // LED 1: Verde
        strip_1->set_pixel(strip_1, 3, 0, 0, 255);  // LED 2: Azul
        strip_1->set_pixel(strip_1, 4, 0, 0, 255);  // LED 2: Azul
        strip_1->refresh(strip_1, 100);             // Atualiza a tira de LEDs

        //---controla a segunda tira de LEDs---
        ESP_LOGI(TAG, "LED RED - Strip 2");
        strip_2->set_pixel(strip_2, 0, 255, 0, 0);  // LED 0: Vermelho
        strip_2->set_pixel(strip_2, 1, 255, 0, 0);  // LED 0: Vermelho
        strip_2->set_pixel(strip_2, 2, 0, 255, 0);  // LED 1: Verde
        strip_2->set_pixel(strip_2, 3, 0, 0, 255);  // LED 2: Azul
        strip_2->set_pixel(strip_2, 4, 0, 0, 255);  // LED 2: Azul
        strip_2->refresh(strip_2, 100);             // Atualiza a tira de LEDs

        vTaskDelay(pdMS_TO_TICKS(1000));

        //---controla a primeira tira de LEDs---
        ESP_LOGI(TAG, "LED RED - Strip 1");
        strip_1->set_pixel(strip_1, 4, 0, 255, 0);  // LED 0: Vermelho
        strip_1->set_pixel(strip_1, 3, 0, 255, 0);  // LED 0: Vermelho
        strip_1->set_pixel(strip_1, 2, 0, 0, 255);  // LED 1: Verde
        strip_1->set_pixel(strip_1, 1, 255, 0, 0);  // LED 2: Azul
        strip_1->set_pixel(strip_1, 0, 255, 0, 0);  // LED 2: Azul
        strip_1->refresh(strip_1, 100);             // Atualiza a tira de LEDs

        //---controla a segunda tira de LEDs---
        ESP_LOGI(TAG, "LED RED - Strip 2");
        strip_2->set_pixel(strip_2, 4, 0, 255, 0);  // LED 0: Vermelho
        strip_2->set_pixel(strip_2, 3, 0, 255, 0);  // LED 0: Vermelho
        strip_2->set_pixel(strip_2, 2, 0, 0, 255);  // LED 1: Verde
        strip_2->set_pixel(strip_2, 1, 255, 0, 0);  // LED 2: Azul
        strip_2->set_pixel(strip_2, 0, 255, 0, 0);  // LED 2: Azul
        strip_2->refresh(strip_2, 100);             // Atualiza a tira de LEDs

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ========================================================================================================
/**
 * @brief Inicializa e configura as tiras de LEDs RGB.
 * 
 * @note Esta função inicializa duas tiras de LEDs, configurando os pinos GPIO e o número de LEDs.
 *       Após a inicialização, as tiras são limpas (todos os LEDs são desligados).
 */
void set_rgb_leds(void) {
    //---inicializa a primeira tira de LEDs---
    strip_1 = led_strip_init(0, PIN_rgb_1, LED_NUM);
    strip_1->clear(strip_1, 100);  // Limpa a tira de LEDs

    //---inicializa a segunda tira de LEDs---
    strip_2 = led_strip_init(1, PIN_rgb_2, LED_NUM);
    strip_2->clear(strip_2, 100);  // Limpa a tira de LEDs
}