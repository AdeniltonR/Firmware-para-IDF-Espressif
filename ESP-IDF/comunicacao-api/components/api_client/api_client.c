/*
* NOME: Adenilton Ribeiro
* DATA: 17/04/2025
* PROJETO: Comunicação API REST
* VERSAO: 1.0.0
* DESCRICAO: - feat: Cliente HTTP para envio de dados de velocidade à API remota
*                    Implementação de JSON parser para estruturação de dados
*                    Controle de conexão WiFi com reconexão automática
*                    Timeout configurável para requisições HTTP
*                    Sistema de logs detalhado com emojis visuais
*                    Tratamento robusto de erros de comunicação
*            - docs: ESP32 32D - ESP-IDF v5.4.0
* LINKS:
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include "api_client.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (api-client)
static const char *TAG = "api-client";  
//---buffer para armazenar URL base---
static char api_url[128] = {0};         

// ========================================================================================================
/**
 * @brief Inicializa o cliente API com URL base
 * @param url_base URL base da API (ex: "http://servidor:porta")
 * @return ESP_OK se sucesso, erro caso contrário
 * 
 * @note Armazena a URL internamente para uso futuro
 * @warning Verifica tamanho máximo da URL (128 bytes)
 */
esp_err_t api_client_init(const char *url_base) {
    if (strlen(url_base) >= sizeof(api_url)) {
        ESP_LOGE(TAG, "❌ URL muito longa (max %d chars)", sizeof(api_url)-1);
        return ESP_ERR_INVALID_ARG;
    }
    strcpy(api_url, url_base);
    ESP_LOGI(TAG, "✅ API inicializada | URL: %s", api_url);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Envia dados do dispositivo para API
 * @param data Ponteiro para estrutura com dados do dispositivo
 * @return ESP_OK se sucesso, erro caso contrário
 * 
 * @note Converte dados para JSON antes do envio
 * @warning Requer api_client_init() prévio
 */
esp_err_t api_send_device_data(const device_data_t *data) {
    //---verificação de inicialização---
    if (strlen(api_url) == 0) {
        ESP_LOGE(TAG, "❌ API não inicializada | Chame api_client_init() primeiro");
        return ESP_FAIL;
    }

    //---criação do JSON---
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "❌ Falha ao criar JSON");
        return ESP_FAIL;
    }

    //---adiciona campos ao JSON---
    cJSON_AddStringToObject(json, "id", data->id);
    cJSON_AddStringToObject(json, "status", data->status);
    cJSON_AddNumberToObject(json, "velocidade", data->velocidade);
    
    //---converte JSON para string---
    char *post_data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (!post_data) {
        ESP_LOGE(TAG, "❌ Falha ao serializar JSON");
        return ESP_FAIL;
    }

    //---monta URL completa---
    char full_url[160];
    snprintf(full_url, sizeof(full_url), "%s/api/device", api_url);

    //---configura cliente HTTP---
    esp_http_client_config_t config = {
        .url = full_url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,  // Timeout de 5s
    };

    //---executa requisição---
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    ESP_LOGI(TAG, "📤 Enviando dados para %s", full_url);
    esp_err_t err = esp_http_client_perform(client);

    //---trata resposta---
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            ESP_LOGI(TAG, "✅ Dados enviados | Status: %d", status_code);
        } else {
            ESP_LOGW(TAG, "⚠️ Resposta inesperada | Status: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "❌ Falha na requisição: %s", esp_err_to_name(err));
    }

    //---limpeza---
    esp_http_client_cleanup(client);
    free(post_data);
    return err;
}