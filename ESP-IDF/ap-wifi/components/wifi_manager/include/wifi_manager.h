#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include "nvs_flash.h"
#include "esp_log.h"
#include <inttypes.h> 

#include "esp_err.h"

// Modos Wi-Fi suportados
typedef enum {
    WIFI_MODE_AP,   // Modo Access Point
    WIFI_MODE_STA   // Modo Station
} wifi_mode_t;

// Funções públicas
esp_err_t wifi_manager_init(void);
esp_err_t wifi_manager_set_mode(wifi_mode_t mode);
wifi_mode_t wifi_manager_get_mode(void);
void wifi_manager_show_saved_credentials(void);

#endif // __WIFI_MANAGER_H__