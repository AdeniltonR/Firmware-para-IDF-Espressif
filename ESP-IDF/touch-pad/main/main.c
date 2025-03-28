/*
 * NOME: Adenilton Ribeiro
 * DATA: 19/03/2025
 * PROJETO: Touch Pad
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Configuração de dois pinos Touch Pad usando RTOS.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include "driver/touch_pad.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define TOUCH_PAD_1 TOUCH_PAD_NUM4  // Touch Pad 4 (GPIO_NUM_13)
#define TOUCH_PAD_2 TOUCH_PAD_NUM7  // Touch Pad 7 (GPIO_NUM_27)

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (touch-pad)
static const char *TAG = "touch-pad";

//---touch dependo do ambiente fica entre o valor 1000 a 1200, clicando nele abaixa para 130 a 185 então ajuste conforme o projeto---
int touch_min = 100;
int touch_max = 200;

//---estados dos botões---
bool button_1_pressed = false; // Estado do botão 1 (false = não tocado, true = tocado)
bool button_2_pressed = false; // Estado do botão 2 (false = não tocado, true = tocado)

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void touch_pad_task(void *pvParameter);

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main() {
    //---inicializa o controlador de touch pad---
    esp_err_t ret = touch_pad_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Falha ao inicializar touch pad: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "✅ Touch pad inicializado com sucesso");

    //---configura a voltagem dos touch pads---
    ret = touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Falha ao configurar voltagem: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "✅ Voltagem configurada: HV=2.7V, LV=0.5V");

    //---configura os touch pads---
    ret = touch_pad_config(TOUCH_PAD_1, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Falha ao configurar Touch Pad 1: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "✅ Touch Pad 1 (GPIO%d) configurado", TOUCH_PAD_NUM4);

    ret = touch_pad_config(TOUCH_PAD_2, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Falha ao configurar Touch Pad 2: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "✅ Touch Pad 2 (GPIO%d) configurado", TOUCH_PAD_NUM7);

    ESP_LOGI(TAG, "✅ Touch pads inicializados e prontos para uso");

    //---cria a tarefa para leitura dos touch pads---
    xTaskCreate(touch_pad_task, "Touch_Pad_Task", 2048, NULL, 1, NULL);

    //---a tarefa principal não faz nada, pois a tarefa do RTOS está ativa---
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ========================================================================================================
/**
 * @brief Tarefa para ler e verificar os valores dos dois Touch Pads.
 *
 * @note
 * Esta tarefa lê os valores dos Touch Pads configurados (TOUCH_PAD_1 e TOUCH_PAD_2) e verifica
 * se os valores estão dentro de um intervalo específico (definido por `touch_min` e `touch_max`).
 * Se o valor estiver dentro do intervalo, uma mensagem de log é exibida indicando que o botão foi tocado.
 *
 * @param pvParameter Parâmetro passado para a tarefa (não utilizado nesta função).
 *
 * @note
 * - A função usa `touch_pad_read` para ler os valores dos Touch Pads.
 * - O intervalo de detecção de toque é definido por `touch_min` e `touch_max`.
 * - A tarefa é executada em um loop infinito com um atraso de 100ms entre as leituras.
 * - Em caso de erro na leitura, uma mensagem de erro é exibida no log.
 */
void touch_pad_task(void *pvParameter) {
    while (1) {
        uint16_t touch_value_1, touch_value_2;
        esp_err_t ret1 = touch_pad_read(TOUCH_PAD_1, &touch_value_1);
        esp_err_t ret2 = touch_pad_read(TOUCH_PAD_2, &touch_value_2);

        if (ret1 == ESP_OK) {
            //---verifica se o valor do Touch Pad 1 está entre touch_min e touch_max---
            if (touch_value_1 > touch_min && touch_value_1 < touch_max) {
                if (!button_1_pressed) {
                    //---botão 1 foi tocado---
                    ESP_LOGI(TAG, "👆 Botão 1 tocado! Valor: %d", touch_value_1);
                    button_1_pressed = true; // Atualiza o estado do botão
                }
            } else {
                //---botão 1 não está mais tocado---
                button_1_pressed = false; // Atualiza o estado do botão
            }
        } else {
            ESP_LOGE(TAG, "❌ Erro ao ler Touch Pad 1: %s", esp_err_to_name(ret1));
        }

        if (ret2 == ESP_OK) {
            //---verifica se o valor do Touch Pad 2 está entre touch_min e touch_max---
            if (touch_value_2 > touch_min && touch_value_2 < touch_max) {
                if (!button_2_pressed) {
                    //---botão 2 foi tocado---
                    ESP_LOGI(TAG, "👆 Botão 2 tocado! Valor: %d", touch_value_2);
                    button_2_pressed = true; // Atualiza o estado do botão
                }
            } else {
                //---botão 2 não está mais tocado---
                button_2_pressed = false; // Atualiza o estado do botão
            }
        } else {
            ESP_LOGE(TAG, "❌ Erro ao ler Touch Pad 2: %s", esp_err_to_name(ret2));
        }

        //---aguarda 100ms antes da próxima leitura---
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}