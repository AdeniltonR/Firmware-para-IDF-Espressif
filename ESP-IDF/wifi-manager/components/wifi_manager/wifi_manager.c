/*
 * NOME: Adenilton Ribeiro
 * DATA: 17/03/2025
 * PROJETO: Wi-Fi Manager
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para Wi-Fi Manager e conexão de internet.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include "wifi_manager.h" 

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

//---tag para identificação nos logs---
static const char *TAG_MG = "wifi_manager";
//---variáveis globais para gerenciar o timeout---
static TaskHandle_t timeout_task_handle = NULL;  // Handle da tarefa de timeout
//---flag para controlar o timeout---
static bool timeout_active = false;      

// ========================================================================================================
/**
 * @brief Inicialização da Manager.
 * 
 * @note Chama todoas a funções para inicializar
 * 
 */
void init_manager(void) {
    //---inicializar NVS---
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //---mostra os dados salvos na NVS---
    show_saved_wifi_credentials();
    //---exibe as credenciais Wi-Fi salvas---
    wifi_manager_show_saved_credentials();

    //---verifica o modo Wi-Fi salvo na NVS---
    wifi_mode_t mode = wifi_manager_get_mode();

    //---inicia no modo correspondente---
    if (mode == WIFI_MODE_AP) {
        ESP_LOGI(TAG_MG, "Iniciando no modo Access Point...");
        //---iniciando AP---
        wifi_init_softap();
    } else if (mode == WIFI_MODE_STA) {
        ESP_LOGI(TAG_MG, "Iniciando no modo Station...");
        //---cancela o timeout---
        wifi_manager_cancel_timeout();
        //---iniciando STA---
        wifi_init_sta();
        //---configura o fuso horário---
        initialize_hora();
        //---testa a conexão com a internet e obtém a hora---
        test_ntp_connection();
    } else {
        ESP_LOGI(TAG_MG, "Modo Wi-Fi desconhecido. Iniciando no modo padrão (AP).");
        //---iniciando AP---
        wifi_init_softap();
    }
}

// ========================================================================================================
/**
 * @brief Inicializa o gerenciador de Wi-Fi.
 * 
 * @return esp_err_t Retorna ESP_OK se a inicialização for bem-sucedida, ou um erro em caso de falha.
 * 
 * @note Esta função inicializa o NVS (Non-Volatile Storage), que é usado para armazenar as configurações do Wi-Fi.
 *       Se a NVS estiver corrompida ou não inicializada, ela será apagada e reinicializada.
 */
esp_err_t wifi_manager_init(void) {
    //---inicializa o NVS---
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);  // Verifica se a inicialização foi bem-sucedida
    return ret;
}

// ========================================================================================================
/**
 * @brief Salva o modo Wi-Fi na NVS.
 * 
 * @param mode Modo Wi-Fi a ser salvo (WIFI_MODE_AP ou WIFI_MODE_STA).
 * @return esp_err_t Retorna ESP_OK se o modo for salvo com sucesso, ou um erro em caso de falha.
 * 
 * @note Esta função salva o modo Wi-Fi (AP ou STA) na NVS para que ele possa ser recuperado após reinicializações.
 */
esp_err_t wifi_manager_set_mode(wifi_mode_t mode) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    //---abre a partição NVS no modo de leitura e escrita---
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_MG, "Erro ao abrir a NVS: %s", esp_err_to_name(err));
        return err;
    }

    //---salva o modo na NVS---
    err = nvs_set_i32(nvs_handle, "wifi_mode", (int32_t)mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_MG, "Erro ao salvar o modo na NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    //---confirma as alterações na NVS---
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_MG, "Erro ao confirmar as alterações na NVS: %s", esp_err_to_name(err));
    }

    //---fecha a NVS---
    nvs_close(nvs_handle);

    ESP_LOGI(TAG_MG, "Modo Wi-Fi salvo na NVS: %u", mode);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Carrega o modo Wi-Fi salvo na NVS.
 * 
 * @return wifi_mode_t Retorna o modo Wi-Fi salvo (WIFI_MODE_AP ou WIFI_MODE_STA).
 * 
 * @note Se a chave 'wifi_mode' não for encontrada na NVS, o modo padrão (AP) será retornado.
 */
wifi_mode_t wifi_manager_get_mode(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    //---abre a partição NVS no modo de leitura---
    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_MG, "Erro ao abrir a NVS: %s", esp_err_to_name(err));
        return WIFI_MODE_AP; // Modo padrão (AP)
    }

    //---recupera o modo da NVS---
    int32_t mode = WIFI_MODE_AP; // Valor padrão
    err = nvs_get_i32(nvs_handle, "wifi_mode", &mode);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG_MG, "Chave 'wifi_mode' não encontrada na NVS. Usando valor padrão (AP).");
        mode = WIFI_MODE_AP;  // Define o valor padrão
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG_MG, "Erro ao recuperar o modo da NVS: %s", esp_err_to_name(err));
    }

    //---fecha a NVS---
    nvs_close(nvs_handle);

    //---usa PRId32 para formatar int32_t corretamente---
    ESP_LOGI(TAG_MG, "Modo Wi-Fi recuperado da NVS: %" PRId32, mode);
    
    return (wifi_mode_t)mode;
}

// ========================================================================================================
/**
 * @brief Exibe as credenciais Wi-Fi salvas na NVS.
 * 
 * @note Esta função exibe o modo Wi-Fi salvo na NVS (AP ou STA) para fins de depuração.
 */
void wifi_manager_show_saved_credentials(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    //---abre a partição NVS no modo de leitura---
    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_MG, "Erro ao abrir a NVS: %s", esp_err_to_name(err));
        return;
    }

    //---recupera o modo da NVS---
    int32_t mode = WIFI_MODE_AP; // Valor padrão
    err = nvs_get_i32(nvs_handle, "wifi_mode", &mode);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG_MG, "Chave 'wifi_mode' não encontrada na NVS. Usando valor padrão (AP).");
        mode = WIFI_MODE_AP;  // Define o valor padrão
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG_MG, "Erro ao recuperar o modo da NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG_MG, "Modo Wi-Fi salvo: %s", (mode == WIFI_MODE_AP) ? "AP" : "STA");
    }

    //---fecha a NVS---
    nvs_close(nvs_handle);
}

// ========================================================================================================
/**
 * @brief Tarefa de timeout que reinicia o ESP32 se o tempo limite for atingido.
 * 
 * @param pvParameter Tempo limite em segundos.
 */
/**
 * @brief Tarefa de timeout que reinicia o ESP32 se o tempo limite for atingido.
 * 
 * @param pvParameter Tempo limite em segundos.
 */
static void timeout_task(void *pvParameter) {
    int timeout_seconds = (int)pvParameter;

    ESP_LOGW(TAG_MG, "Iniciando timeout de %d segundos...", timeout_seconds);

    //---aguarda o tempo limite---
    for (int i = timeout_seconds; i > 0; i--) {
        if (!timeout_active) {
            ESP_LOGW(TAG_MG, "Timeout cancelado.");
            vTaskDelete(NULL);  // Encerra a tarefa
        }

        //---exibe uma mensagem a cada 20 segundos---
        if (i % 20 == 0) {
            ESP_LOGW(TAG_MG, "Tempo restante para reiniciar: %d segundos", i);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Aguarda 1 segundo
    }

    //---se o timeout expirar, reinicia o ESP32---
    if (timeout_active) {
        ESP_LOGW(TAG_MG, "Timeout atingido. Reiniciando o ESP32...");
        esp_restart();
    }
}

// ========================================================================================================
/**
 * @brief Inicia o timeout.
 * 
 * @param timeout_seconds Tempo limite em segundos.
 * 
 * @note Se o tempo limite for atingido, o ESP32 será reiniciado.
 */
void wifi_manager_start_timeout(int timeout_seconds) {
    if (timeout_task_handle != NULL) {
        ESP_LOGW(TAG_MG, "Timeout já está ativo.");
        return;
    }

    timeout_active = true;
    xTaskCreate(&timeout_task, "timeout_task", 4096, (void *)timeout_seconds, 5, &timeout_task_handle);
    ESP_LOGW(TAG_MG, "Timeout iniciado com sucesso.");
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
}

// ========================================================================================================
/**
 * @brief Cancela o timeout.
 * 
 * @note Se o timeout for cancelado, o ESP32 não será reiniciado.
 */
void wifi_manager_cancel_timeout(void) {
    if (timeout_task_handle == NULL) {
        ESP_LOGW(TAG_MG, "Nenhum timeout ativo para cancelar.");
        return;
    }

    timeout_active = false;
    //---encerra a tarefa de timeout---
    vTaskDelete(timeout_task_handle);  
    timeout_task_handle = NULL;
    ESP_LOGW(TAG_MG, "Timeout cancelado com sucesso.");
}

// ========================================================================================================
/**
 * @brief Reiniciar em modo AP.
 * 
 */
void reset_AP(void) {
    //---salva o modo Wi-Fi na NVS (por exemplo, modo AP)---
    wifi_manager_set_mode(WIFI_MODE_AP);
    ESP_LOGI(TAG_MG, "Modo AP iniciado e estado salvo na NVS.");

    vTaskDelay(1000 / portTICK_PERIOD_MS); 

    //---reinicia o ESP32---
    ESP_LOGW(TAG_MG, "Reiniciando o ESP32...");
    esp_restart();
}

// ========================================================================================================
/**
 * @brief Reiniciar em modo STA.
 * 
 */
void reset_STA(void) {
    //---salva o modo Wi-Fi na NVS (por exemplo, modo STA)---
    wifi_manager_set_mode(WIFI_MODE_STA);
    ESP_LOGI(TAG_MG, "Modo STA iniciado e estado salvo na NVS.");

    vTaskDelay(1000 / portTICK_PERIOD_MS); 

    //---reinicia o ESP32---
    ESP_LOGW(TAG_MG, "Reiniciando o ESP32...");
    esp_restart();
}
