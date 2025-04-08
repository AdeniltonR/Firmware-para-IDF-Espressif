/*
 * NOME: Adenilton Ribeiro
 * DATA: 03/04/2025
 * PROJETO: UART Control
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Controle da UART.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include "uart_control.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (uart-control)
static const char *TAG = "uart-control";

// ========================================================================================================
/**
 * @brief Inicializa a uart
 * 
 */
void uart_init_control(void) {
    //---inicializa com pinos padrão---
    uart_init(UART_DEFAULT_PORT, UART_DEFAULT_TXD_PIN, UART_DEFAULT_RXD_PIN, UART_DEFAULT_BAUD_RATE);
}

// ========================================================================================================
/**
 * @brief Processa os comandos recebidos via UART
 * @param data Dados recebidos (string terminada em '\0')
 */
void process_uart_command(const char *data) {
    char *token;
    char *rest = (char *)data;
    float servo_angle;

    //---exemplo: "sm1;40" ou "sm2;90"---
    token = strtok_r(rest, ";", &rest);
    if (token == NULL) return;

    if (strcmp(token, "sm1") == 0) {
        token = strtok_r(rest, ";", &rest);
        if (token != NULL) {
            servo_angle = atof(token);
            xQueueSend(xQueueServo1, &servo_angle, portMAX_DELAY);  // Envia para a fila do Servo 1
        }
    } 
    else if (strcmp(token, "sm2") == 0) {
        token = strtok_r(rest, ";", &rest);
        if (token != NULL) {
            servo_angle = atof(token);
            xQueueSend(xQueueServo2, &servo_angle, portMAX_DELAY);  // Envia para a fila do Servo 2
        }
    }
}

/**
 * @brief Enviar comandos para uart
 * 
 * @note 1 - iniciado com sucesso
 * @note 2 - servo 1 OK
 * @note 3 - servo 2 OK
 *       
 * @param cmd 
 */
void enviar_comandos_uart(int cmd) {
    switch(cmd) {
        case 1://---iniciado com sucesso---
            xQueueSend(tx_control_queue, &cmd, portMAX_DELAY);
            break;

        case 2://---servo 1 OK---
            xQueueSend(tx_control_queue, &cmd, portMAX_DELAY);
            break;

        case 3://---servo 1 OK---
            xQueueSend(tx_control_queue, &cmd, portMAX_DELAY);
            break;
        
        default:
            ESP_LOGW(TAG, "❌ Comando inválido recebido: %d", cmd);
            char error_msg[50];
            snprintf(error_msg, sizeof(error_msg), "ERROR:INVALID_CMD:%d\n", cmd);
            uart_send_data(TAG, error_msg);
            break;
        }
}
