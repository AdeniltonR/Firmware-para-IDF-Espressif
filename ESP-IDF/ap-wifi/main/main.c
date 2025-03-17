/*
 * NOME: Adenilton Ribeiro
 * DATA: 17/03/2025
 * PROJETO: Manager
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para Manager e conexão de internet.
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
#include "wifi_manager.h"
#include "wifi.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

const char *EXAMPLE_ESP_WIFI_SSID = "ESPIDF";  // Define o SSID (nome da rede) do Access Point Wi-Fi que será criado pelo ESP32.
const char *EXAMPLE_ESP_WIFI_PASS = "12345678"; // Define a senha do Access Point Wi-Fi. Se a senha for deixada em branco (""), a rede será aberta (sem senha).
int EXAMPLE_ESP_WIFI_CHANNEL      = 1;          // Define o canal de frequência Wi-Fi que o Access Point usará. O canal 6 é comum para redes 2.4 GHz.
int EXAMPLE_MAX_STA_CONN          = 2;          // Define o número máximo de dispositivos (estações) que podem se conectar ao Access Point simultaneamente.
int EXAMPLE_ESP_MAXIMUM_RETRY     = 10;         // Número máximo de tentativas de conexão 

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define PIN_start_ap GPIO_NUM_0 

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

//---tag para identificação nos logs---
static const char *TAG_MAIN = "Main";
//---variável para travar botão---
volatile bool ap_started = false; 

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void check_button_task(void *pvParameter); // Tarefa para verificar o botão

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void) {
    //---inicia o timeout  Manager (180 segundos, por exemplo)---
    wifi_manager_start_timeout(180);

    //---inicialização da Manager---
    init_manager();
    
    //---configura o pino do botão como entrada---
    gpio_reset_pin(PIN_start_ap);
    gpio_set_direction(PIN_start_ap, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_start_ap, GPIO_PULLUP_ONLY);  // Habilita o resistor de pull-up interno

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
    ESP_LOGI(TAG_MAIN, "Tarefa do botão iniciada. Estado inicial de ap_started: %d", ap_started);

    while (1) {
        //---verifica o uso da pilha---
        //UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
        //ESP_LOGI(TAG_MAIN, "Stack high water mark: %d", stack_high_water_mark);

        //---verifica se o botão foi pressionado---
        if (gpio_get_level(PIN_start_ap) == 0) {
            if (!ap_started) {
                ESP_LOGI(TAG_MAIN, "Botão pressionado! Iniciando modo Access Point...");

                //---reiniciando em modo AP---
                reset_AP();

                ap_started = true;
            } else {
                ESP_LOGI(TAG_MAIN, "Access Point já está ativo.");
            }
            //---debounce---
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        //---aguarda um curto período antes de verificar novamente---
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
