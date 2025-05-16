/*
 * NOME: Adenilton Ribeiro
 * DATA: 16/05/2025
 * PROJETO: PWM com RTOS
 * VERSAO: 03
 * DESCRICAO: - feat: Controle PWM com valor externo
 *            - docs: ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <inttypes.h> 
#include "esp_log.h"

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define PIN_pwm 2

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";

volatile int pwm_duty_percent = 0;   // Valor em porcentagem (0 a 100)
static int direction = 1;            // 1 para incremento, -1 para decremento

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void pwm_init(void);
void pwm_task(void *param);

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void) {

    //---inicialização PWM---
    pwm_init();

    //---criando a tarefa RTOS para PWM---
    xTaskCreate(pwm_task, "PWM Task", 2048, NULL, 5, NULL);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ========================================================================================================
/**
 * @brief Inicialização do PWM
 *
 */
void pwm_init(void) {
    //---configura o temporizador do PWM---
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,               // Modo de baixa velocidade
        .timer_num = LEDC_TIMER_0,                       // Temporizador 0
        .duty_resolution = LEDC_TIMER_13_BIT,            // Resolução de 13 bits (8192 níveis)
        .freq_hz = 5000,                                 // Frequência do PWM de 5 kHz
        .clk_cfg = LEDC_AUTO_CLK                         // Configuração automática do clock
    };
    //---aplica a configuração do temporizador---
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));     // Adicionado tratamento de erro
    ESP_LOGI(TAG, "⚙️  Temporizador PWM configurado: %" PRIu32 " Hz, %" PRIu32 " bits", (uint32_t) ledc_timer.freq_hz, (uint32_t) ledc_timer.duty_resolution);
    
    //---configura o canal PWM---
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,               // Modo de baixa velocidade
        .channel = LEDC_CHANNEL_0,                       // Canal 0
        .timer_sel = LEDC_TIMER_0,                       // Seleciona o temporizador 0
        .intr_type = LEDC_INTR_DISABLE,                  // Interrupções desabilitadas
        .gpio_num = PIN_pwm,                             // Define o pino de saída PWM (definido pelo usuário)
        .duty = 0,                                       // Duty cycle inicial de 0%
        .hpoint = 0                                      // Ponto de início
    };
    //---aplica a configuração do canal---
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel)); // Adicionado tratamento de erro
    ESP_LOGI(TAG, "⚙️  Canal PWM configurado no pino GPIO %d", ledc_channel.gpio_num);
}

// ========================================================================================================
/**
 * @brief Task de controle PWM
 * @param param Parâmetro de criação da task (não utilizado nesta implementação)
 * 
 * @note Esta task monitora e atualiza continuamente o duty cycle do PWM com base no valor global
 * pwm_duty_percent. A atualização só ocorre quando há mudança no valor desejado.
 * O duty cycle é limitado a 0-100% e convertido para o range de 13 bits (0-8191).
*/
void pwm_task(void *param) {
    ESP_LOGI(TAG, "✅ Task PWM iniciada");
    int last_duty = -1;  // Armazena o último valor para evitar atualizações desnecessárias

    while (1) {
        //---incrementa/decrementa o duty cycle em 10%---
        pwm_duty_percent += (10 * direction);
        
        //---verifica os limites e inverte a direção se necessário---
        if (pwm_duty_percent >= 100) {
            pwm_duty_percent = 100;
            direction = -1;  // Muda para decremento
            ESP_LOGI(TAG, "🔝 Máximo alcançado (100), invertendo direção");
        } 
        else if (pwm_duty_percent <= 0) {
            pwm_duty_percent = 0;
            direction = 1;   // Muda para incremento
            ESP_LOGI(TAG, "🔚 Mínimo alcançado (0), invertendo direção");
        }

        //---limita o valor para entre 0 e 100%---
        int current_duty = pwm_duty_percent;
        if (current_duty < 0) current_duty = 0;
        if (current_duty > 100) current_duty = 100;

        //---só atualiza se o valor tiver mudado---
        if (current_duty != last_duty) {
            int duty_cycle = (current_duty * 8191) / 100;
            ESP_LOGI(TAG, "⚡ Atualizando duty cycle para %d%% (valor bruto: %d)", current_duty, duty_cycle);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            last_duty = current_duty;
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Ajuste conforme necessário
    }
}
