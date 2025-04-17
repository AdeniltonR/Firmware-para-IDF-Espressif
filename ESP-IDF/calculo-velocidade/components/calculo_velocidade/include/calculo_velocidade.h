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
/**
 * @brief calculo_velocidade.h
 * 
 */
#ifndef CALCULO_VELOCIDADE_H
#define CALCULO_VELOCIDADE_H

// ========================================================================================================
//---BIBLIOTECAS---

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "esp_log.h"

// ========================================================================================================
//---PROTÓTIPOS DE FUNÇÕES---

//---inicialização---
void velocity_init(float pulley_diameter_mm, uint32_t pulses_per_revolution);
//---contagem de pulsos---
void IRAM_ATTR velocity_register_pulse(void);
uint32_t velocity_get_raw_pulses(void);
void velocity_reset_pulses(void);
//---cálculo de velocidade---
float velocity_calculate_mps(uint32_t update_interval_ms);
float velocity_calculate_kmph(uint32_t update_interval_ms);
float velocity_calculate_mpm(uint32_t update_interval_ms);
float velocity_get_smoothed_mps(uint32_t window_size);
bool velocity_is_stopped(float threshold);
//---cálculo de distância---
float velocity_get_pulley_circumference_mm(void);
float velocity_get_mm_per_pulse(void);
float velocity_get_distance_meters(void);
float velocity_get_distance_centimeters(void);
float velocity_get_distance_millimeters(void);
//---revoluções e tempo---
uint32_t velocity_get_revolutions(void);
uint32_t velocity_get_operating_time_seconds(void);
//---controle---
void velocity_reset_all(void);

#endif //calculo_velocidade.h