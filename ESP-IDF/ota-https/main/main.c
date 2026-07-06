/*
 * NOME: Adenilton Ribeiro
 * DATA: 12/06/2026
 * PROJETO: Wi-Fi Manager + OTA Manager
 * VERSAO: 1.1.0
 * DESCRICAO:
 *          - feat: Biblioteca atualizada para Wi-Fi Manager e conexão de internet.
 *          - feat: Integração com OTA Manager.
 *          - feat: Botão dedicado para atualização OTA.
 *          - docs: ESP32 - ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
//---BIBLIOTECAS AUXILIARES---

#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"

#include "access_point.h"
#include "wifi_manager.h"
#include "wifi.h"
#include "ota_manager.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// const char *EXAMPLE_ESP_WIFI_SSID = "ESPIDF";   // Define o SSID (nome da rede) do Access Point Wi-Fi que será criado pelo ESP32.
// const char *EXAMPLE_ESP_WIFI_PASS = "12345678"; // Define a senha do Access Point Wi-Fi. Se a senha for deixada em branco (""), a rede será aberta (sem senha).
// int EXAMPLE_ESP_WIFI_CHANNEL      = 1;          // Define o canal de frequência Wi-Fi que o Access Point usará. O canal 6 é comum para redes 2.4 GHz.
// int EXAMPLE_MAX_STA_CONN          = 2;          // Define o número máximo de dispositivos (estações) que podem se conectar ao Access Point simultaneamente.
// int EXAMPLE_ESP_MAXIMUM_RETRY     = 10;         // Número máximo de tentativas de conexão 
// int NUMERO_MAX_TENTATIVAS         = 0;          // 1 para abilitar ele entrar no modo AP, 0 para ESP32 só reiniciar

const char *EXAMPLE_ESP_WIFI_SSID = CONFIG_ESP_WIFI_SSID;
const char *EXAMPLE_ESP_WIFI_PASS = CONFIG_ESP_WIFI_PASSWORD;
int EXAMPLE_ESP_WIFI_CHANNEL = CONFIG_ESP_WIFI_CHANNEL;
int EXAMPLE_MAX_STA_CONN = CONFIG_MAX_STA_CONN;
int EXAMPLE_ESP_MAXIMUM_RETRY = CONFIG_ESP_MAXIMUM_RETRY;
int NUMERO_MAX_TENTATIVAS = CONFIG_NUMERO_MAX_TENTATIVAS;

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

//---botão para iniciar/resetar modo Access Point---
#define PIN_start_ap       GPIO_NUM_4

//---botão para iniciar atualização OTA---
#define PIN_start_ota      GPIO_NUM_0

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";
//---variável para travar botão---
volatile bool ap_started = false; 

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void check_button_task(void *pvParameter);             // Tarefa responsável pelo botão do Access Point
static void check_ota_button_task(void *pvParameter);  // Tarefa responsável pelo botão de atualização OTA

static bool app_firmware_validation(void)
{
    ESP_LOGI(
        TAG,
        "Executando validação do firmware"
    );

    /*
     * Aqui futuramente você pode testar:
     *
     * - Wi-Fi Manager
     * - NVS
     * - sensores
     * - comunicação
     * - inicialização de tasks
     * - periféricos críticos
     */

    bool sistema_ok = true;

    if (sistema_ok)
    {
        ESP_LOGI(
            TAG,
            "Sistema funcionando corretamente"
        );

        return true;
    }

    ESP_LOGE(
        TAG,
        "Falha detectada no sistema"
    );

    return false;
}

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void) {
    ESP_LOGI(TAG, "🔄  Inicializando aplicação");

    //---inicia o timeout  Manager (180 segundos, por exemplo)---
    wifi_manager_start_timeout(180);

    //---inicialização da Manager---
    init_manager();

    //---inicializa o OTA Manager---
    esp_err_t ota_err = ota_manager_init(app_firmware_validation);

    if (ota_err != ESP_OK) {
        ESP_LOGE(TAG, "❌  Falha ao inicializar OTA Manager: %s", esp_err_to_name(ota_err));
    } else {
        ESP_LOGI(TAG, "✅ OTA Manager inicializado com sucesso");
    }
    
    //---configura o pino do botão como entrada---
    gpio_reset_pin(PIN_start_ap);
    gpio_set_direction(PIN_start_ap, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_start_ap, GPIO_PULLUP_ONLY);  // Habilita o resistor de pull-up interno

    //---configura o pino do botão de OTA como entrada---
    gpio_reset_pin(PIN_start_ota);
    gpio_set_direction(PIN_start_ota, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_start_ota, GPIO_PULLUP_ONLY);  // Habilita o resistor de pull-up interno

    //---cria uma tarefa para verificar o botão---
    xTaskCreate(&check_button_task, "check_button_task", 4096, NULL, 5, NULL);
    //---cria uma tarefa para verificar o botão de OTA---
    xTaskCreate(check_ota_button_task, "check_ota_button_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "✅ Sistema inicializado");

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
    ESP_LOGI(TAG, "✅ Tarefa do botão iniciada. Estado inicial de ap_started: %d", ap_started);

    while (1) {
        //---verifica o uso da pilha---
        //UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
        //ESP_LOGI(TAG_MAIN, "Stack high water mark: %d", stack_high_water_mark);

        //---verifica se o botão foi pressionado---
        if (gpio_get_level(PIN_start_ap) == 0) {
            if (!ap_started) {
                ESP_LOGI(TAG, "Botão pressionado! Iniciando modo Access Point...");

                //---reiniciando em modo AP---
                reset_AP();

                ap_started = true;
            } else {
                ESP_LOGI(TAG, "Access Point já está ativo.");
            }
            //---debounce---
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        //---aguarda um curto período antes de verificar novamente---
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// ========================================================================================================
/**
 * @brief Tarefa para verificar o botão de atualização OTA
 *
 * @note botão deve permanecer pressionado durante 3 segundos para iniciar a atualização.
 *
 * @param pvParameter Parâmetro da tarefa (não utilizado)
 */
static void check_ota_button_task(void *pvParameter) {
    ESP_LOGI(TAG, "✅ Tarefa do botão OTA iniciada");

    while (1) {
        //---verifica se o botão OTA foi pressionado---
        if (gpio_get_level(PIN_start_ota) == 0)
        {
            ESP_LOGI(TAG, "Botão OTA pressionado. Mantenha pressionado por 3 segundos...");

            //---aguarda confirmação de 3 segundos---
            vTaskDelay(pdMS_TO_TICKS(3000));

            //---verifica se continua pressionado---
            if (gpio_get_level(PIN_start_ota) == 0) {
                ESP_LOGI(TAG, "OTA solicitado pelo usuário");

                //---verifica se já existe OTA em andamento---
                if (ota_manager_is_running()) {
                    ESP_LOGW(TAG,"OTA já está em andamento");
                } else {
                    //---inicia atualização OTA---
                    esp_err_t err = ota_manager_start();

                    if (err == ESP_OK) {
                        ESP_LOGI(TAG, "✅ Processo OTA iniciado");
                    } else {
                        ESP_LOGE(TAG, "❌  Falha ao iniciar OTA: %s", esp_err_to_name(err));
                    }
                }

                //---aguarda o usuário soltar o botão---
                while (gpio_get_level(PIN_start_ota) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }
        }

        //---aguarda antes da próxima leitura---
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
