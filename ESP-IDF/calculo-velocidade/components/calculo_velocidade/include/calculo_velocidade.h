#ifndef CALCULO_VELOCIDADE_H
#define CALCULO_VELOCIDADE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>  // Adicione esta linha
#include <sys/types.h>  // Para size_t se necessário
#include "freertos/task.h"
#include <math.h>
#include "esp_log.h"


// ================= CONFIGURAÇÃO =================
void velocity_init(float pulley_diameter_mm, uint32_t pulses_per_revolution);
void IRAM_ATTR velocity_register_pulse(void);

// ================= LEITURA CRUA =================
uint32_t velocity_get_raw_pulses();
void velocity_reset_pulses();

// ================= VELOCIDADE =================
float velocity_calculate_mps(uint32_t update_interval_ms);
float velocity_calculate_cmps(uint32_t update_interval_ms);
float velocity_get_smoothed_mps(uint32_t window_size);
bool velocity_is_stopped(float threshold);

// ================= DISTÂNCIA =================
float distance_get_meters();
float distance_get_centimeters();
float distance_get_millimeters();

// ================= REVOLUÇÕES =================
uint32_t velocity_get_revolutions();

// ================= TEMPO =================
uint32_t velocity_get_operating_time_seconds();

// ================= CONTROLE =================
void velocity_reset_all();

#endif // VELOCITY_CALCULATOR_H