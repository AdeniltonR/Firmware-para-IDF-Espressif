#include "wifi_manager.h"
#include <inttypes.h> 

static const char *TAG = "wifi_manager";

// Função para inicializar o gerenciador de Wi-Fi
esp_err_t wifi_manager_init(void) {
    // Inicializa o NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

// Função para salvar o modo Wi-Fi na NVS
esp_err_t wifi_manager_set_mode(wifi_mode_t mode) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Abre a partição NVS no modo de leitura e escrita
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao abrir a NVS: %s", esp_err_to_name(err));
        return err;
    }

    // Salva o modo na NVS
    err = nvs_set_i32(nvs_handle, "wifi_mode", (int32_t)mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao salvar o modo na NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Confirma as alterações na NVS
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao confirmar as alterações na NVS: %s", esp_err_to_name(err));
    }

    // Fecha a NVS
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "Modo Wi-Fi salvo na NVS: %" PRId32, mode);
    return ESP_OK;
}

// Função para carregar o modo Wi-Fi da NVS
wifi_mode_t wifi_manager_get_mode(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Abre a partição NVS no modo de leitura
    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao abrir a NVS: %s", esp_err_to_name(err));
        return WIFI_MODE_AP; // Modo padrão (AP)
    }

    // Recupera o modo da NVS
    int32_t mode = WIFI_MODE_AP; // Valor padrão
    err = nvs_get_i32(nvs_handle, "wifi_mode", &mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao recuperar o modo da NVS: %s", esp_err_to_name(err));
    }

    // Fecha a NVS
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "Modo Wi-Fi recuperado da NVS: %" PRId32, mode);
    return (wifi_mode_t)mode;
}

// Função para exibir as credenciais Wi-Fi salvas na NVS
void wifi_manager_show_saved_credentials(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Abre a partição NVS no modo de leitura
    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao abrir a NVS: %s", esp_err_to_name(err));
        return;
    }

    // Recupera o modo da NVS
    int32_t mode = WIFI_MODE_AP; // Valor padrão
    err = nvs_get_i32(nvs_handle, "wifi_mode", &mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao recuperar o modo da NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Modo Wi-Fi salvo: %s", (mode == WIFI_MODE_AP) ? "AP" : "STA");
    }

    // Fecha a NVS
    nvs_close(nvs_handle);
}