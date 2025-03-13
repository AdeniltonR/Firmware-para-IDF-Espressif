/*
 * NOME: Adenilton Ribeiro
 * DATA: 13/03/2025
 * PROJETO: access point.h
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para criar um access point e abrir uma página HTML para controle de um led.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
/**
 * @brief ccess_point.h
 * 
 */
#ifndef __ACCESS_POINT_H__
#define __ACCESS_POINT_H__

// ========================================================================================================
// ---BIBLIOTECA---

#include <string.h>
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/dns.h"

#include "html.h"  // Inclui o arquivo HTML

#include "lwip/err.h"
#include "lwip/sys.h"

// ========================================================================================================
//---CONSTANTS---

#define EXAMPLE_ESP_WIFI_SSID      "ESP-IDF"  // Define o SSID (nome da rede) do Access Point Wi-Fi que será criado pelo ESP32.
#define EXAMPLE_ESP_WIFI_PASS      "12345678" // Define a senha do Access Point Wi-Fi. Se a senha for deixada em branco (""), a rede será aberta (sem senha).
#define EXAMPLE_ESP_WIFI_CHANNEL   6          // Define o canal de frequência Wi-Fi que o Access Point usará. O canal 6 é comum para redes 2.4 GHz.
#define EXAMPLE_MAX_STA_CONN       2          // Define o número máximo de dispositivos (estações) que podem se conectar ao Access Point simultaneamente.

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define LED_GPIO_PIN               GPIO_NUM_2  // Pino D2

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
esp_err_t root_handler(httpd_req_t *req);
esp_err_t led_on_handler(httpd_req_t *req);
esp_err_t led_off_handler(httpd_req_t *req);
esp_err_t favicon_handler(httpd_req_t *req);
esp_err_t captive_portal_detection_android_handler(httpd_req_t *req);
esp_err_t captive_portal_detection_ios_handler(httpd_req_t *req);
esp_err_t captive_portal_detection_windows_handler(httpd_req_t *req);
void start_webserver(void);
void start_dns_server(void);
void wifi_init_softap(void);

#endif // access_point.h