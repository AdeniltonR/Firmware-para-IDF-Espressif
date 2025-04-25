/*
* NOME: Adenilton Ribeiro
* DATA: 25/04/2025
* PROJETO: Pulsein IDF
* VERSAO: 1.0.0
* DESCRICAO: - feat: Biblioteca para leitura de pulso digital utilizando o periférico RMT com suporte a múltiplos pinos.
*            - docs: ESP32-S3 - ESP-IDF v5.4.0
* LINKS: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/rmt.html
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include "pulsein_idf.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (pulsein)
static const char *TAG = "pulsein";

//---estrutura de canais utilizados---
static rmt_pulsein_channel_t pulsein_channels[MAX_CHANNELS] = {0};

// ========================================================================================================
/**
 * @brief Inicializa um canal RMT para leitura de pulso em um pino
 *
 * @param gpio GPIO a ser usado
 * @param index Índice do canal (0 a MAX_CHANNELS - 1)
 * @param resolution_hz Resolução em Hz (ex: 1 MHz = 1us resolução)
 * @return esp_err_t ESP_OK em caso de sucesso
 */
esp_err_t rmt_pulsein_init(gpio_num_t gpio, int index, uint32_t resolution_hz) {
    if (index >= MAX_CHANNELS) {
        ESP_LOGE(TAG, "🚫  Índice de canal inválido: %d", index);
        return ESP_ERR_INVALID_ARG;
    }

    rmt_rx_channel_config_t rx_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,     // Clock padrão
        .gpio_num = gpio,                   // GPIO configurado
        .resolution_hz = resolution_hz,     // Resolução do tempo
        .mem_block_symbols = 64,            // Quantidade de símbolos por bloco
        .flags.with_dma = false,            // Sem uso de DMA
    };

    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_config, &pulsein_channels[index].channel));

    //---armazena configurações do canal---
    pulsein_channels[index].gpio = gpio;
    pulsein_channels[index].resolution_hz = resolution_hz;
    pulsein_channels[index].active = true;

    ESP_LOGI(TAG, "✅ Canal RMT inicializado no GPIO %d com resolução %lu Hz (index %d)", gpio, resolution_hz, index);

    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Desativa e libera o canal
 *
 * @param index Índice do canal
 * @return esp_err_t ESP_OK em caso de sucesso
 */
esp_err_t rmt_pulsein_deinit(int index) {
    if (index >= MAX_CHANNELS || !pulsein_channels[index].active) {
        ESP_LOGE(TAG, "🚫  Canal inválido ou inativo: %d", index);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(rmt_del_channel(pulsein_channels[index].channel));

    pulsein_channels[index].active = false;

    ESP_LOGI(TAG, "✅ Canal RMT %d desativado com sucesso", index);

    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Lê um pulso de nível alto em microsegundos no canal
 *
 * @param index Índice do canal
 * @param timeout_us Timeout da leitura em microsegundos
 * @return int64_t Duração do pulso em microsegundos, ou 0 em timeout, -1 em erro
 */
int64_t rmt_pulsein_read_us(int index, uint32_t timeout_us) {
    if (index >= MAX_CHANNELS || !pulsein_channels[index].active) {
        ESP_LOGE(TAG, "❌ Leitura falhou. Canal %d inválido ou inativo", index);
        return -1;
    }

    rmt_symbol_word_t symbols[64];
    size_t received = 0;

    //---configuração de recepção---
    rmt_receive_config_t recv_cfg = {
        .signal_range_min_ns = 1000,        // Mínimo de 1 us
        .signal_range_max_ns = 10000000,   // Máximo de 10 ms
    };

    //---inicia a recepção. Essa função é bloqueante e retorna quando o buffer enche ou dá timeout---
    esp_err_t ret = rmt_receive(pulsein_channels[index].channel, symbols, sizeof(symbols), &recv_cfg);

    if (ret != ESP_OK || received == 0) {
        ESP_LOGW(TAG, "❌ Timeout ou falha na leitura do canal %d", index);
        return 0;
    }

    // Para este exemplo, vamos assumir que symbols[0] contém o pulso de nível alto
    uint32_t high_us = (symbols[0].duration0 * 1000000ULL) / pulsein_channels[index].resolution_hz;

    ESP_LOGI(TAG, "⚡  Pulso detectado no canal %d: %lu us", index, high_us);

    return high_us;
}