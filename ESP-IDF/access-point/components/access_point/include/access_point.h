#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/dns.h"
#include "html.h"  // Inclui o arquivo HTML

#define EXAMPLE_ESP_WIFI_SSID      "ESP-IDF"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL   6
#define EXAMPLE_MAX_STA_CONN       2

#define LED_GPIO_PIN               GPIO_NUM_2  // Pino D2

// Função para inicializar o Wi-Fi no modo Access Point
void wifi_init_softap(void);

// Manipulador de eventos Wi-Fi
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

// Manipulador de requisições HTTP para a raiz
esp_err_t root_handler(httpd_req_t *req);

// Manipulador de requisições HTTP para /led/on
esp_err_t led_on_handler(httpd_req_t *req);

// Manipulador de requisições HTTP para /led/off
esp_err_t led_off_handler(httpd_req_t *req);

// Manipulador de requisições HTTP para /favicon.ico
esp_err_t favicon_handler(httpd_req_t *req);

// Manipulador para Captive Portal Detection (Android)
esp_err_t captive_portal_detection_android_handler(httpd_req_t *req);

// Manipulador para Captive Portal Detection (iOS)
esp_err_t captive_portal_detection_ios_handler(httpd_req_t *req);

// Manipulador para Captive Portal Detection (Windows)
esp_err_t captive_portal_detection_windows_handler(httpd_req_t *req);

// Iniciar o servidor HTTP
void start_webserver(void);

// Iniciar o servidor DNS
void start_dns_server(void);

#endif // ACCESS_POINT_H