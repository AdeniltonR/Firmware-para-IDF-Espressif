/*
 * NOME: Adenilton Ribeiro
 * DATA: 31/01/2025
 * PROJETO: i-BUS
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Envio de dados de múltiplos sensores via protocolo i-BUS (FlySky) usando UART.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: 
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "ibus.h"

static const char *TAG = "main";

// Definições de hardware
#define IBUS_UART_NUM    UART_NUM_1
#define IBUS_TX_PIN      16
#define IBUS_RX_PIN      15
#define LED_GPIO         2

// Constantes de telemetria
#define TEMP_BASE        400  // Base para 0°C (400 = -40°C)
#define TEMP_OFFSET      237  // 23.7°C

// Variáveis globais
static ibus_handle_t ibus;
static uint16_t rpm = 1000;
static uint16_t temp = TEMP_BASE + TEMP_OFFSET;
static uint32_t last_poll_count = 0;

void app_main() {
    // Configura LED
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    
    // Inicializa iBUS
    esp_err_t ret = ibus_init(&ibus, IBUS_UART_NUM, IBUS_TX_PIN, IBUS_RX_PIN, 115200);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar iBUS: %s", esp_err_to_name(ret));
        return;
    }
    
    // Adiciona sensores
    if (!ibus_add_sensor(&ibus, IBUSS_RPM, 2) || !ibus_add_sensor(&ibus, IBUSS_TEMP, 2)) {
        ESP_LOGE(TAG, "Falha ao adicionar sensores");
        return;
    }
    
    ESP_LOGI(TAG, "Telemetria iBUS inicializada");
    
    while (1) {
        // Pisca LED a cada 1s
        static uint32_t last_blink = 0;
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        if (now - last_blink > 1000) {
            last_blink = now;
            gpio_set_level(LED_GPIO, !gpio_get_level(LED_GPIO));
            
            // Loga estado atual
            ESP_LOGI(TAG, "RPM: %" PRIu16 ", Temp: %.1f°C", rpm, (temp - TEMP_BASE) / 10.0f);
        }
        
        // Atualiza valores dos sensores
        ibus_set_sensor_value(&ibus, 1, rpm);
        ibus_set_sensor_value(&ibus, 2, temp);
        
        // Detecta polls do receptor
        if (ibus.poll_count != last_poll_count) {
            last_poll_count = ibus.poll_count;
            ESP_LOGI(TAG, "Poll recebido do receptor (total: %" PRIu32 ")", ibus.poll_count);
        }
        
        // Processa comunicação iBUS
        ibus_loop(&ibus);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}