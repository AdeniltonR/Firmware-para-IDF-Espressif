#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "html.h"  // Inclui o arquivo HTML

#include "lwip/err.h"
#include "lwip/sys.h"

#define EXAMPLE_ESP_WIFI_SSID      "ESP-IDF"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL   6
#define EXAMPLE_MAX_STA_CONN       2
#define LED_GPIO_PIN               GPIO_NUM_2  // Pino D2

static const char *TAG = "wifi softAP";

// Manipulador de eventos Wi-Fi
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

// Manipulador de requisições HTTP para a raiz
static esp_err_t root_handler(httpd_req_t *req)
{
    // Define o tipo de conteúdo como HTML com codificação UTF-8
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Type", "text/html; charset=UTF-8");

    // Envia o conteúdo HTML
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Manipulador de requisições HTTP para /led/on
static esp_err_t led_on_handler(httpd_req_t *req)
{
    gpio_set_level(LED_GPIO_PIN, 1);  // Liga o LED
    ESP_LOGI(TAG, "LED ligado");      // Log quando o LED é ligado
    httpd_resp_send(req, "LED ligado", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Manipulador de requisições HTTP para /led/off
static esp_err_t led_off_handler(httpd_req_t *req)
{
    gpio_set_level(LED_GPIO_PIN, 0);  // Desliga o LED
    ESP_LOGI(TAG, "LED desligado");   // Log quando o LED é desligado
    httpd_resp_send(req, "LED desligado", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Manipulador de requisições HTTP para /favicon.ico
static esp_err_t favicon_handler(httpd_req_t *req)
{
    // Retorna uma resposta vazia para evitar o erro 404
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// Iniciar o servidor HTTP
static void start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri);

        httpd_uri_t led_on_uri = {
            .uri = "/led/on",
            .method = HTTP_GET,
            .handler = led_on_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &led_on_uri);

        httpd_uri_t led_off_uri = {
            .uri = "/led/off",
            .method = HTTP_GET,
            .handler = led_off_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &led_off_uri);

        httpd_uri_t favicon_uri = {
            .uri = "/favicon.ico",
            .method = HTTP_GET,
            .handler = favicon_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &favicon_uri);

        ESP_LOGI(TAG, "Servidor HTTP iniciado");
    }
}

// Configurar e iniciar o Wi-Fi softAP
void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));

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

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);

    // Iniciar o servidor HTTP após o Wi-Fi estar pronto
    start_webserver();
}

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
    wifi_init_softap();
}