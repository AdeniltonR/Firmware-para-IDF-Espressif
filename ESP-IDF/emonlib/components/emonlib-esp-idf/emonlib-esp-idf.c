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
// ---BIBLIOTECA---

#include "emonlib-esp-idf.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

#define USE_VOLTAGE_SENSOR 1  // 0 para desabilitar, 1 para habilitar

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

//---defina os canais ADC---
#define PIN_tensao     ADC_CHANNEL_0  // GPIO 36
#define PIN_corrente_1 ADC_CHANNEL_6  // GPIO 34
#define PIN_corrente_2 ADC_CHANNEL_7  // GPIO 35

// ========================================================================================================
//---VARIAVEIS GLOBAIS---
static const char *TAG = "emonlib";

//---calibration coefficients---
double VCAL, ICAL, PHASECAL;

int current_channels[NUM_SENSORS] = {
    PIN_corrente_1,
    PIN_corrente_2
};

int sampleV, sampleI[NUM_SENSORS];

double lastFilteredV, filteredV;
double filteredI[NUM_SENSORS];
double offsetV;
double offsetI[NUM_SENSORS];

double phaseShiftedV;

double sqV, sumV, sqI[NUM_SENSORS], sumI[NUM_SENSORS], instP[NUM_SENSORS], sumP[NUM_SENSORS];

int startV;

bool lastVCross, checkVCross;

adc_oneshot_unit_handle_t adc_handle;

#define SUPPLY_VOLTAGE 3300  // 3.3V em mV

// ========================================================================================================
/**
 * @brief Obtém o tempo atual em milissegundos.
 * 
 * @return O tempo em milissegundos.
 */
unsigned long millisec() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

// ========================================================================================================
/**
 * @brief Inicializa a biblioteca de medição de tensão e corrente.
 * 
 * @param _VCAL Coeficiente de calibração de tensão.
 * @param _PHASECAL Coeficiente de calibração de fase.
 * @param _ICAL Coeficiente de calibração de corrente.
 * 
 * @return ESP_OK em caso de sucesso, ou erro caso contrário.
 */
esp_err_t emonlib_init(double _VCAL, double _PHASECAL, double _ICAL) {
    ESP_LOGD(TAG, "Calibration values: VCal = %f; PhaseCal = %f; ICal = %f", _VCAL, _PHASECAL, _ICAL);
    VCAL = _VCAL;
    PHASECAL = _PHASECAL;
    offsetV = ADC_COUNTS >> 1;
    ICAL = _ICAL;
    for (int i = 0; i < NUM_SENSORS; i++) {
        offsetI[i] = ADC_COUNTS >> 1;
    }

    //---configuração do ADC com a nova API---
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,  // Usar ADC_UNIT_1 para GPIOs 32-39
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    //---configuração do canal de tensão---
    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_12,  // Resolução de 12 bits
        .atten = ADC_ATTEN_DB_12,     // Atenuação de 12 dB (para leitura de até 3.3V)
    };
    //---configuração do canal de tensão---
    if (USE_VOLTAGE_SENSOR) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, PIN_tensao, &channel_config));
    }

    //---configuração do canal de corrente---
    for (int i = 0; i < NUM_SENSORS; i++) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, current_channels[i], &channel_config));
    }

    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Calcula os valores de tensão, corrente e potência com base nas amostras coletadas.
 * 
 * @param crossings Número de cruzamentos por zero a serem detectados.
 * @param timeout Tempo limite (em milissegundos) para a medição.
 * @param data Ponteiro para a estrutura que armazenará os resultados da medição.
 * 
 * @return ESP_OK em caso de sucesso, ou erro caso contrário.
 */
esp_err_t emonlib_calc_vi(unsigned int crossings, unsigned int timeout, emonlib_esp_idf_data_t *data) {
    int SupplyVoltage = SUPPLY_VOLTAGE;
    unsigned int crossCount = 0;      // Usado para contar o número de cruzamentos por zero
    unsigned int numberOfSamples = 0; // Contador de amostras

    //-------------------------------------------------------------------------------------------------------------------------
    // 1) Aguarda a forma de onda estar próxima de 'zero' (parte central da curva senoidal)
    //-------------------------------------------------------------------------------------------------------------------------
    unsigned long start = millisec(); // Garante que não fique preso no loop em caso de erro

    if (USE_VOLTAGE_SENSOR) {
        while (1) {
            int raw_value;
            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, PIN_tensao, &raw_value));
            startV = raw_value; // Usando a forma de onda de tensão
            if ((startV < (ADC_COUNTS * 0.55)) && (startV > (ADC_COUNTS * 0.45)))
                break; // Verifica se está dentro da faixa
            if ((millisec() - start) > timeout)
                break;
        }
    }

    //-------------------------------------------------------------------------------------------------------------------------
    // 2) Loop principal de medição
    //-------------------------------------------------------------------------------------------------------------------------
    start = millisec();

    while ((crossCount < crossings) && ((millisec() - start) < timeout)) {
        numberOfSamples++;         // Conta o número de amostras
        lastFilteredV = filteredV; // Usado para compensação de fase

        //-----------------------------------------------------------------------------
        // A) Leitura das amostras de tensão e corrente
        //-----------------------------------------------------------------------------
        int raw_value;
        if (USE_VOLTAGE_SENSOR) {
            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, PIN_tensao, &raw_value));
            sampleV = raw_value; // Leitura da tensão
        }

        for (int i = 0; i < NUM_SENSORS; i++) {
            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, PIN_corrente_1 + i, &raw_value));
            sampleI[i] = raw_value; // Leitura da corrente para cada sensor

            //-----------------------------------------------------------------------------
            // B) Aplicação de filtros digitais passa-baixa
            //-----------------------------------------------------------------------------
            offsetI[i] = offsetI[i] + ((sampleI[i] - offsetI[i]) / ADC_COUNTS);
            filteredI[i] = sampleI[i] - offsetI[i];

            //-----------------------------------------------------------------------------
            // D) Método RMS para corrente
            //-----------------------------------------------------------------------------
            sqI[i] = filteredI[i] * filteredI[i];
            sumI[i] += sqI[i];
        }

        if (USE_VOLTAGE_SENSOR) {
            //-----------------------------------------------------------------------------
            // B) Aplicação de filtros digitais passa-baixa
            //-----------------------------------------------------------------------------
            offsetV = offsetV + ((sampleV - offsetV) / ADC_COUNTS);
            filteredV = sampleV - offsetV;

            //-----------------------------------------------------------------------------
            // C) Método RMS para tensão
            //-----------------------------------------------------------------------------
            sqV = filteredV * filteredV; // 1) Quadrado dos valores de tensão
            sumV += sqV;                 // 2) Soma

            //-----------------------------------------------------------------------------
            // E) Calibração de fase
            //-----------------------------------------------------------------------------
            phaseShiftedV = lastFilteredV + PHASECAL * (filteredV - lastFilteredV);

            //-----------------------------------------------------------------------------
            // F) Cálculo da potência instantânea
            //-----------------------------------------------------------------------------
            for (int i = 0; i < NUM_SENSORS; i++) {
                instP[i] = phaseShiftedV * filteredI[i]; // Potência instantânea para cada sensor
                sumP[i] += instP[i];                     // Soma
            }
        }

        //-----------------------------------------------------------------------------
        // G) Contagem de cruzamentos por zero
        //-----------------------------------------------------------------------------
        if (USE_VOLTAGE_SENSOR) {
            lastVCross = checkVCross;
            if (sampleV > startV) {
                checkVCross = true;
            }else{
                checkVCross = false;
            }

            if (numberOfSamples == 1) {
                lastVCross = checkVCross;
            }

            if (lastVCross != checkVCross) {
                crossCount++;
            }
        }
    }

    //-------------------------------------------------------------------------------------------------------------------------
    // 3) Cálculos pós-loop
    //-------------------------------------------------------------------------------------------------------------------------
    double V_RATIO = 0.0;
    if (USE_VOLTAGE_SENSOR) {
        V_RATIO = VCAL * ((SupplyVoltage / 1000.0) / (ADC_COUNTS));
        data->Vrms = V_RATIO * sqrt(sumV / numberOfSamples);
    } else {
        data->Vrms = 0.0; // Se o sensor de tensão estiver desabilitado, defina Vrms como 0
    }

    for (int i = 0; i < NUM_SENSORS; i++) {
        double I_RATIO = ICAL * ((SupplyVoltage / 1000.0) / (ADC_COUNTS));
        data->Irms[i] = I_RATIO * sqrt(sumI[i] / numberOfSamples);

        if (USE_VOLTAGE_SENSOR) {
            //---cálculo dos valores de potência---
            data->realPower[i] = V_RATIO * I_RATIO * sumP[i] / numberOfSamples;
            data->apparentPower[i] = data->Vrms * data->Irms[i];
            data->powerFactor[i] = data->realPower[i] / data->apparentPower[i];
        }else{
            //---se o sensor de tensão estiver desabilitado, defina potência e fator de potência como 0---
            data->realPower[i] = 0.0;
            data->apparentPower[i] = 0.0;
            data->powerFactor[i] = 0.0;
        }
    }

    ESP_LOGD(TAG, "VRMS = %f; IRMS = [%f, %f, %f]; Real Power = [%f, %f, %f]; Apparent Power = [%f, %f, %f], Power Factor = [%f, %f, %f]",
        data->Vrms, data->Irms[0], data->Irms[1], data->Irms[2],
        data->realPower[0], data->realPower[1], data->realPower[2],
        data->apparentPower[0], data->apparentPower[1], data->apparentPower[2],
        data->powerFactor[0], data->powerFactor[1], data->powerFactor[2]);

    //---reinicializa os acumuladores---
    sumV = 0;
    for (int i = 0; i < NUM_SENSORS; i++) {
        sumI[i] = 0;
        sumP[i] = 0;
    }

    return ESP_OK;
}