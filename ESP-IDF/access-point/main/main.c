#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_http_server.h"

#define WIFI_SSID "ESP32_AP"
#define WIFI_PASS "12345678"
#define MAX_STA_CONN 4

static const char *TAG = "WiFi_AP";
static char scan_results[1024];

static esp_err_t root_handler(httpd_req_t *req) {
    const char *resp_str = "<html><body><h1>Bem-vindo!</h1><p><a href='/scan'>Clique aqui para escanear redes Wi-Fi</a></p></body></html>";
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t wifi_scan_handler(httpd_req_t *req) {
    uint16_t number = 10;
    wifi_ap_record_t ap_info[10];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    snprintf(scan_results, sizeof(scan_results), "<html><body><h1>Redes Disponíveis</h1><ul>");
    for (int i = 0; i < ap_count; i++) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "<li>%s (RSSI: %d)</li>", ap_info[i].ssid, ap_info[i].rssi);
        strncat(scan_results, buffer, sizeof(scan_results) - strlen(scan_results) - 1);
    }
    strncat(scan_results, "</ul></body></html>", sizeof(scan_results) - strlen(scan_results) - 1);

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, scan_results, HTTPD_RESP_USE_STRLEN);
}

static httpd_uri_t root_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_handler,
    .user_ctx = NULL
};

static httpd_uri_t scan_uri = {
    .uri = "/scan",
    .method = HTTP_GET,
    .handler = wifi_scan_handler,
    .user_ctx = NULL
};

static httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &scan_uri);
    }
    return server;
}

void wifi_init_softap(void) {
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .ssid_len = strlen(WIFI_SSID),
            .channel = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .max_connection = MAX_STA_CONN
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
    ESP_LOGI(TAG, "Access Point Iniciado. SSID: %s, Senha: %s", WIFI_SSID, WIFI_PASS);
}

void app_main(void) {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    wifi_init_softap();
    start_webserver();
}
