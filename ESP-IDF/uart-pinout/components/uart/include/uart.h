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
/**
 * @brief uart.h
 * 
 */
#ifndef  UART_H
#define  UART_H

// ========================================================================================================
// ---BIBLIOTECA---

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

#define UART_DEFAULT_PORT       UART_NUM_1
#define UART_DEFAULT_BAUD_RATE  115200
#define UART_RX_BUF_SIZE        1024

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define UART_DEFAULT_TXD_PIN    GPIO_NUM_4
#define UART_DEFAULT_RXD_PIN    GPIO_NUM_5

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void uart_init(uart_port_t port, int tx_pin, int rx_pin, int baud_rate);
int uart_send_data(const char* logName, const char* data);
void tx_task(void *arg);
void rx_task(void *arg);

#endif //uart.h