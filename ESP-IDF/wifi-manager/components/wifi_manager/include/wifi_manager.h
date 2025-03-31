/*
 * NOME: Adenilton Ribeiro
 * DATA: 17/03/2025
 * PROJETO: Wi-Fi Manager
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para Wi-Fi Manager e conexão de internet.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
/**
 * @brief wifi_manager.h
 * 
 */
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

// ========================================================================================================
// ---BIBLIOTECA---

#include "nvs_flash.h"
#include "esp_log.h"
#include <inttypes.h>
#include "esp_wifi.h"  
#include "esp_err.h"
#include "wifi.h"
#include "access_point.h"

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

//---funções públicas---
void init_manager(void);
esp_err_t wifi_manager_init(void);
esp_err_t wifi_manager_set_mode(wifi_mode_t mode);
wifi_mode_t wifi_manager_get_mode(void);
void wifi_manager_show_saved_credentials(void);
//---funções para gerenciar o timeout---
void wifi_manager_start_timeout(int timeout_seconds);
void wifi_manager_cancel_timeout(void);
void reset_AP(void);
void reset_STA(void);

#endif //wifi_manager.h