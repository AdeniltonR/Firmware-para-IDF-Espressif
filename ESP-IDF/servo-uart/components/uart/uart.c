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

//---comunicação entre as tarefas---
QueueHandle_t tx_control_queue = NULL;

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

    //---cria a fila de controle de comandos---
    tx_control_queue = xQueueCreate(5, sizeof(uint8_t));
    if (tx_control_queue == NULL) {
        ESP_LOGE(TAG, "❌ Falha ao criar fila de controle");
    }
    ESP_LOGI(TAG, "✅ Fila de comandos inicializada com sucesso!");

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
    //ESP_LOGI(logName, "Wrote %d bytes", txBytes);
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
        uint8_t command;
        //---verifica se há comandos (timeout de 50ms)---
        if(xQueueReceive(tx_control_queue, &command, pdMS_TO_TICKS(50))) {
            switch(command) {
                case 0:  // Stop
                    ESP_LOGI(TX_TASK_TAG, "Modo periódico desativado");
                    break;
                    
                case 1:  // Start
                    ESP_LOGI(TX_TASK_TAG, "☑️  Iniciado com sucesso!");
                    uart_send_data(TX_TASK_TAG, "Sucesso!\n");
                    break;
                    
                case 2:  // servo 1 OK
                    ESP_LOGI(TX_TASK_TAG, "➡️  Servo 1 OK!");
                    uart_send_data(TX_TASK_TAG, "Servo1-OK\n");
                    break;
                    
                case 3:  // servo 2 OK
                    ESP_LOGI(TX_TASK_TAG, "➡️  Servo 2 OK!");
                    uart_send_data(TX_TASK_TAG, "Servo2-OK\n");
                    break;

                default:
                    ESP_LOGW(TX_TASK_TAG, "❌ Comando inválido recebido: %d", command);
                    char error_msg[50];
                    snprintf(error_msg, sizeof(error_msg), "ERROR:INVALID_CMD:%d\n", command);
                    uart_send_data(TX_TASK_TAG, error_msg);
                    break;
            }
        }
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
            data[rxBytes] = '\0'; // Garante terminação nula
            ESP_LOGI(RX_TASK_TAG, "⬅️  Recebido: '%s'", data);
            
            //---processa o comando (ex: "sm1;40 ou sm2;40")---
            process_uart_command((const char *)data);
        }
    }
    free(data);
}