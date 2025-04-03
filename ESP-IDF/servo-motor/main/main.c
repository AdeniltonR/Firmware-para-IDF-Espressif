/*
 * NOME: Adenilton Ribeiro
 * DATA: 03/04/2025
 * PROJETO: Servo Motor
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Controle de dois servo motores usando RTOS.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: - https://docs.espressif.com/projects/esp-iot-solution/en/latest/motor/servo.html
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "servo_motor.h"
#include "esp_log.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

//---definição dos pinos e canais---
#define SERVO1_PIN      GPIO_NUM_20
#define SERVO1_CHANNEL  LEDC_CHANNEL_0
#define SERVO2_PIN      GPIO_NUM_2
#define SERVO2_CHANNEL  LEDC_CHANNEL_1

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";

//---configuração dos servos---
static servo_config_t servo_cfg = {
    .max_angle = 180,                           // Ângulo máximo do servo (180 graus)
    .min_width_us = 500,                        // Largura de pulso mínima (500us)
    .max_width_us = 2500,                       // Largura de pulso máxima (2500us)
    .freq = 50,                                 // Frequência PWM (50Hz)
    .timer_number = LEDC_TIMER_0,
    .channels = {
        .servo_pin = {SERVO1_PIN, SERVO2_PIN},  // Pinos dos servos
        .ch = {SERVO1_CHANNEL, SERVO2_CHANNEL}  // Canais LEDC
    },
    .channel_number = 2                         // Número de servos (2)
};

//---parâmetros de velocidade---
typedef struct {
    float speed_deg_per_sec;  // Velocidade em graus por segundo
    int step_size;            // Tamanho do passo em graus
} servo_speed_t;

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

static void servo1_task(void *pvParameter);
static void servo2_task(void *pvParameter);
static void move_servo_smoothly(uint8_t channel, float target_angle, const servo_speed_t *speed_cfg);

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main(void)
{
    //---inicializa a biblioteca de servos---
    esp_err_t ret = iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Falha ao inicializar servos: %d", ret);
        return;
    }

    //---cria as tarefas para controlar os servos---
    xTaskCreate(servo1_task, "servo1_task", 2048, NULL, 5, NULL);
    xTaskCreate(servo2_task, "servo2_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "✅ Controle de servos iniciado!");
}

// ========================================================================================================
/**
 * @brief Tarefa para controle contínuo do Servo 1
 * 
 * @param pvParameter Parâmetro genérico do FreeRTOS (não utilizado nesta implementação)
 * 
 * @note Esta tarefa move o Servo 1 em um movimento de varredura entre 0° e 180° com incrementos de 10°,
 *       lê e exibe a posição atual em ambos os limites do movimento.
 *       A velocidade é controlada por um delay de 100ms entre cada movimento.
 */
static void servo1_task(void *pvParameter) {
    float current_angle;

    servo_speed_t speed_cfg = {
        .speed_deg_per_sec = 60.0f,  // 60 graus por segundo
        .step_size = 5               // Passo de 5 graus
    };

    while(1) {

        //---move para 180° com velocidade controlada---
        move_servo_smoothly(SERVO1_CHANNEL, 180.0f, &speed_cfg);
        
        //---lê e exibe a posição---
        if(iot_servo_read_angle(LEDC_LOW_SPEED_MODE, SERVO1_CHANNEL, &current_angle) == ESP_OK) {
            ESP_LOGI(TAG, "1️⃣  Posição: %.1f° | Velocidade: %.1f°/s", 
                    current_angle, speed_cfg.speed_deg_per_sec);
        }
        
        //---move para 0° com velocidade controlada---
        move_servo_smoothly(SERVO1_CHANNEL, 0.0f, &speed_cfg);
        
        //---lê e exibe a posição---
        if(iot_servo_read_angle(LEDC_LOW_SPEED_MODE, SERVO1_CHANNEL, &current_angle) == ESP_OK) {
            ESP_LOGI(TAG, "1️⃣  Posição: %.1f° | Velocidade: %.1f°/s", 
                    current_angle, speed_cfg.speed_deg_per_sec);
        }
        
        //---aumenta a velocidade para o próximo ciclo (até 180°/s)---
        speed_cfg.speed_deg_per_sec += 30.0f;
        if(speed_cfg.speed_deg_per_sec > 180.0f) {
            speed_cfg.speed_deg_per_sec = 30.0f;
        }
    }
}

// ========================================================================================================
/**
 * @brief Tarefa para controle contínuo do Servo 2
 * 
 * @param pvParameter Parâmetro genérico do FreeRTOS (não utilizado nesta implementação)
 * 
 * @note Esta tarefa move o Servo 2 em um movimento de varredura entre 0° e 180° com incrementos de 5°,
 *       lê e exibe a posição atual em ambos os limites do movimento.
 *       A velocidade é controlada por um delay de 50ms entre cada movimento, resultando em
 *       uma operação mais rápida que o Servo 1.
 */
static void servo2_task(void *pvParameter) {
    float current_angle;

    servo_speed_t speed_cfg = {
        .speed_deg_per_sec = 90.0f,  // 90 graus por segundo
        .step_size = 3               // Passo de 3 graus
    };

    while(1) {
        //---move para 180° com velocidade controlada---
        move_servo_smoothly(SERVO2_CHANNEL, 180.0f, &speed_cfg);
        
        //---lê e exibe a posição---
        if(iot_servo_read_angle(LEDC_LOW_SPEED_MODE, SERVO2_CHANNEL, &current_angle) == ESP_OK) {
            ESP_LOGI(TAG, "2️⃣  Posição: %.1f° | Velocidade: %.1f°/s", 
                    current_angle, speed_cfg.speed_deg_per_sec);
        }
        
        //---move para 0° com velocidade controlada---
        move_servo_smoothly(SERVO2_CHANNEL, 0.0f, &speed_cfg);
        
        //---lê e exibe a posição---
        if(iot_servo_read_angle(LEDC_LOW_SPEED_MODE, SERVO2_CHANNEL, &current_angle) == ESP_OK) {
            ESP_LOGI(TAG, "2️⃣  Posição: %.1f° | Velocidade: %.1f°/s", 
                    current_angle, speed_cfg.speed_deg_per_sec);
        }
        
        //---aumenta a velocidade para o próximo ciclo (até 270°/s)---
        speed_cfg.speed_deg_per_sec += 45.0f;
        if(speed_cfg.speed_deg_per_sec > 270.0f) {
            speed_cfg.speed_deg_per_sec = 45.0f;
        }
    }
}

// ========================================================================================================
/**
 * @brief Move o servo de forma suave com velocidade controlada e verificação de limites
 * @param channel Canal do servo a ser controlado
 * @param target_angle Ângulo alvo em graus (será limitado entre 0 e max_angle)
 * @param speed_cfg Ponteiro para configuração de velocidade
 */
static void move_servo_smoothly(uint8_t channel, float target_angle, const servo_speed_t *speed_cfg) {
    float current_angle;
    
    //---verifica e corrige o ângulo alvo---
    float max_angle = servo_cfg.max_angle;
    target_angle = target_angle < 0 ? 0 : (target_angle > max_angle ? max_angle : target_angle);
    
    //---lê a posição atual---
    esp_err_t ret = iot_servo_read_angle(LEDC_LOW_SPEED_MODE, channel, &current_angle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao ler ângulo do servo");
        return;
    }
    
    float angle_diff = target_angle - current_angle;
    if (fabs(angle_diff) < 0.5f) return;  // Diferença insignificante
    
    //---calcula o tempo total necessário em milissegundos---
    float total_time_ms = (fabs(angle_diff) / speed_cfg->speed_deg_per_sec) * 1000;
    int steps = (int)(fabs(angle_diff) / speed_cfg->step_size);
    steps = steps == 0 ? 1 : steps;  // Garante pelo menos 1 passo
    
    float increment = angle_diff / steps;
    int delay_ms = total_time_ms / steps;
    
    //---executa o movimento em passos---
    for (int i = 0; i < steps; i++) {
        current_angle += increment;
        
        //---garante que não ultrapasse os limites---
        current_angle = current_angle < 0 ? 0 : (current_angle > max_angle ? max_angle : current_angle);
        
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, channel, current_angle);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    
    //---garante posição final exata (dentro dos limites)---
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, channel, target_angle);
}