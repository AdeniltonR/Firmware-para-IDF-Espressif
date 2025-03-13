#include "scan_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializa o loop de eventos do sistema (apenas uma vez)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Cria a interface de rede STA (apenas uma vez)
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    while (1) { // Loop infinito
        wifi_scan(); // Executa a varredura Wi-Fi

        // Aguarda 5 segundos antes de executar novamente
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}