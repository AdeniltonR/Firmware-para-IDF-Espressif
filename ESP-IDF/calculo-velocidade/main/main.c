/*
* NOME: Adenilton Ribeiro
* DATA: 15/04/2025
* PROJETO: Calculo de Velocidade
* VERSAO: 1.0.0
* DESCRICAO: - feat: Sistema de medição de velocidade linear usando encoder de 200 PPR
 *                   Cálculo em tempo real de velocidade (m/s e km/h) e distância percorrida
 *                   Interface com polia de 160mm de diâmetro
 *                   Implementação de filtro de média móvel para suavização
 *                   Controle de interrupções de alta performance com IRAM_ATTR
*            - docs: ESP32 32D - ESP-IDF v5.4.0
* LINKS:
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "calculo_velocidade.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

#define POLIA_DIAMETRO_MM 160.0f
#define PULSOS_POR_VOLTA  200
#define UPDATE_INTERVAL_MS 2000

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define ENCODER_GPIO      19

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void IRAM_ATTR encoder_isr_handler(void* arg);
void velocity_task(void *pvParameters);

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main() {
    //---configuração do GPIO---
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ENCODER_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&io_conf);
    
    //---configuração da interrupção---
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ENCODER_GPIO, encoder_isr_handler, NULL);
    
    //---inicialização do cálculo---
    velocity_init(POLIA_DIAMETRO_MM, PULSOS_POR_VOLTA);
    
    //---criação da task---
    xTaskCreate(velocity_task, "velocity_task", 4096, NULL, 2, NULL);
    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ========================================================================================================
/**
 * @brief Manipulador de interrupção para pulsos do encoder
 * @param arg Argumento genérico (não utilizado)
 * @note Função alocada na IRAM para acesso rápido durante interrupções
 * @note Chama velocity_register_pulse() para contagem de pulsos
 */
void IRAM_ATTR encoder_isr_handler(void* arg) {
    velocity_register_pulse();
}

// ========================================================================================================
/**
 * @brief Task principal para cálculo e exibição de parâmetros de velocidade
 * @param pvParameters Parâmetros da task (não utilizado)
 * @note Executa em loop contínuo com intervalo definido por UPDATE_INTERVAL_MS
 * @note Realiza:
 *        - Cálculo de velocidade em m/s e km/h
 *        - Cálculo de distância percorrida
 *        - Exibição dos parâmetros via serial
 */
void velocity_task(void *pvParameters) {
    while(1) {
        // Cálculos
        float speed_mps = velocity_calculate_mps(UPDATE_INTERVAL_MS);
        float speed_kmph = velocity_calculate_kmph(UPDATE_INTERVAL_MS);
        float distance_m = velocity_get_distance_meters();
        float mm_per_pulse = velocity_get_mm_per_pulse();
        
        // Exibição
        printf("\n--- Dados do Encoder ---\n");
        printf("Velocidade: %.2f m/s (%.1f km/h)\n", speed_mps, speed_kmph);
        printf("Distância: %.3f metros\n", distance_m);
        printf("Resolução: %.4f mm/pulso\n", mm_per_pulse);
        printf("Pulsos totais: %"PRIu32"\n", velocity_get_raw_pulses());
        
        vTaskDelay(pdMS_TO_TICKS(UPDATE_INTERVAL_MS));
    }
}