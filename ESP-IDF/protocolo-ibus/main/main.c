/*
 * NOME: Adenilton Ribeiro
 * DATA: 31/01/2025
 * PROJETO: i-BUS
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Envio de dados de múltiplos sensores via protocolo i-BUS (FlySky) usando UART.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "ibus.h"

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define PIN_ibus 6  // GPIO1 → porta SENS do receptor FlySky

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

static void ibus_task(void *pvParameters);

// ========================================================================================================
/**
 * @brief Void main
 *
*/
void app_main(void) {
    ibus_init(UART_NUM_1, PIN_ibus);  // GPIO1 → porta SENS do receptor FlySky

    // Adiciona sensor de tensão com tipo EXT_V (tensão externa) e ID 0
    ibus_add_sensor(IBUS_MEAS_TYPE_EXTV, 2);

    ESP_LOGI(TAG, "✅ Inicializando task iBUS...");

    xTaskCreatePinnedToCore(ibus_task, "ibus_task", 4096, NULL, 5, NULL, 1);
}

// ========================================================================================================
/**
 * @brief Task responsável por enviar dados simulados via i-BUS para o receptor.
 *
 * @note Os valores são atualizados a cada 100 ms e enviados pelo barramento UART.
 * 
 * @param pvParameters Não utilizado (NULL)
*/
static void ibus_task(void *pvParameters) {

    while (1) {
        uint16_t tensao_mV = 1180;  // 11.80 V → representado como 1180 (décimos)

        // Define o valor da tensão
        ibus_set_sensor_value(2, tensao_mV);

        ESP_LOGI(TAG, "⚡ Enviando tensão: %d mV", tensao_mV);

        // Envia para o receptor via i-BUS
        ibus_send_all();

        vTaskDelay(pdMS_TO_TICKS(500));  // A cada 1 segundo
    }
}