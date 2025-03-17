/*
 * NOME: Adenilton Ribeiro
 * DATA: 14/03/2025
 * PROJETO: ap wifi
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para criar um access point e conexão de internet.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
//---BIBLIOTECAS AUXILIARES---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"  
#include "access_point.h"
#include "wifi_manager"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define PIN_start_ap GPIO_NUM_0  // Define o pino do botão para iniciar o Access Point

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

static const char *TAG = "wifi softAP";

volatile bool ap_started = false; 

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void check_button_task(void *pvParameter);  // Tarefa para verificar o botão

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void)
{
    //---inicializar NVS---
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //---mostra os dados salvos na NVS---
    show_saved_wifi_credentials();
    // Exibe as credenciais Wi-Fi salvas
    wifi_manager_show_saved_credentials();

    // Verifica o modo Wi-Fi salvo na NVS
    wifi_mode_t mode = wifi_manager_get_mode();

    // Inicia no modo correspondente
    if (mode == WIFI_MODE_AP) {
        ESP_LOGI(TAG, "Iniciando no modo Access Point...");
        wifi_init_softap();
    } else if (mode == WIFI_MODE_STA) {
        ESP_LOGI(TAG, "Iniciando no modo Station...");
        //wifi_init_sta();
    } else {
        ESP_LOGI(TAG, "Modo Wi-Fi desconhecido. Iniciando no modo padrão (AP).");
        wifi_init_softap();
    }

    //---configura o pino do botão como entrada---
    gpio_reset_pin(PIN_start_ap);
    gpio_set_direction(PIN_start_ap, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_start_ap, GPIO_PULLUP_ONLY);  // Habilita o resistor de pull-up interno

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");

    //---cria uma tarefa para verificar o botão---
    xTaskCreate(&check_button_task, "check_button_task", 4096, NULL, 5, NULL);

    while (1) {
        //ESP_LOGI(TAG, "Aguardando pressionamento do botão para iniciar o Access Point...");
        vTaskDelay(5000 / portTICK_PERIOD_MS);  
    }
}

// ========================================================================================================
/**
 * @brief Tarefa para verificar o estado do botão
 * @param pvParameter Parâmetro da tarefa (não utilizado)
 */
void check_button_task(void *pvParameter) {
    ESP_LOGI(TAG, "Tarefa do botão iniciada. Estado inicial de ap_started: %d", ap_started);

    while (1) {
        // Verifica o uso da pilha
        //UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
        //ESP_LOGI(TAG, "Stack high water mark: %d", stack_high_water_mark);

        // Verifica se o botão foi pressionado
        if (gpio_get_level(PIN_start_ap) == 0) {
            if (!ap_started) {
                ESP_LOGI(TAG, "Botão pressionado! Iniciando modo Access Point...");
                wifi_init_softap();
                ap_started = true;
            } else {
                ESP_LOGI(TAG, "Access Point já está ativo.");
            }

            // Debounce
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        // Aguarda um curto período antes de verificar novamente
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
