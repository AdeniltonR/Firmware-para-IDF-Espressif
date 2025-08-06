/*
 * NOME: Adenilton Ribeiro
 * DATA: 05/08/2025
 * PROJETO: Exemplo ESP32-C3 - FreeRTOS
 * VERSAO: 1.0.0
 * DESCRIÇÃO: - feat: Tarefa 1: Pisca LED no GPIO 8, Tarefa 2: Log de informações no console
 *            - docs: ESP32C3 - ESP-IDF v5.4.0
 * LINKS: - https://github.com/esp-rs
 */

 // ========================================================================================================
//---BIBLIOTECAS---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define PIN_led        GPIO_NUM_10

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";

#define TAG_LED          "led_task"
#define TAG_LOG          "log_task"

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void tarefa_pisca_led(void *pvParameter);
void tarefa_log(void *pvParameter);

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void) {
    //---configura pino do LED como saída---
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_led),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "✅ Led Configurado: %d", PIN_led);

    //---cria as tarefas---
    xTaskCreate(tarefa_pisca_led, "tarefa_pisca_led", 2048, NULL, 5, NULL);
    xTaskCreate(tarefa_log,      "tarefa_log",      2048, NULL, 5, NULL);
}

// ========================================================================================================
/**
 * @brief Tarefa responsável por piscar o LED
 *
 * Alterna o estado do pino definido em PIN_led a cada 500ms,
 * enviando mensagens de log informando o estado atual do LED.
 *
 * @param pvParameter Parâmetro opcional (não utilizado).
 */
void tarefa_pisca_led(void *pvParameter) {
    bool estado = false;
    while (1) {
        estado = !estado;
        gpio_set_level(PIN_led, estado);
        ESP_LOGI(TAG_LED, "LED %s", estado ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(500)); // 500 ms
    }
}

// ========================================================================================================
/**
 * @brief Tarefa responsável por gerar logs periódicos
 *
 * Envia mensagens no console a cada 2 segundos, contendo
 * um contador incremental para monitoramento.
 *
 * @param pvParameter Parâmetro opcional (não utilizado).
 */
void tarefa_log(void *pvParameter) {
    int contador = 0;
    while (1) {
        ESP_LOGI(TAG_LOG, "Contador: %d", contador++);
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 segundos
    }
}
