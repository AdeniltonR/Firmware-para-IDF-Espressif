/*
 * NOME: Adenilton
 * DATA: 16/03/2025
 * PROJETO: Wi-Fi
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Conexão de wifi manual com teste de internete com horário.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include "wifi.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

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
    //---inicializa o NVS (Non-Volatile Storage)---
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    //---inicializa Wi-Fi---
    wifi_init_sta();
    
    //---configura o fuso horário---
    initialize_hora();
    //---testa a conexão com a internet e obtém a hora---
    test_ntp_connection();

    while (1) {
        vTaskDelay(5000 / portTICK_PERIOD_MS); 
    }
}