/*
 * NOME: Adenilton Ribeiro
 * DATA: 13/03/2025
 * PROJETO: access point.c
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para criar um access point e abrir uma página HTML para exibir redes Wi-Fi disponíveis.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include "access_point.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

static const char *TAG = "wifi softAP";

// ========================================================================================================
/**
 * @brief Manipulador de eventos Wi-Fi
 * @param arg Argumento passado para o manipulador de eventos
 * @param event_base Base do evento (Wi-Fi)
 * @param event_id ID do evento (conexão ou desconexão de um dispositivo)
 * @param event_data Dados do evento (informações sobre o dispositivo conectado/desconectado)
 */
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Estação " MACSTR " conectada, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Estação " MACSTR " desconectada, AID=%d, motivo=%d", MAC2STR(event->mac), event->aid, event->reason);
    }
}

// ========================================================================================================
/**
 * @brief Manipulador de requisições HTTP para a raiz
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Type", "text/html; charset=UTF-8");
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Manipulador de requisições HTTP para /favicon.ico
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t favicon_handler(httpd_req_t *req) {
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Manipulador para Captive Portal Detection (Android)
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t captive_portal_detection_android_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Manipulador para Captive Portal Detection (iOS)
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t captive_portal_detection_ios_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Manipulador para Captive Portal Detection (Windows)
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t captive_portal_detection_windows_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Manipulador para retornar a lista de redes Wi-Fi
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t wifi_list_handler(httpd_req_t *req) {
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    if (ap_count == 0) {
        const char *response = "[]"; // Lista vazia
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    wifi_ap_record_t ap_info[ap_count];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));

    char response[1024] = "["; // Buffer para armazenar a resposta JSON
    for (int i = 0; i < ap_count; i++) {
        char ssid[64];
        snprintf(ssid, sizeof(ssid), "\"%s\"", ap_info[i].ssid);
        strcat(response, ssid);
        if (i < ap_count - 1) {
            strcat(response, ",");
        }
    }
    strcat(response, "]");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Iniciar o servidor HTTP com suporte a Captive Portal Detection
 */
void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        // Registra o manipulador para a raiz
        httpd_uri_t uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri);

        // Registra o manipulador para /wifi/list
        httpd_uri_t wifi_list_uri = {
            .uri = "/wifi/list",
            .method = HTTP_GET,
            .handler = wifi_list_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &wifi_list_uri);

        // Registra o manipulador para /favicon.ico
        httpd_uri_t favicon_uri = {
            .uri = "/favicon.ico",
            .method = HTTP_GET,
            .handler = favicon_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &favicon_uri);

        // Registra o manipulador para /generate_204 (Android)
        httpd_uri_t captive_portal_android_uri = {
            .uri = "/generate_204",
            .method = HTTP_GET,
            .handler = captive_portal_detection_android_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &captive_portal_android_uri);

        // Registra o manipulador para /hotspot-detect.html (iOS)
        httpd_uri_t captive_portal_ios_uri = {
            .uri = "/hotspot-detect.html",
            .method = HTTP_GET,
            .handler = captive_portal_detection_ios_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &captive_portal_ios_uri);

        // Registra o manipulador para /connecttest.txt (Windows)
        httpd_uri_t captive_portal_windows_uri = {
            .uri = "/connecttest.txt",
            .method = HTTP_GET,
            .handler = captive_portal_detection_windows_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &captive_portal_windows_uri);

        ESP_LOGI(TAG, "Servidor HTTP com Captive Portal Detection iniciado");
    }
}

// ========================================================================================================
/**
 * @brief Iniciar o servidor DNS
 */
void start_dns_server(void) {
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

    if (ap_netif == NULL) {
        ESP_LOGE(TAG, "Falha ao obter a interface de rede AP");
        return;
    }

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(ap_netif, &ip_info);

    // Configura o servidor DNS
    ip_addr_t dns_addr;
    dns_addr.type = IPADDR_TYPE_V4;  // Define o tipo de endereço como IPv4
    dns_addr.u_addr.ip4.addr = ip4_addr_get_u32(&ip_info.ip);  // Configura o endereço IP

    dns_setserver(0, &dns_addr);  // Configura o servidor DNS

    ESP_LOGI(TAG, "Servidor DNS iniciado");
}

// ========================================================================================================
/**
 * @brief Configurar e iniciar o Wi-Fi softAP
 */
void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                .required = true,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finalizado. SSID:%s senha:%s canal:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);

    start_webserver();  // Iniciar o servidor HTTP após o Wi-Fi estar pronto
    start_dns_server(); // Iniciar o servidor DNS
}