/*
 * NOME: Nome
 * DATA: 01/04/2025
 * PROJETO: USB
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Comunição uart usano pinos.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "usb.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void) {

    //---inicializa com pinos padrão---
    uart_init(UART_DEFAULT_PORT, UART_DEFAULT_TXD_PIN, UART_DEFAULT_RXD_PIN, UART_DEFAULT_BAUD_RATE);
    
    //---criando tarefas---
    xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 2, NULL);

    while(1) {
        //...
    }
}