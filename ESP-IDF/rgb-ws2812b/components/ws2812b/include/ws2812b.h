/*
* NOME: Adenilton
* DATA: 18/03/2025
* PROJETO: RGB WS2812B
* VERSAO: 1.0.0
* DESCRICAO: - feat: Controle de LEDs WS2812B via SPI.
*            - docs: ESP32-S3 - ESP-IDF v5.4.0
* LINKS: 
*/

// ========================================================================================================
/**
 * @brief ws2812b.h
 * 
 */
#ifndef __WS2812B_H__
#define __WS2812B_H__

// ========================================================================================================
//---BIBLIOTECAS---

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <stdio.h>
#include <string.h>

// ========================================================================================================
//---DEFINIÇÕES DE TIPOS---

// ========================================================================================================
/**
 * @brief Estrutura para representar uma cor RGB.
 * 
 * @note A estrutura permite acessar os componentes de cor (vermelho, verde, azul) de duas maneiras:
 *       - Através de campos nomeados (r, g, b ou red, green, blue).
 *       - Através de um array de bytes (raw[3]).
 *       - Também pode ser acessada como um número inteiro de 32 bits (num).
 */
typedef struct {
    union {
        struct {
            union {
                uint8_t r;      // Componente vermelho (8 bits)
                uint8_t red;    // Alternativa para o componente vermelho
            };

            union {
                uint8_t g;      // Componente verde (8 bits)
                uint8_t green;  // Alternativa para o componente verde
            };

            union {
                uint8_t b;      // Componente azul (8 bits)
                uint8_t blue;   // Alternativa para o componente azul
            };
        };

        uint8_t raw[3];         // Array de bytes para acessar os componentes de cor
        uint32_t num;           // Número inteiro de 32 bits para acessar a cor
    };
} CRGB;

// ========================================================================================================
/**
 * @brief Estrutura para configurações do SPI.
 * 
 * @note Contém as configurações necessárias para a comunicação SPI com os LEDs WS2812B.
 */
typedef struct {
    spi_host_device_t host;               // Host SPI (ex: SPI2_HOST)
    spi_device_handle_t spi;              // Handle do dispositivo SPI
    int dma_chan;                         // Canal DMA (ex: SPI_DMA_CH_AUTO)
    spi_device_interface_config_t devcfg; // Configuração do dispositivo SPI
    spi_bus_config_t buscfg;              // Configuração do barramento SPI
} spi_settings_t;

// ========================================================================================================
/**
 * @brief Enumeração para os modelos de tiras de LED suportados.
 * 
 * @note Atualmente suporta WS2812B e WS2815.
 */
typedef enum {
    WS2812B = 0,  // Modelo WS2812B
    WS2815,       // Modelo WS2815
} led_strip_model_t;

// ========================================================================================================
//---PROTÓTIPOS DE FUNÇÕES---

esp_err_t ws28xx_init(int pin, led_strip_model_t model, int num_of_leds, CRGB **led_buffer_ptr);
void ws28xx_fill_all(CRGB color);
esp_err_t ws28xx_update();

#endif //ws2812b.h