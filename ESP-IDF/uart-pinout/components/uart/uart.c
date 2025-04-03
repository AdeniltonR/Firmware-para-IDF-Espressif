/*
 * NOME: Adenilton Ribeiro
 * DATA: 01/04/2025
 * PROJETO: UART
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Comunição uart usano pinos.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include "uart.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (uart)
static const char *TAG = "uart";
//---variável global para armazenar a porta UART configurada---
static uart_port_t uart_port = UART_DEFAULT_PORT;

// ========================================================================================================
/**
 * @brief Inicialização da UART
 * @param port Número da porta UART
 * @param tx_pin Pino de transmissão
 * @param rx_pin Pino de recepção
 * @param baud_rate Taxa de transmissão
 */
void uart_init(uart_port_t port, int tx_pin, int rx_pin, int baud_rate) {
    //---log inicial---
    ESP_LOGI(TAG, "🔁 Iniciando configuração da UART (Porta %d)...", port);
    ESP_LOGI(TAG, "🔁 Pinos: TX=%d, RX=%d", tx_pin, rx_pin);
    ESP_LOGI(TAG, "🔁 Baud rate: %d", baud_rate);

    //---configuração dos parâmetros da UART---
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    //---instala driver---
    if (uart_driver_install(port, UART_RX_BUF_SIZE * 2, 0, 0, NULL, 0) == ESP_OK) {
        ESP_LOGI(TAG, "✅ Driver UART instalado (RX Buffer: %d bytes)", UART_RX_BUF_SIZE * 2);
    } else {
        ESP_LOGE(TAG, "❌ Falha ao instalar driver UART!");
        return;
    }
    
    //---aplica configuração---
    if (uart_param_config(port, &uart_config) == ESP_OK) {
        ESP_LOGI(TAG, "✅ Parâmetros da UART configurados com sucesso!");
    } else {
        ESP_LOGE(TAG, "❌ Falha ao configurar parâmetros da UART!");
        return;
    }


    //---configura pinos---
    if (uart_set_pin(port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) == ESP_OK) {
        ESP_LOGI(TAG, "✅ Pinos da UART definidos: TX=%d, RX=%d", tx_pin, rx_pin);
    } else {
        ESP_LOGE(TAG, "❌ Falha ao definir pinos da UART!");
        return;
    }

    uart_port = port; // Guarda a porta configurada
    ESP_LOGI(TAG, "✅ UART inicializada com sucesso no port %d", port);
}

// ========================================================================================================
/**
 * @brief Função para envio de dados
 * @param logName Tag para logs
 * @param data Dados a serem enviados
 * @return Número de bytes enviados
 */
int uart_send_data(const char* logName, const char* data) {
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(uart_port, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

// ========================================================================================================
/**
 * @brief Tarefa de transmissão
 * @param arg Argumentos da tarefa
 */
void tx_task(void *arg) {
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        uart_send_data(TX_TASK_TAG, "Hello world \n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

// ========================================================================================================
/**
 * @brief Tarefa de recepção
 * @param arg Argumentos da tarefa
 */
void rx_task(void *arg) {
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(UART_RX_BUF_SIZE + 1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, UART_RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}