#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#define AP_SSID "ESP32_AP"
#define AP_PASS "12345678"
#define MAX_STA_CONN 4

static const char *TAG = "wifi_ap";

static esp_err_t wifi_scan_get_handler(httpd_req_t *req) {
    uint16_t number = 10;
    wifi_ap_record_t ap_info[10];
    uint16_t ap_count = 0;
    char buffer[1024] = "<html><body><h2>Redes WiFi Disponíveis</h2><form method='post'>";

    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    
    for (int i = 0; i < ap_count; i++) {
        char ap_entry[128];
        snprintf(ap_entry, sizeof(ap_entry), "<input type='radio' name='ssid' value='%s'> %s<br>", ap_info[i].ssid, ap_info[i].ssid);
        strcat(buffer, ap_entry);
    }

    strcat(buffer, "Senha: <input type='password' name='password'><br>");
    strcat(buffer, "<input type='submit' value='Conectar'></form></body></html>");

    httpd_resp_send(req, buffer, strlen(buffer));
    return ESP_OK;
}

static esp_err_t wifi_connect_post_handler(httpd_req_t *req) {
    char content[100];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) return ESP_FAIL;
    content[ret] = '\0';
    
    char ssid[32] = {0}, password[64] = {0};
    sscanf(content, "ssid=%31[^&]&password=%63s", ssid, password);
    
    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    httpd_resp_send(req, "Conectando...", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static httpd_uri_t wifi_scan = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = wifi_scan_get_handler,
    .user_ctx = NULL
};

static httpd_uri_t wifi_connect = {
    .uri = "/connect",
    .method = HTTP_POST,
    .handler = wifi_connect_post_handler,
    .user_ctx = NULL
};

static httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &wifi_scan);
        httpd_register_uri_handler(server, &wifi_connect);
    }
    return server;
}

void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    
    wifi_config_t ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .password = AP_PASS,
            .ssid_len = strlen(AP_SSID),
            .channel = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = MAX_STA_CONN
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_softap();
    start_webserver();
}
