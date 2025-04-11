#include <stdio.h>
#include <inttypes.h>  // Para PRIu32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "velocity_calculator.h"

// Configurações
#define ENCODER_GPIO      19
#define VELOCITY_UPDATE_MS 200
#define POLIA_DIAMETRO_MM  150.0f
#define PULSOS_POR_VOLTA   20

// Protótipos
void velocity_task(void *pvParameters);
void IRAM_ATTR encoder_isr_handler(void* arg);

void app_main() {
    // 1. Inicialização
    printf("\n=== Sistema de Monitoramento ===\n");
    
    // 2. Configuração GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ENCODER_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&io_conf);
    
    // 3. Interrupção
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ENCODER_GPIO, encoder_isr_handler, NULL);
    
    // 4. Inicialização do cálculo
    velocity_init(POLIA_DIAMETRO_MM, PULSOS_POR_VOLTA);
    
    // 5. Criação da task
    xTaskCreate(velocity_task, "Velocity", 4096, NULL, 2, NULL);
    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void velocity_task(void *pvParameters) {
    while(1) {
        float speed = velocity_calculate_mps(VELOCITY_UPDATE_MS);
        uint32_t revs = velocity_get_revolutions();
        
        printf("Velocidade: %.2f m/s\n", speed);
        printf("Revoluções: %"PRIu32"\n", revs);  // Formato corrigido
        
        vTaskDelay(pdMS_TO_TICKS(VELOCITY_UPDATE_MS));
    }
}

void IRAM_ATTR encoder_isr_handler(void* arg) {
    velocity_register_pulse();  // Agora corretamente declarada
}