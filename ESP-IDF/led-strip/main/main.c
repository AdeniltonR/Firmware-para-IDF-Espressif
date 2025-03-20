#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"

// Defina os pinos GPIO para as duas tiras de LEDs
#define LED_GPIO_1 19
#define LED_GPIO_2 23
#define LED_NUM 6  // Número de LEDs em cada tira

static const char *TAG = "example";

// Declare duas instâncias do led_strip_t
static led_strip_t *strip_1;
static led_strip_t *strip_2;

// Função para inicializar as tiras de LEDs
static void set_rgb_leds(void) {
    // Inicializa a primeira tira de LEDs
    strip_1 = led_strip_init(0, LED_GPIO_1, LED_NUM);
    strip_1->clear(strip_1, 100);  // Limpa a tira de LEDs

    // Inicializa a segunda tira de LEDs
    strip_2 = led_strip_init(1, LED_GPIO_2, LED_NUM);
    strip_2->clear(strip_2, 100);  // Limpa a tira de LEDs
}

void app_main(void) {
    // Inicializa as tiras de LEDs
    set_rgb_leds();

    while (true) {
        // Controla a primeira tira de LEDs
        ESP_LOGI(TAG, "LED RED - Strip 1");
        strip_1->set_pixel(strip_1, 0, 255, 0, 0);  // LED 0: Vermelho
        strip_1->set_pixel(strip_1, 1, 255, 0, 0);  // LED 0: Vermelho
        strip_1->set_pixel(strip_1, 2, 0, 255, 0);  // LED 1: Verde
        strip_1->set_pixel(strip_1, 3, 0, 0, 255);  // LED 2: Azul
        strip_1->set_pixel(strip_1, 4, 0, 0, 255);  // LED 2: Azul
        strip_1->refresh(strip_1, 100);  // Atualiza a tira de LEDs

        // Controla a segunda tira de LEDs
        ESP_LOGI(TAG, "LED RED - Strip 2");
        strip_2->set_pixel(strip_2, 0, 255, 0, 0);  // LED 0: Vermelho
        strip_2->set_pixel(strip_2, 1, 255, 0, 0);  // LED 0: Vermelho
        strip_2->set_pixel(strip_2, 2, 0, 255, 0);  // LED 1: Verde
        strip_2->set_pixel(strip_2, 3, 0, 0, 255);  // LED 2: Azul
        strip_2->set_pixel(strip_2, 4, 0, 0, 255);  // LED 2: Azul
        strip_2->refresh(strip_2, 100);  // Atualiza a tira de LEDs

        // Aguarda 1 segundo
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Controla a primeira tira de LEDs
        ESP_LOGI(TAG, "LED RED - Strip 1");
        strip_1->set_pixel(strip_1, 4, 0, 255, 0);  // LED 0: Vermelho
        strip_1->set_pixel(strip_1, 3, 0, 255, 0);  // LED 0: Vermelho
        strip_1->set_pixel(strip_1, 2, 0, 0, 255);  // LED 1: Verde
        strip_1->set_pixel(strip_1, 1, 255, 0, 0);  // LED 2: Azul
        strip_1->set_pixel(strip_1, 0, 255, 0, 0);  // LED 2: Azul
        strip_1->refresh(strip_1, 100);  // Atualiza a tira de LEDs

        // Controla a segunda tira de LEDs
        ESP_LOGI(TAG, "LED RED - Strip 2");
        strip_2->set_pixel(strip_2, 4, 0, 255, 0);  // LED 0: Vermelho
        strip_2->set_pixel(strip_2, 3, 0, 255, 0);  // LED 0: Vermelho
        strip_2->set_pixel(strip_2, 2, 0, 0, 255);  // LED 1: Verde
        strip_2->set_pixel(strip_2, 1, 255, 0, 0);  // LED 2: Azul
        strip_2->set_pixel(strip_2, 0, 255, 0, 0);  // LED 2: Azul
        strip_2->refresh(strip_2, 100);  // Atualiza a tira de LEDs

        // Aguarda 1 segundo
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}