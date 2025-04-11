#include "calculo_velocidade.h"

#define MAX_SAMPLES 10 // Tamanho máximo para média móvel

static const char* TAG = "velocity";

// Variáveis de estado
static volatile uint32_t total_pulses = 0;
static uint32_t pulses_per_rev = 1;
static float pulley_circumference_m = 0.0f;
static TickType_t start_time = 0;
static float velocity_history[MAX_SAMPLES] = {0};
static uint8_t history_index = 0;

// Funções privadas
static float calculate_instant_velocity(uint32_t delta_pulses, float delta_time_s);

void velocity_init(float pulley_diameter_mm, uint32_t pulses_per_revolution) {
    pulley_circumference_m = M_PI * (pulley_diameter_mm / 1000.0f);
    pulses_per_rev = pulses_per_revolution;
    velocity_reset_all();
    
    ESP_LOGI(TAG, "Config: Diameter=%.1fmm, Pulses/rev=%d", 
             pulley_diameter_mm, pulses_per_revolution);
}

// Adicione esta implementação
void IRAM_ATTR velocity_register_pulse(void) {
    total_pulses++;
}

// ================= LEITURA CRUA =================
uint32_t velocity_get_raw_pulses() {
    return total_pulses;
}

void velocity_reset_pulses() {
    total_pulses = 0;
}

// ================= CÁLCULO DE VELOCIDADE =================
float velocity_calculate_mps(uint32_t update_interval_ms) {
    static uint32_t last_pulses = 0;
    static TickType_t last_time = 0;
    
    uint32_t current_pulses = total_pulses;
    TickType_t current_time = xTaskGetTickCount();
    
    float delta_time_s = (float)(current_time - last_time) * portTICK_PERIOD_MS / 1000.0f;
    uint32_t delta_pulses = current_pulses - last_pulses;
    
    float velocity = calculate_instant_velocity(delta_pulses, delta_time_s);
    
    // Atualiza histórico para média móvel
    velocity_history[history_index] = velocity;
    history_index = (history_index + 1) % MAX_SAMPLES;
    
    last_pulses = current_pulses;
    last_time = current_time;
    
    return velocity;
}

float velocity_calculate_cmps(uint32_t update_interval_ms) {
    return velocity_calculate_mps(update_interval_ms) * 100.0f;
}

float velocity_get_smoothed_mps(uint32_t window_size) {
    if(window_size > MAX_SAMPLES) window_size = MAX_SAMPLES;
    
    float sum = 0.0f;
    uint8_t count = 0;
    
    for(uint8_t i = 0; i < window_size; i++) {
        if(velocity_history[i] > 0.0f) { // Ignora valores não inicializados
            sum += velocity_history[i];
            count++;
        }
    }
    
    return (count > 0) ? (sum / count) : 0.0f;
}

bool velocity_is_stopped(float threshold) {
    return (velocity_get_smoothed_mps(3) < threshold); // Usa média de 3 amostras
}

// ================= DISTÂNCIA =================
float distance_get_meters() {
    float revolutions = (float)total_pulses / (float)pulses_per_rev;
    return revolutions * pulley_circumference_m;
}

float distance_get_centimeters() {
    return distance_get_meters() * 100.0f;
}

float distance_get_millimeters() {
    return distance_get_meters() * 1000.0f;
}

// ================= REVOLUÇÕES =================
uint32_t velocity_get_revolutions() {
    return total_pulses / pulses_per_rev;
}

// ================= TEMPO =================
uint32_t velocity_get_operating_time_seconds() {
    return (xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS / 1000;
}

// ================= CONTROLE =================
void velocity_reset_all() {
    total_pulses = 0;
    start_time = xTaskGetTickCount();
    for(int i = 0; i < MAX_SAMPLES; i++) {
        velocity_history[i] = 0.0f;
    }
}

// ================= INTERRUPÇÃO =================
void IRAM_ATTR velocity_register_pulse() {
    total_pulses++;
}

// ================= FUNÇÃO PRIVADA =================
static float calculate_instant_velocity(uint32_t delta_pulses, float delta_time_s) {
    if(delta_time_s <= 0.0f) return 0.0f;
    
    float revolutions = (float)delta_pulses / (float)pulses_per_rev;
    return (revolutions * pulley_circumference_m) / delta_time_s;
}