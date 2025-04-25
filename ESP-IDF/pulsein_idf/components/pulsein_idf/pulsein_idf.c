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
 * @brief Callback chamado quando um pulso é detectado pelo RMT
 */
static bool rmt_rx_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data) {
    int channel_index = (int)user_data;
    pulsein_channels[channel_index].receiving = false;
    return false;
}

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

    //---configura o callback para este canal---
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rmt_rx_callback
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(pulsein_channels[index].channel, &cbs, (void*)index));

    //---habilitar o canal antes de usar---
    ESP_ERROR_CHECK(rmt_enable(pulsein_channels[index].channel));

    //---armazena configurações do canal---
    pulsein_channels[index].gpio = gpio;
    pulsein_channels[index].resolution_hz = resolution_hz;
    pulsein_channels[index].active = true;
    pulsein_channels[index].receiving = false;

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

    ESP_ERROR_CHECK(rmt_disable(pulsein_channels[index].channel));
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

    //---configuração de recepção---
    rmt_receive_config_t recv_cfg = {
        .signal_range_min_ns = 1000,
        .signal_range_max_ns = 10000000,
    };

    //---inicia a recepção---
    pulsein_channels[index].receiving = true;
    ESP_ERROR_CHECK(rmt_receive(pulsein_channels[index].channel, 
                              pulsein_channels[index].symbols, 
                              sizeof(pulsein_channels[index].symbols), 
                              &recv_cfg));

    //---espera pelo pulso com timeout---
    uint64_t start_time = esp_timer_get_time();
    while (pulsein_channels[index].receiving && 
          (esp_timer_get_time() - start_time) < timeout_us) {
        vTaskDelay(pdMS_TO_TICKS(10));
        taskYIELD();      // permite outras tarefas rodarem
    }

    if (pulsein_channels[index].receiving) {
        ESP_LOGW(TAG, "❌ Timeout na leitura do canal %d", index);
        rmt_disable(pulsein_channels[index].channel);
        rmt_enable(pulsein_channels[index].channel); // Reinicia o canal
        return 0;
    }

    uint32_t high_us = (pulsein_channels[index].symbols[0].duration0 * 1000000ULL) / 
                      pulsein_channels[index].resolution_hz;

    ESP_LOGI(TAG, "⚡ Pulso detectado no canal %d: %lu us", index, high_us);

    return high_us;
}