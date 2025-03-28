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
//---BIBLIOTECAS---

#include "ws2812b.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (ws2812b)
static const char *TAG = "ws2812b";

//---array para mapear os bits de cor para o formato SPI---
static const uint16_t timing_bits[16] = {
    0x1111, 0x7111, 0x1711, 0x7711, 0x1171, 0x7171, 0x1771, 0x7771,
    0x1117, 0x7117, 0x1717, 0x7717, 0x1177, 0x7177, 0x1777, 0x7777
};

uint16_t *dma_buffer;           // Buffer DMA para comunicação SPI
CRGB *ws28xx_pixels;            // Buffer de pixels (cores dos LEDs)
static int n_of_leds;           // Número de LEDs na tira
static int reset_delay;         // Atraso de reset (depende do modelo do LED)
static int dma_buf_size;        // Tamanho do buffer DMA
led_strip_model_t led_model;    // Modelo da tira de LED (WS2812B ou WS2815)

// ========================================================================================================
/**
 * @brief Configurações padrão do SPI para comunicação com os LEDs WS2812B.
 * 
 * @note A configuração inclui velocidade do clock, modo SPI, pinos, etc.
 */
static spi_settings_t spi_settings = {
    .host = SPI2_HOST,           // Host SPI (SPI2)
    .dma_chan = SPI_DMA_CH_AUTO, // Canal DMA automático
    .buscfg = {
        .miso_io_num = -1,       // Pino MISO não utilizado
        .sclk_io_num = -1,       // Pino SCLK não utilizado
        .quadwp_io_num = -1,     // Pino QUADWP não utilizado
        .quadhd_io_num = -1,     // Pino QUADHD não utilizado
    },
    .devcfg = {
        .clock_speed_hz = 3.2 * 1000 * 1000, // Clock de 3.2 MHz
        .mode = 0,                           // Modo SPI 0
        .spics_io_num = -1,                  // Pino CS não utilizado
        .queue_size = 1,                     // Tamanho da fila de transmissão
        .command_bits = 0,                   // Sem bits de comando
        .address_bits = 0,                   // Sem bits de endereço
        .flags = SPI_DEVICE_TXBIT_LSBFIRST,  // Transmissão LSB primeiro
    },
};

// ========================================================================================================
/**
 * @brief Inicializa a tira de LEDs WS2812B.
 * 
 * @param pin Pino GPIO conectado ao barramento de dados dos LEDs.
 * @param model Modelo da tira de LED (WS2812B ou WS2815).
 * @param num_of_leds Número de LEDs na tira.
 * @param led_buffer_ptr Ponteiro para o buffer de LEDs.
 * @return esp_err_t Código de erro ESP_OK em caso de sucesso.
 */
esp_err_t ws28xx_init(int pin, led_strip_model_t model, int num_of_leds, CRGB **led_buffer_ptr) {
    esp_err_t err = ESP_OK;
    n_of_leds = num_of_leds;
    led_model = model;

    //---define o atraso de reset com base no modelo do LED---
    reset_delay = (model == WS2812B) ? 3 : 30;

    //---calcula o tamanho do buffer DMA (12 bytes por LED + bytes de reset)---
    dma_buf_size = n_of_leds * 12 + (reset_delay + 1) * 2;

    //---aloca memória para o buffer de pixels---
    ws28xx_pixels = malloc(sizeof(CRGB) * n_of_leds);
    if (ws28xx_pixels == NULL) {
        ESP_LOGE(TAG, "❌ Falha na alocação do buffer de pixels (Memória insuficiente)");
        return ESP_ERR_NO_MEM;
    }
    *led_buffer_ptr = ws28xx_pixels;
    ESP_LOGI(TAG, "✅ Buffer de pixels alocado com sucesso");

    //---configura o pino MOSI (dados) no barramento SPI---
    spi_settings.buscfg.mosi_io_num = pin;
    spi_settings.buscfg.max_transfer_sz = dma_buf_size;

    //---inicializa o barramento SPI---
    err = spi_bus_initialize(spi_settings.host, &spi_settings.buscfg, spi_settings.dma_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Falha na inicialização do SPI: %s", esp_err_to_name(err));
        free(ws28xx_pixels);
        return err;
    }
    ESP_LOGI(TAG, "✅ Barramento SPI inicializado com sucesso");

    //---adiciona o dispositivo SPI ao barramento---
    err = spi_bus_add_device(spi_settings.host, &spi_settings.devcfg, &spi_settings.spi);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Falha ao adicionar dispositivo SPI: %s", esp_err_to_name(err));
        free(ws28xx_pixels);
        return err;
    }
    ESP_LOGI(TAG, "✅ Dispositivo SPI configurado com sucesso");

    //---aloca memória para o buffer DMA (deve ser memória DMA)---
    dma_buffer = heap_caps_malloc(dma_buf_size, MALLOC_CAP_DMA);
    if (dma_buffer == NULL) {
        ESP_LOGE(TAG, "❌ Falha na alocação do buffer DMA (Memória insuficiente)");
        free(ws28xx_pixels);
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "✅ Buffer DMA alocado com sucesso");

    //---log de sucesso geral---
    ESP_LOGI(TAG, "✅ Tira de LEDs inicializada com sucesso no pino GPIO %d", pin);

    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Preenche todos os LEDs com uma cor específica.
 * 
 * @param color Cor a ser aplicada a todos os LEDs.
 */
void ws28xx_fill_all(CRGB color) {
    for (int i = 0; i < n_of_leds; i++) {
        ws28xx_pixels[i] = color;
    }
}

// ========================================================================================================
/**
 * @brief Atualiza a tira de LEDs com os valores atuais do buffer.
 * 
 * @return esp_err_t Código de erro ESP_OK em caso de sucesso.
 */
esp_err_t ws28xx_update() {
    esp_err_t err;
    int n = 0;

    //---limpa o buffer DMA---
    memset(dma_buffer, 0, dma_buf_size);

    //---adiciona um byte inicial de zero---
    dma_buffer[n++] = 0;

    //---converte os valores de cor dos LEDs para o formato SPI---
    for (int i = 0; i < n_of_leds; i++) {
        uint32_t temp = ws28xx_pixels[i].num;

        //---converte os bits de cor para o formato SPI---
        if (led_model == WS2815) {
            //---WS2815 usa uma ordem diferente para os bits de cor---
            dma_buffer[n++] = timing_bits[0x0f & (temp >> 4)];
            dma_buffer[n++] = timing_bits[0x0f & (temp)];

            dma_buffer[n++] = timing_bits[0x0f & (temp >> 12)]; // Verde
            dma_buffer[n++] = timing_bits[0x0f & (temp) >> 8];
        } else {
            //---WS2812B usa a ordem padrão---
            dma_buffer[n++] = timing_bits[0x0f & (temp >> 12)]; // Verde
            dma_buffer[n++] = timing_bits[0x0f & (temp) >> 8];

            dma_buffer[n++] = timing_bits[0x0f & (temp >> 4)];  // Vermelho
            dma_buffer[n++] = timing_bits[0x0f & (temp)];
        }

        //---azul---
        dma_buffer[n++] = timing_bits[0x0f & (temp >> 20)];
        dma_buffer[n++] = timing_bits[0x0f & (temp) >> 16];
    }

    //---adiciona o atraso de reset---
    for (int i = 0; i < reset_delay; i++) {
        dma_buffer[n++] = 0;
    }

    //---transmite os dados via SPI---
    err = spi_device_transmit(spi_settings.spi, &(spi_transaction_t){
                                                    .length = dma_buf_size * 8,
                                                    .tx_buffer = dma_buffer,
                                                });
    return err;
}