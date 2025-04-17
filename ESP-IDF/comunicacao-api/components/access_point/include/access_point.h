/*
 * NOME: Adenilton Ribeiro
 * DATA: 13/03/2025
 * PROJETO: access point.h
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para criar um access point e abrir uma página HTML para configuração de Wi-Fi.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
/**
 * @brief access_point.h
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
#include "esp_system.h" 
#include "cJSON.h"      
#include "html.h"       
#include "wifi_manager.h"
#include "lwip/err.h" 
#include "lwip/sys.h"

// ========================================================================================================
//---CONSTANTS---

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

extern const char *EXAMPLE_ESP_WIFI_SSID;
extern const char *EXAMPLE_ESP_WIFI_PASS;
extern int EXAMPLE_ESP_WIFI_CHANNEL;
extern int EXAMPLE_MAX_STA_CONN;

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

//---funções de inicialização e configuração---
void wifi_init_softap(void);
void start_webserver(void);
void start_dns_server(void);
void stop_webserver(void);
//---manipuladores de eventos Wi-Fi---
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
//---manipuladores de requisições HTTP---
esp_err_t root_handler(httpd_req_t *req);
esp_err_t wifi_connect_handler(httpd_req_t *req); 
esp_err_t close_ap_handler(httpd_req_t *req);
esp_err_t connected_html_handler(httpd_req_t *req);
esp_err_t favicon_handler(httpd_req_t *req);
//---manipuladores de Captive Portal Detection---
esp_err_t captive_portal_detection_android_handler(httpd_req_t *req);
esp_err_t captive_portal_detection_ios_handler(httpd_req_t *req);
esp_err_t captive_portal_detection_windows_handler(httpd_req_t *req);
//---funções para manipulação de credenciais Wi-Fi---
esp_err_t save_wifi_credentials(const char *ssid, const char *password);  
esp_err_t load_wifi_credentials(char *ssid, size_t ssid_size, char *password, size_t password_size);  
void show_saved_wifi_credentials(void);  

#endif // access_point.h