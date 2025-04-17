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

#include "calculo_velocidade.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

#define MAX_SAMPLES 10
#define PI 3.14159265358979323846f

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (calculo-velocidade)
static const char* TAG = "calculo-velocidade";

static volatile uint32_t total_pulses = 0;         // Contador total de pulsos do encoder
static uint32_t pulses_per_rev = 1;                // Pulsos por volta completa do encoder
static float pulley_circumference_m = 0.0f;        // Circunferência da polia em metros
static TickType_t start_time = 0;                  // Tempo de inicialização do sistema
static float velocity_history[MAX_SAMPLES] = {0};  // Histórico de velocidades para cálculo de média
static uint8_t history_index = 0;                  // Índice atual do histórico circular

// ========================================================================================================
/**
 * @brief Calcula a velocidade instantânea baseada em pulsos e intervalo de tempo
 * @param delta_pulses Número de pulsos detectados no intervalo
 * @param delta_time_s Intervalo de tempo em segundos
 * @return Velocidade instantânea em metros por segundo
 */
static float calculate_instant_velocity(uint32_t delta_pulses, float delta_time_s) {
    if(delta_time_s <= 0.0f) return 0.0f;
    float revolutions = (float)delta_pulses / (float)pulses_per_rev;
    return (revolutions * pulley_circumference_m) / delta_time_s;
}

// ========================================================================================================
/**
 * @brief Inicializa o módulo de cálculo de velocidade
 * @param pulley_diameter_mm Diâmetro da polia em milímetros
 * @param pulses_per_revolution Número de pulsos do encoder por volta completa
 */
void velocity_init(float pulley_diameter_mm, uint32_t pulses_per_revolution) {
    pulley_circumference_m = PI * (pulley_diameter_mm / 1000.0f);
    pulses_per_rev = pulses_per_revolution;
    velocity_reset_all();
    ESP_LOGI(TAG, "✅ Config: Diametro=%.1fmm, Pulsos/rev=%"PRIu32, pulley_diameter_mm, pulses_per_rev);
}

// ========================================================================================================
/**
 * @brief Rotina de serviço de interrupção para registrar pulsos do encoder
 * @note Deve ser chamada pela ISR do GPIO, marcada para alocação na IRAM
 */
void IRAM_ATTR velocity_register_pulse(void) {
    total_pulses++;
}

// ========================================================================================================
/**
 * @brief Obtém a contagem total de pulsos do encoder
 * @return Número total de pulsos registrados desde o último reset
 */
uint32_t velocity_get_raw_pulses(void) {
    return total_pulses;
}

// ========================================================================================================
/**
 * @brief Reinicia o contador de pulsos
 */
void velocity_reset_pulses(void) {
    total_pulses = 0;
}

// ========================================================================================================
/**
 * @brief Calcula a velocidade atual em metros por segundo
 * @param update_interval_ms Intervalo de tempo para cálculo (não utilizado atualmente)
 * @return Velocidade atual em m/s
 */
float velocity_calculate_mps(uint32_t update_interval_ms) {
    static uint32_t last_pulses = 0;
    static TickType_t last_time = 0;
    
    uint32_t current_pulses = total_pulses;
    TickType_t current_time = xTaskGetTickCount();
    
    float delta_time_s = (float)(current_time - last_time) * portTICK_PERIOD_MS / 1000.0f;
    uint32_t delta_pulses = current_pulses - last_pulses;
    
    float velocity = calculate_instant_velocity(delta_pulses, delta_time_s);
    
    velocity_history[history_index] = velocity;
    history_index = (history_index + 1) % MAX_SAMPLES;
    
    last_pulses = current_pulses;
    last_time = current_time;
    
    return velocity;
}

// ========================================================================================================
/**
 * @brief Calcula a velocidade atual em quilômetros por hora
 * @param update_interval_ms Intervalo de tempo para cálculo
 * @return Velocidade atual em km/h
 */
float velocity_calculate_kmph(uint32_t update_interval_ms) {
    return velocity_calculate_mps(update_interval_ms) * 3.6f;
}

// ========================================================================================================
/**
 * @brief Calcula a velocidade atual em metros por minuto
 * @param update_interval_ms Intervalo de tempo para cálculo
 * @return Velocidade atual em m/min
 */
float velocity_calculate_mpm(uint32_t update_interval_ms) {
    // Converte m/s para m/min (multiplicando por 60)
    return velocity_calculate_mps(update_interval_ms) * 60.0f;
}

// ========================================================================================================
/**
 * @brief Calcula a velocidade suavizada usando média móvel
 * @param window_size Número de amostras para média (máximo MAX_SAMPLES)
 * @return Velocidade suavizada em m/s
 */
float velocity_get_smoothed_mps(uint32_t window_size) {
    if(window_size > MAX_SAMPLES) window_size = MAX_SAMPLES;
    
    float sum = 0.0f;
    uint8_t count = 0;
    
    for(uint8_t i = 0; i < window_size; i++) {
        if(velocity_history[i] > 0.0f) {
            sum += velocity_history[i];
            count++;
        }
    }
    return (count > 0) ? (sum / count) : 0.0f;
}

// ========================================================================================================
/**
 * @brief Verifica se o sistema está parado
 * @param threshold Limite de velocidade abaixo do qual é considerado parado (m/s)
 * @return true se velocidade abaixo do limite, false caso contrário
 */
bool velocity_is_stopped(float threshold) {
    return (velocity_get_smoothed_mps(3) < threshold);
}

// ========================================================================================================
/**
 * @brief Obtém a circunferência da polia
 * @return Circunferência da polia em milímetros
 */
float velocity_get_pulley_circumference_mm(void) {
    return pulley_circumference_m * 1000.0f;
}

// ========================================================================================================
/**
 * @brief Calcula o deslocamento linear por pulso do encoder
 * @return Milímetros percorridos por pulso
 */
float velocity_get_mm_per_pulse(void) {
    if(pulses_per_rev == 0) return 0.0f;
    return velocity_get_pulley_circumference_mm() / pulses_per_rev;
}

// ========================================================================================================
/**
 * @brief Calcula a distância total percorrida
 * @return Distância em metros
 */
float velocity_get_distance_meters(void) {
    return (total_pulses * velocity_get_mm_per_pulse()) / 1000.0f;
}

// ========================================================================================================
/**
 * @brief Calcula a distância total percorrida
 * @return Distância em centímetros
 */
float velocity_get_distance_centimeters(void) {
    return (total_pulses * velocity_get_mm_per_pulse()) / 10.0f;
}

// ========================================================================================================
/**
 * @brief Calcula a distância total percorrida
 * @return Distância em milímetros
 */
float velocity_get_distance_millimeters(void) {
    return total_pulses * velocity_get_mm_per_pulse();
}

// ========================================================================================================
/**
 * @brief Obtém o número total de voltas completas
 * @return Número de voltas completas
 */
uint32_t velocity_get_revolutions(void) {
    if(pulses_per_rev == 0) return 0;
    return total_pulses / pulses_per_rev;
}

// ========================================================================================================
/**
 * @brief Obtém o tempo de operação do sistema
 * @return Tempo desde o último reset em segundos
 */
uint32_t velocity_get_operating_time_seconds(void) {
    return (xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS / 1000;
}

// ========================================================================================================
/**
 * @brief Reinicia todos os contadores e histórico do módulo
 */
void velocity_reset_all(void) {
    total_pulses = 0;
    start_time = xTaskGetTickCount();
    for(int i = 0; i < MAX_SAMPLES; i++) {
        velocity_history[i] = 0.0f;
    }
}