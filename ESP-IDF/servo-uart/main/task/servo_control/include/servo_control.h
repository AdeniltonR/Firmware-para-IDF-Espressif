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
/**
 * @brief servo_control.h
 * 
 */
#ifndef  SERVO_CONTROL_H
#define  SERVO_CONTROL_H

// ========================================================================================================
// ---BIBLIOTECA---

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "servo_motor.h"
#include "uart_control.h"

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

//---parâmetros de velocidade---
typedef struct {
    float speed_deg_per_sec;  // Velocidade em graus por segundo
    int step_size;            // Tamanho do passo em graus
} servo_speed_t;

//---comunicação entre as tarefas---
extern QueueHandle_t xQueueServo1;
extern QueueHandle_t xQueueServo2;

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void servo_init_control(void);
void servo1_task(void *pvParameter);
void servo2_task(void *pvParameter);
void move_servo_smoothly(uint8_t channel, float target_angle, const servo_speed_t *speed_cfg);

#endif //servo_control.h
