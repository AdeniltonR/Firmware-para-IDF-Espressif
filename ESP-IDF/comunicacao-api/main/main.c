/*
 * NOME: Adenilton Ribeiro
 * DATA: 17/04/2025
 * PROJETO: Comunicação API REST
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Cliente HTTP para envio de dados de velocidade à API remota
 *                    Implementação de JSON parser para estruturação de dados
 *                    Controle de conexão WiFi com reconexão automática
 *                    Timeout configurável para requisições HTTP
 *                    Sistema de logs detalhado com emojis visuais
 *                    Tratamento robusto de erros de comunicação
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
 */

// ========================================================================================================
//---BIBLIOTECAS AUXILIARES---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"  
#include "esp_log.h"
#include "access_point.h"
#include "wifi_manager.h"
#include "wifi.h"
#include "api_client.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

const char *EXAMPLE_ESP_WIFI_SSID = "ESP-IDF";   // Define o SSID (nome da rede) do Access Point Wi-Fi que será criado pelo ESP32.
const char *EXAMPLE_ESP_WIFI_PASS = "12345678"; // Define a senha do Access Point Wi-Fi. Se a senha for deixada em branco (""), a rede será aberta (sem senha).
int EXAMPLE_ESP_WIFI_CHANNEL      = 1;          // Define o canal de frequência Wi-Fi que o Access Point usará. O canal 6 é comum para redes 2.4 GHz.
int EXAMPLE_MAX_STA_CONN          = 2;          // Define o número máximo de dispositivos (estações) que podem se conectar ao Access Point simultaneamente.
int EXAMPLE_ESP_MAXIMUM_RETRY     = 10;         // Número máximo de tentativas de conexão 
int NUMERO_MAX_TENTATIVAS         = 0;          // 1 para abilitar ele entrar no modo AP, 0 para ESP32 só reiniciar

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define PIN_start_ap GPIO_NUM_0

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";
//---variável para travar botão---
volatile bool ap_started = false; 

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void check_button_task(void *pvParameter);
void send_data_to_api_task(void *pvParameters);

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

    //---cria as tarefa---
    xTaskCreate(&send_data_to_api_task, "api_task", 4096, NULL, 5, NULL);
    xTaskCreate(&check_button_task, "check_button_task", 4096, NULL, 2, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); 
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
 * @brief Tarefa de envio periódico de dados para a API
 * 
 * @note Esta tarefa:
 * 1. Aguarda até que a conexão WiFi esteja estabelecida
 * 2. Inicializa o cliente HTTP para comunicação com a API
 * 3. Envia dados periodicamente a cada 5 segundos
 * 
 * @param pvParameters Parâmetros da tarefa (não utilizado neste caso)
 * 
 * @note Comportamento:
 * - Fica bloqueada no loop inicial até WiFi conectar
 * - Encerra a tarefa se falhar na inicialização da API
 * - Logs de sucesso/erro são gerados a cada tentativa de envio
 * - Intervalo fixo de 5 segundos entre envios (ajustável)
 * 
 * @warning A URL da API está hardcoded - considerar usar configuração externa
 * @warning Não há tratamento de reconexão WiFi - pode ficar bloqueada se WiFi cair
 */
void send_data_to_api_task(void *pvParameters) {
    //---aguarda conexão WiFi---
    while(!wifi_is_connected()) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    //---inicializa o cliente API---
    if (api_client_init("http://172.17.57.14:5000") != ESP_OK) {
        ESP_LOGE("API", "Falha ao inicializar cliente API");
        vTaskDelete(NULL);
    }

    device_data_t data = {
        .id = "ESP32_002",
        .status = "produzindo",
        .velocidade = 1.5f
    };

    while(1) {
        //---envia dados para a API---
        if (api_send_device_data(&data) == ESP_OK) {
            ESP_LOGI("API", "Dados enviados com sucesso");
        } else {
            ESP_LOGE("API", "Falha ao enviar dados");
        }

        //---aguarda 5 segundos antes de enviar novamente---
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}