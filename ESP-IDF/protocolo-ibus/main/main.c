/*
 * NOME: Adenilton Ribeiro
 * DATA: 31/01/2025
 * PROJETO: i-BUS
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Envio de dados de múltiplos sensores via protocolo i-BUS (FlySky) usando UART.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "ibus.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";

//---instância principal do protocolo iBUS---
static ibusbm_t ibus;                
//---Endereço do sensor registrado no protocolo---
static uint8_t sensor_voltage_addr = 0;
static uint8_t sensor_temp_addr = 0;
static uint8_t sensor_rpm_addr = 0;      

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void update_sensor_task(void *arg);
void loop_task(void *arg);

// ========================================================================================================
/**
 * @brief Função principal da aplicação.
 *
 * Inicializa o protocolo iBUS, registra sensor simulado,
 * e cria as tasks de atualização e processamento do protocolo.
*/
void app_main(void) {
    ESP_LOGI(TAG, "✅ Inicializando protocolo iBUS...");

    //---inicializa protocolo iBUS com UART e pinos definidos---
    ibusbm_init(&ibus, UART_NUM, UART_RX_PIN, UART_TX_PIN);
    ESP_LOGI(TAG, "✅ UART configurada: TX=%d, RX=%d", UART_TX_PIN, UART_RX_PIN);

    //---adiciona sensores---
    sensor_voltage_addr = ibusbm_add_sensor(&ibus, IBUSS_EXTV, 2);
    sensor_temp_addr    = ibusbm_add_sensor(&ibus, IBUSS_TEMP, 2);
    sensor_rpm_addr     = ibusbm_add_sensor(&ibus, IBUSS_RPM, 2);

    ESP_LOGI(TAG, "⚙️  Sensores registrados: Tensão=%d, Temperatura=%d, RPM=%d",
             sensor_voltage_addr, sensor_temp_addr, sensor_rpm_addr);

    //---cria tarefas para gerenciar protocolo e simulação de sensor---
    xTaskCreate(loop_task, "ibus_loop", 4096, NULL, 5, NULL);
    xTaskCreate(update_sensor_task, "update_sensor", 4096, NULL, 5, NULL);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ========================================================================================================
/**
 * @brief Task de atualização periódica dos dados do sensor.
 *
 * Essa tarefa simula uma leitura de tensão (ex: bateria 12.3V),
 * e envia o valor para o protocolo iBUS a cada 100ms.
 *
 * @param arg Ponteiro não utilizado.
*/
void update_sensor_task(void *arg) {
    while (1) {
        //---tensão simulada (7,86 V)---
        int32_t voltage_cv = 786; // centivolts
        ibusbm_set_sensor_value(&ibus, sensor_voltage_addr, voltage_cv);
        ESP_LOGI(TAG, "⚡ Tensão enviada: %.2f V", voltage_cv / 100.0);

        //---temperatura simulada (24,5 °C)---
        float temp_celsius = 24.5;
        int32_t temp_raw = (int32_t)((temp_celsius + 40.0) * 10);  // Conversão para protocolo iBUS
        ibusbm_set_sensor_value(&ibus, sensor_temp_addr, temp_raw);
        ESP_LOGI(TAG, "🌡️  Temperatura enviada: %.1f °C (raw=%" PRId32 ")", temp_celsius, temp_raw);

        //---RPM simulado (3.00rpm) ---
        int32_t rpm = 300;
        ibusbm_set_sensor_value(&ibus, sensor_rpm_addr, rpm);
        ESP_LOGI(TAG, "🌀  RPM enviado: %" PRId32, rpm);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ========================================================================================================
/**
 * @brief Task principal de loop do protocolo iBUS.
 *
 * Executa a função `ibusbm_loop()` para tratar comandos do transmissor
 * e responder com dados de sensores registrados.
 *
 * @note Essa task deve rodar com intervalo baixo (ex: 1ms) para garantir
 *       resposta rápida ao transmissor FlySky.
 *
 * @param arg Ponteiro não utilizado.
*/
void loop_task(void *arg) {
    while (1) {
        ibusbm_loop(&ibus);               // Processa dados do protocolo iBUS
        vTaskDelay(pdMS_TO_TICKS(1));     // Delay curto para evitar travamento
    }
}
