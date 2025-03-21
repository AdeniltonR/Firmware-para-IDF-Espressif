/*
 * NOME: Adenilton Ribeiro
 * DATA: 20/03/2025
 * PROJETO: Led Strip
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Controle de LEDs RGB usando RMT, mas a biblioteca driver/rmt.h está obsoleta.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "driver/rmt_tx.h"
#include "driver/rmt.h"

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

typedef struct led_strip_s led_strip_t;
typedef void *led_strip_dev_t;

// ========================================================================================================
/**
 * @brief Declaração do Tipo de Tira de LEDs
 *
 */
struct led_strip_s {
    /**
     * @brief Define a cor RGB para um pixel específico
     *
     * @param strip: Tira de LEDs
     * @param index: Índice do pixel a ser configurado
     * @param red: Parte vermelha da cor
     * @param green: Parte verde da cor
     * @param blue: Parte azul da cor
     *
     * @return
     *      - ESP_OK: Configuração do RGB para o pixel específico foi bem-sucedida
     *      - ESP_ERR_INVALID_ARG: Configuração do RGB falhou devido a parâmetros inválidos
     *      - ESP_FAIL: Configuração do RGB falhou devido a outro erro
     */
    esp_err_t (*set_pixel)(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue);

    /**
     * @brief Atualiza as cores na memória para os LEDs
     *
     * @param strip: Tira de LEDs
     * @param timeout_ms: Valor de timeout para a tarefa de atualização
     *
     * @return
     *      - ESP_OK: Atualização bem-sucedida
     *      - ESP_ERR_TIMEOUT: Atualização falhou devido ao timeout
     *      - ESP_FAIL: Atualização falhou devido a outro erro
     *
     * @note:
     *      Após atualizar as cores dos LEDs na memória, é necessário chamar esta API para enviar as cores para a tira.
     */
    esp_err_t (*refresh)(led_strip_t *strip, uint32_t timeout_ms);

    /**
     * @brief Limpa a tira de LEDs (desliga todos os LEDs)
     *
     * @param strip: Tira de LEDs
     * @param timeout_ms: Valor de timeout para a tarefa de limpeza
     *
     * @return
     *      - ESP_OK: Limpeza dos LEDs foi bem-sucedida
     *      - ESP_ERR_TIMEOUT: Limpeza dos LEDs falhou devido ao timeout
     *      - ESP_FAIL: Limpeza dos LEDs falhou devido a outro erro
     */
    esp_err_t (*clear)(led_strip_t *strip, uint32_t timeout_ms);

    /**
     * @brief Libera os recursos da tira de LEDs
     *
     * @param strip: Tira de LEDs
     *
     * @return
     *      - ESP_OK: Liberação dos recursos foi bem-sucedida
     *      - ESP_FAIL: Liberação dos recursos falhou devido a um erro
     */
    esp_err_t (*del)(led_strip_t *strip);
};

// ========================================================================================================
/**
 * @brief Tipo de Configuração da Tira de LEDs
 *
 */
typedef struct {
    uint32_t max_leds;   /*!< Número máximo de LEDs em uma única tira */
    led_strip_dev_t dev; /*!< Dispositivo da tira de LEDs (ex: canal RMT, canal PWM, etc) */
} led_strip_config_t;

// ========================================================================================================
/**
 * @brief Configuração padrão para a tira de LEDs
 *
 */
#define LED_STRIP_DEFAULT_CONFIG(number, dev_hdl) \
    {                                             \
        .max_leds = number,                       \
        .dev = dev_hdl,                           \
    }

// ========================================================================================================
//---PROTÓTIPOS DE FUNÇÕES---

led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config);
led_strip_t * led_strip_init(uint8_t channel, uint8_t gpio, uint16_t led_num);
esp_err_t led_strip_denit(led_strip_t *strip);

#ifdef __cplusplus
}
#endif