/*
 * NOME: Adenilton Ribeiro
 * DATA: 03/04/2025
 * PROJETO: Servo Control
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Controle dos servos.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: - https://docs.espressif.com/projects/esp-iot-solution/en/latest/motor/servo.html
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include "servo_control.h"

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

//---definição dos pinos e canais---
#define SERVO1_PIN      GPIO_NUM_20
#define SERVO1_CHANNEL  LEDC_CHANNEL_0
#define SERVO2_PIN      GPIO_NUM_2
#define SERVO2_CHANNEL  LEDC_CHANNEL_1

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (servo-control)
static const char *TAG = "servo-control";

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

//---comunicação entre as tarefas---
QueueHandle_t xQueueServo1 = NULL;
QueueHandle_t xQueueServo2 = NULL;

// ========================================================================================================
/**
 * @brief Inicializa dos servos
 * 
 */
void servo_init_control(void) {
    //---inicializa a biblioteca de servos---
    esp_err_t ret = iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Falha ao inicializar servos: %d", ret);
        return;
    }
    ESP_LOGI(TAG, "✅ Controle de servos iniciado!");

    //---cria as filas para os ângulos dos servos (tamanho: 1 elemento)---
    xQueueServo1 = xQueueCreate(1, sizeof(float));
    xQueueServo2 = xQueueCreate(1, sizeof(float));

    ESP_LOGI(TAG, "✅ Filas de controle dos servos criadas!");
}

// ========================================================================================================
/**
 * @brief Tarefa para controle contínuo do Servo 1
 * 
 * @param pvParameter Parâmetro genérico do FreeRTOS (não utilizado nesta implementação)
 * 
 * @note Esta tarefa move o Servo 1 em um movimento de varredura entre 0° e 180° com incrementos de x°,
 *       lê e exibe a posição atual em ambos os limites do movimento.
 *       A velocidade é controlada por um delay de 50ms entre cada movimento.
 */
void servo1_task(void *pvParameter) {
    float target_angle = 0.0f;          // Ângulo neutro inicial
    float current_angle;
    float last_reported_angle = -1.0f;  // Valor inválido inicial
    servo_speed_t speed_cfg = {
        .speed_deg_per_sec = 60.0f,     // Velocidade média
        .step_size = 3                  // Incremento suave
    };

    while(1) {
        //---verifica se há novo ângulo na fila (com timeout de 50ms)---
        if (xQueueReceive(xQueueServo1, &target_angle, pdMS_TO_TICKS(50))) {
            ESP_LOGI(TAG, "🔄 Comando recebido - Servo 1 → %.1f°", target_angle);
            
            //---limita o ângulo entre 0-180°---
            target_angle = (target_angle < 0) ? 0 : (target_angle > 180) ? 180 : target_angle;
            
            //---move para o ângulo alvo---
            move_servo_smoothly(SERVO1_CHANNEL, target_angle, &speed_cfg);
            
            //---lê e exibe a posição atual apenas após movimento significativo---
            if (iot_servo_read_angle(LEDC_LOW_SPEED_MODE, SERVO1_CHANNEL, &current_angle) == ESP_OK) {
                if (fabs(current_angle - last_reported_angle) > 1.0f) {  // Só mostra se mudou significativamente
                    ESP_LOGI(TAG, "1️⃣  Posição atual: %.1f°", current_angle);
                    last_reported_angle = current_angle;
                }
            }
            //---enviando comandos---
            enviar_comandos_uart(2);
        }
        
        vTaskDelay(50 / portTICK_PERIOD_MS); 
    }
}

// ========================================================================================================
/**
 * @brief Tarefa para controle contínuo do Servo 2
 * 
 * @param pvParameter Parâmetro genérico do FreeRTOS (não utilizado nesta implementação)
 * 
 * @note Esta tarefa move o Servo 1 em um movimento de varredura entre 0° e 180° com incrementos de x°,
 *       lê e exibe a posição atual em ambos os limites do movimento.
 *       A velocidade é controlada por um delay de 50ms entre cada movimento.
 */
void servo2_task(void *pvParameter) {
    float target_angle = 0.0f;          // Ângulo padrão inicial
    float current_angle; 
    float last_reported_angle = -1.0f;  // Valor inválido inicial
    servo_speed_t speed_cfg = {
        .speed_deg_per_sec = 60.0f,     // Velocidade média
        .step_size = 3                  // Incremento suave
    };

    while(1) {
        //---verifica se há novo ângulo na fila (com timeout de 50ms)---
        if (xQueueReceive(xQueueServo2, &target_angle, pdMS_TO_TICKS(100))) {
            ESP_LOGI(TAG, "🔄 Comando recebido - Servo 2 → %.1f°", target_angle);
            
            //---limita o ângulo entre 0-180°---
            target_angle = (target_angle < 0) ? 0 : (target_angle > 180) ? 180 : target_angle;
            
            //---move para o ângulo alvo---
            move_servo_smoothly(SERVO2_CHANNEL, target_angle, &speed_cfg);
            
            //---lê e exibe a posição atual apenas após movimento---
            if (iot_servo_read_angle(LEDC_LOW_SPEED_MODE, SERVO2_CHANNEL, &current_angle) == ESP_OK) {
                if (fabs(current_angle - last_reported_angle) > 1.0f) {  // Só mostra se mudou significativamente
                    ESP_LOGI(TAG, "2️⃣  Posição atual: %.1f°", current_angle);
                    last_reported_angle = current_angle;
                }
            }
            //---enviando comandos---
            enviar_comandos_uart(3);
        }
        
        vTaskDelay(50 / portTICK_PERIOD_MS);  
    }
}

// ========================================================================================================
/**
 * @brief Move o servo de forma suave com velocidade controlada e verificação de limites
 * @param channel Canal do servo a ser controlado
 * @param target_angle Ângulo alvo em graus (será limitado entre 0 e max_angle)
 * @param speed_cfg Ponteiro para configuração de velocidade
 */
void move_servo_smoothly(uint8_t channel, float target_angle, const servo_speed_t *speed_cfg) {
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