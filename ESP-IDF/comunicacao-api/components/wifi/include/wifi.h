/*
 * NOME: Adenilton
 * DATA: 16/03/2025
 * PROJETO: Wi-Fi
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Conexão de wifi com teste de internet com horário.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
/**
 * @brief wifi.h
 * 
 */
#ifndef WIFI_H
#define WIFI_H

// ========================================================================================================
//---BIBLIOTECAS---

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <stdio.h>
#include <time.h>
#include "esp_sntp.h"
#include "access_point.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---
 
extern int EXAMPLE_ESP_MAXIMUM_RETRY;   
extern int NUMERO_MAX_TENTATIVAS;

// ========================================================================================================
//---MACROS---

//---configurações para o modo de operação WPA3 SAE (Simultaneous Authentication of Equals)---
#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif

//---configurações para o modo de autenticação Wi-Fi---
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* O grupo de eventos permite múltiplos bits para cada evento, mas nos preocupamos apenas com dois eventos:
 * - estamos conectados ao AP com um IP
 * - falhamos ao conectar após o número máximo de tentativas 
 */
#define WIFI_CONNECTED_BIT BIT0  // Bit para sinalizar conexão bem-sucedida 
#define WIFI_FAIL_BIT      BIT1  // Bit para sinalizar falha na conexão 

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

//---configuração do Wi-Fi em modo Station---
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void wifi_init_sta(void);
//---teste de conexão com intenet---
void initialize_hora(void);
void initialize_sntp(void);
void set_timezone(void);
char* get_current_time(void);
void test_ntp_connection(void);
//---status da conexão Wi-Fi---
bool wifi_is_connected(void); 

#endif //wifi.h