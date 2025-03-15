#include "wifi.h"

// Função para inicializar o SNTP
void initialize_sntp(void) {
    ESP_LOGI(TAG, "Inicializando SNTP...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);  // Função atualizada
    esp_sntp_setservername(0, "pool.ntp.org");   // Função atualizada
    esp_sntp_init();                             // Função atualizada
}

// Função para configurar o fuso horário
void set_timezone(void) {
    setenv("TZ", "BRT3BRST,M10.3.0/0,M2.3.0/0", 1);  // Configura o fuso horário para Brasília
    tzset();
}

// Função para obter a hora atual da internet
char* get_current_time(void) {
    time_t now;
    struct tm timeinfo;
    char *time_str = (char *)malloc(64 * sizeof(char));  // Aloca memória para a string de tempo

    // Obtém o tempo atual
    time(&now);
    localtime_r(&now, &timeinfo);

    // Verifica se o tempo foi sincronizado
    if (timeinfo.tm_year < (2024 - 1900)) {
        ESP_LOGE(TAG, "Tempo não sincronizado. Verifique a conexão com a internet.");
        strcpy(time_str, "Tempo não disponível");
        return time_str;
    }

    // Formata o tempo como uma string
    strftime(time_str, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);
    return time_str;
}

// Função para testar a conexão e obter a hora
void test_ntp_connection(void) {
    // Aguarda a sincronização do tempo (pode levar alguns segundos)
    ESP_LOGI(TAG, "Aguardando sincronização do tempo...");
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Aguardando... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);  // Aguarda 2 segundos
    }

    // Obtém a hora atual
    char *current_time = get_current_time();
    ESP_LOGI(TAG, "Hora atual: %s", current_time);

    // Libera a memória alocada para a string de tempo
    free(current_time);
}

void app_main(void)
{
    // Inicializa o NVS (Non-Volatile Storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    // Configura o fuso horário
    set_timezone();

    // Inicializa o SNTP (apenas uma vez)
    initialize_sntp();

    // Loop principal para outras tarefas
    while (1) {
        // Testa a conexão com a internet e obtém a hora
        test_ntp_connection();

        vTaskDelay(5000 / portTICK_PERIOD_MS); 
    }
}