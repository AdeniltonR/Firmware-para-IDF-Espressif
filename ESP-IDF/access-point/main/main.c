/*
 * NOME: Adenilton Ribeiro
 * DATA: 13/03/2025
 * PROJETO: access point
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para criar um access point e abrir uma página HTML para controle de um led.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
//---BIBLIOTECAS AUXILIARES---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "access_point.h"


// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void)
{
    //---inicializar NVS---
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //---configurar o pino do LED como saída---
    gpio_reset_pin(LED_GPIO_PIN);
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "📶 ESP_WIFI_MODE_AP");

    //---inicializa o Wi-Fi no modo Access Point---
    wifi_init_softap();  
}