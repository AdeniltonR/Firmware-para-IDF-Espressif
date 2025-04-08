
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
/**
 * @brief uart_control.h
 * 
 */
#ifndef  UART_CONTROL_H
#define  UART_CONTROL_H

// ========================================================================================================
// ---BIBLIOTECA---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "uart.h"

#include "servo_control.h"

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void uart_init_control(void);
void process_uart_command(const char *data);
void enviar_comandos_uart(int cmd);

#endif //uart_control.h
