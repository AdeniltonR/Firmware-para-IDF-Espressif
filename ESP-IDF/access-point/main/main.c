#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "access_point.h"

static const char *TAG = "wifi softAP";

void app_main(void)
{
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Configurar o pino do LED como saída
    gpio_reset_pin(LED_GPIO_PIN);
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();  // Inicializa o Wi-Fi no modo Access Point
}