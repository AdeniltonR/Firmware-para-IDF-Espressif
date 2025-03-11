/*
 * NOME: Adenilton Ribeiro
 * DATA: 11/03/2025
 * PROJETO: emonlib-esp-idf.h
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para ler tensao e corrente da Emonlib.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS: Link de referencia - https://github.com/uktechbr/emonlib-esp-idf/tree/main
*/

// ========================================================================================================
//---BIBLIOTECAS AUXILIARES---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "emonlib-esp-idf.h" 

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
    //---inicializa a biblioteca com os valores de calibração---
    double VCAL = 300.6;    // Valor de calibração para tensão (ajuste conforme necessário)
    double PHASECAL = 0.8;  // Valor de calibração para fase (ajuste conforme necessário)
    double ICAL = 40.0;     // Valor de calibração para corrente (ajuste conforme necessário)
    emonlib_init(VCAL, PHASECAL, ICAL);

    //---estrutura para armazenar os dados calculados---
    emonlib_esp_idf_data_t data;

    while (1) {
        //---calcula os valores de tensão, corrente, potência e fator de potência---
        unsigned int crossings = 20;  // Número de cruzamentos por zero para medição
        unsigned int timeout = 1000;  // Timeout em milissegundos
        emonlib_calc_vi(crossings, timeout, &data);

        //---exibe os valores calculados---
        printf("Tensão RMS: %.2f V\n", data.Vrms);
        for (int i = 0; i < NUM_SENSORS; i++) {
            printf("Corrente RMS (Sensor %d): %.2f A\n", i + 1, data.Irms[i]);
            printf("Potência Real (Sensor %d): %.2f W\n", i + 1, data.realPower[i]);
            printf("Potência Aparente (Sensor %d): %.2f VA\n", i + 1, data.apparentPower[i]);
            printf("Fator de Potência (Sensor %d): %.2f\n", i + 1, data.powerFactor[i]);
        }
        printf("----------------------------------------\n");

        //---aguarda 1 segundo antes da próxima leitura---
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}