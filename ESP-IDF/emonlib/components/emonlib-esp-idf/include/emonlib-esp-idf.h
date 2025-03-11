/*
 * NOME: Adenilton Ribeiro
 * DATA: 11/03/2025
 * PROJETO: emonlib-esp-idf.h
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para ler tensao e corrente da Emonlib.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS: Link de referencia - https://github.com/uktechbr/emonlib-esp-idf/tree/main
*/

// ========================================================================================================
/**
 * @brief emonlib-esp-idf.h
 * 
 */
#ifndef __EMONLIB_ESP_IDF_H__
#define __EMONLIB_ESP_IDF_H__

// ========================================================================================================
// ---BIBLIOTECA---

#include "esp_err.h"
#include <math.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

// ========================================================================================================
//---MACROS---

// ========================================================================================================
//---CONSTANTS---

#define NUM_SENSORS 2  // Número de sensores de corrente
#define READVCC_CALIBRATION_CONST 1126400L
#define ADC_BITS    12
#define ADC_COUNTS  (1<<ADC_BITS)

// ========================================================================================================
// ---ESTRUTURAS DE DADOS---

typedef struct {
    double Vrms;
    double Irms[NUM_SENSORS];          // Array para armazenar a corrente de cada sensor
    double realPower[NUM_SENSORS];     // Array para armazenar a potência real de cada sensor
    double apparentPower[NUM_SENSORS]; // Array para armazenar a potência aparente de cada sensor
    double powerFactor[NUM_SENSORS];   // Array para armazenar o fator de potência de cada sensor
    int64_t time;
} emonlib_esp_idf_data_t;

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

unsigned long millisec();
esp_err_t emonlib_init(double _VCAL, double _PHASECAL, double _ICAL);
esp_err_t emonlib_calc_vi(unsigned int crossings, unsigned int timeout, emonlib_esp_idf_data_t *data);

#endif // emonlib-esp-idf