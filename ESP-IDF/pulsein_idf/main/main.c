/*
* NOME: Adenilton Ribeiro
* DATA: 25/04/2025
* PROJETO: Pulsein IDF
* VERSAO: 1.0.0
* DESCRICAO: - feat: Biblioteca para leitura de pulso digital utilizando o periférico RMT com suporte a múltiplos pinos.
*            - docs: ESP32-S3 - ESP-IDF v5.4.0
* LINKS: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/rmt.html
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pulsein_idf.h"

#define TIMEOUT_MS 100  // Tempo máximo de espera por pulso (em ms)

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define PIN_ch1 4
#define PIN_ch2 5

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void pulsein_task(void *pvParameter);

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void) {
    //---criando a tarefa RTOS para leitura dos pulsos---
    xTaskCreate(pulsein_task, "pulsein_task", 4096, NULL, 5, NULL);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ========================================================================================================
/**
 * @brief Função que inicializa os pinos para leitura de pulsos digitais utilizando RMT.
 *        A tarefa irá ler continuamente os pulsos dos pinos configurados e realizar a leitura
 *        da duração dos pulsos em microsegundos. As leituras são feitas a cada 500ms.
 *
 * @note Esta função executa em uma tarefa do FreeRTOS e realiza a leitura dos pulsos de dois pinos
 *       configurados (PIN_ch1 e PIN_ch2) com uma resolução de 1 MHz. Os pulsos são lidos
 *       com um timeout de 1 segundo. Após cada leitura, os valores de duração dos pulsos são logados.
 *       A função utiliza `vTaskDelay` para criar um intervalo de 500ms entre cada iteração da leitura.
 */
void pulsein_task(void *pvParameter) {
    rmt_pulsein_init(PIN_ch1, 0, 1000000); // 1 MHz resolução
    rmt_pulsein_init(PIN_ch2, 1, 1000000);

    while (1) {
        //---leitura dos pulsos com timeout de 2.5 segundo (2.5000000 us)---
        int64_t pulse1 = rmt_pulsein_read_us(0, 2500000); 
        int64_t pulse2 = rmt_pulsein_read_us(1, 2500000);

        //---log dos valores de pulso---
        ESP_LOGI(TAG, "⚙️  Pino %d: %lld us\tPino %d: %lld us", PIN_ch1, pulse1, PIN_ch2, pulse2);

        //---atraso de 500ms entre as leituras---
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}