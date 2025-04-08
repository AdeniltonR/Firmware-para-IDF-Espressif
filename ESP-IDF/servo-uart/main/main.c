/*
 * NOME: Adenilton Ribeiro
 * DATA: 03/04/2025
 * PROJETO: Servo UART
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Controle de dois servos usando UART com Raspberry Pi 4 Model B.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: 
 */

// ========================================================================================================
//---BIBLIOTECAS---

#include "servo_control.h"
#include "uart_control.h"

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

// ========================================================================================================
//---PROTOTIPOS DAS FUNCOES---

// ========================================================================================================
/**
 * @brief Função principal.
 */
void app_main() {
    //---inicializa o servo---
    servo_init_control();

    //---inicializa a uart---
    uart_init_control();
    
    //---cria as tarefas para controlar os servos---
    xTaskCreate(servo1_task, "servo1_task", 4096, NULL, 3, NULL);
    xTaskCreate(servo2_task, "servo2_task", 4096, NULL, 3, NULL);
    //---criando tarefas uart---
    xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 2, NULL);

    //---enviando comandos---
    enviar_comandos_uart(1);

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}