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
    char full_url[256];

    //---receber quebrada---
    snprintf(full_url, sizeof(full_url), "%s/api/device", api_url);

    //---receber inteira---
    //strncpy(full_url, api_url, sizeof(full_url) - 1);
    //full_url[sizeof(full_url) - 1] = '\0';          // garante terminação nula

    //---configura cliente HTTP---
    esp_http_client_config_t config = {
        .url = full_url,                              // ✅ URL completa para a requisição (ex: "https://api.com/endpoint")
        .method = HTTP_METHOD_POST,                   // ✅ Tipo de requisição HTTP (GET, POST, PUT, DELETE, etc.)
        .timeout_ms = 5000,                           // ⏱️ Tempo máximo de espera pela resposta (em milissegundos)
        //.crt_bundle_attach = esp_crt_bundle_attach, // 🔒 Usa o bundle de certificados da Espressif para validar HTTPS (sem precisar de .pem manual)
        //.disable_auto_redirect = true,              // 🔄 Se true, desativa redirecionamento automático (útil para debug)
        //.buffer_size = 1024,                        // 📥 Tamanho do buffer de leitura (recepção da resposta)
        //.buffer_size_tx = 1024,                     // 📤 Tamanho do buffer de escrita (envio do corpo da requisição)
    };

    //vTaskDelay(pdMS_TO_TICKS(100));

    //---inicializa o cliente HTTP com a configuração fornecida---
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "❌ Falha ao inicializar cliente HTTP");
        free(post_data);
        return ESP_FAIL;;
    }

    //---executa requisição---
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Connection", "close");

    ESP_LOGI(TAG, "📤 Enviando dados para %s", full_url);
    esp_err_t err = esp_http_client_perform(client);

    //---trata resposta---
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            ESP_LOGI(TAG, "✅ Dados enviados com sucesso | Status: %d", status_code);
        } else if (status_code == 201) {
            ESP_LOGI(TAG, "✅ Novo recurso criado com sucesso | Status: %d", status_code);
        } else {
            ESP_LOGW(TAG, "⚠️  Resposta inesperada | Status: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "❌ Falha na requisição: %s", esp_err_to_name(err));
        //---verificação alternativa de erro para versões mais antigas do ESP-IDF---
        int sock_errno = esp_http_client_get_errno(client);
        if (sock_errno != 0) {
            ESP_LOGE(TAG, "Socket error: %d (%s)", sock_errno, strerror(sock_errno));
        }
    }

    //---limpeza---
    free(post_data);
    esp_http_client_cleanup(client);

    return err;
}