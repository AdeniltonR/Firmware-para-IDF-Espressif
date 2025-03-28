/*
 * NOME: Adenilton Ribeiro
 * DATA: 20/03/2025
 * PROJETO: Led Strip
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Controle de leds RGB usando RMT, mas a biblioteca driver/rmt.h está obsoleta.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include "led_strip.h"

#define RMT_TX_CHANNEL RMT_CHANNEL_0  // Canal RMT padrão para transmissão

/// @brief Tag para identificação dos logs deste módulo (led-strip)
static const char *TAG = "led-strip";  

//---macro para verificação de erros---
#define STRIP_CHECK(a, str, goto_tag, ret_value, ...)                             \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            ret = ret_value;                                                      \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

//---definições de temporização para o protocolo WS2812 (em nanossegundos)---
#define WS2812_T0H_NS (350)   // Tempo de HIGH para bit 0
#define WS2812_T0L_NS (1000)  // Tempo de LOW para bit 0
#define WS2812_T1H_NS (1000)  // Tempo de HIGH para bit 1
#define WS2812_T1L_NS (350)   // Tempo de LOW para bit 1
#define WS2812_RESET_US (280) // Tempo de reset (em microssegundos)

//---variáveis para armazenar os ticks do RMT correspondentes aos tempos do WS2812---
static uint32_t ws2812_t0h_ticks = 0;
static uint32_t ws2812_t1h_ticks = 0;
static uint32_t ws2812_t0l_ticks = 0;
static uint32_t ws2812_t1l_ticks = 0;

// ========================================================================================================
/**
 * @brief Estrutura para representar uma tira de LEDs WS2812.
 */
typedef struct {
    led_strip_t parent;      // Estrutura base da tira de LEDs
    rmt_channel_t rmt_channel; // Canal RMT usado para controle
    uint32_t strip_len;      // Número de LEDs na tira
    uint8_t buffer[0];       // Buffer para armazenar os dados dos LEDs (formato GRB)
} ws2812_t;

// ========================================================================================================
/**
 * @brief Converte dados RGB para o formato RMT.
 *
 * @note Para WS2812, cada bit é representado por pulsos HIGH e LOW específicos.
 *
 * @param[in] src: Dados de origem (RGB).
 * @param[in] dest: Local para armazenar o resultado convertido (formato RMT).
 * @param[in] src_size: Tamanho dos dados de origem.
 * @param[in] wanted_num: Número de itens RMT desejados.
 * @param[out] translated_size: Número de dados de origem convertidos.
 * @param[out] item_num: Número de itens RMT gerados.
 */
static void IRAM_ATTR ws2812_rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size, size_t wanted_num, size_t *translated_size, size_t *item_num) {
    if (src == NULL || dest == NULL) {
        *translated_size = 0;
        *item_num = 0;
        return;
    }

    //---definição dos pulsos RMT para bits 0 e 1---
    const rmt_item32_t bit0 = {{{ ws2812_t0h_ticks, 1, ws2812_t0l_ticks, 0 }}}; // Bit 0
    const rmt_item32_t bit1 = {{{ ws2812_t1h_ticks, 1, ws2812_t1l_ticks, 0 }}}; // Bit 1

    size_t size = 0;
    size_t num = 0;
    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;

    //---conversão dos dados RGB para o formato RMT---
    while (size < src_size && num < wanted_num) {
        for (int i = 0; i < 8; i++) {
            //---MSB first---
            if (*psrc & (1 << (7 - i))) {
                pdest->val = bit1.val; // Bit 1
            } else {
                pdest->val = bit0.val; // Bit 0
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }

    *translated_size = size;
    *item_num = num;
}

// ========================================================================================================
/**
 * @brief Define a cor de um LED específico na tira.
 *
 * @param[in] strip: Ponteiro para a tira de LEDs.
 * @param[in] index: Índice do LED a ser configurado.
 * @param[in] red: Intensidade do vermelho (0-255).
 * @param[in] green: Intensidade do verde (0-255).
 * @param[in] blue: Intensidade do azul (0-255).
 * @return esp_err_t: ESP_OK em caso de sucesso, erro caso contrário.
 */
static esp_err_t ws2812_set_pixel(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue) {
    esp_err_t ret = ESP_OK;
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    STRIP_CHECK(index < ws2812->strip_len, "index out of the maximum number of leds", err, ESP_ERR_INVALID_ARG);

    //---armazena a cor no buffer (formato GRB)---
    uint32_t start = index * 3;
    ws2812->buffer[start + 0] = green & 0xFF;
    ws2812->buffer[start + 1] = red & 0xFF;
    ws2812->buffer[start + 2] = blue & 0xFF;

    return ESP_OK;
err:
    return ret;
}

// ========================================================================================================
/**
 * @brief Atualiza a tira de LEDs com os dados do buffer.
 *
 * @param[in] strip: Ponteiro para a tira de LEDs.
 * @param[in] timeout_ms: Tempo máximo de espera para a transmissão.
 * @return esp_err_t: ESP_OK em caso de sucesso, erro caso contrário.
 */
static esp_err_t ws2812_refresh(led_strip_t *strip, uint32_t timeout_ms) {
    esp_err_t ret = ESP_OK;
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);

    //---envia os dados do buffer para a tira de LEDs via RMT---
    STRIP_CHECK(rmt_write_sample(ws2812->rmt_channel, ws2812->buffer, ws2812->strip_len * 3, true) == ESP_OK,
                "transmit RMT samples failed", err, ESP_FAIL);

    //---aguarda a conclusão da transmissão---
    return rmt_wait_tx_done(ws2812->rmt_channel, pdMS_TO_TICKS(timeout_ms));
err:
    return ret;
}

// ========================================================================================================
/**
 * @brief Desliga todos os LEDs da tira.
 *
 * @param[in] strip: Ponteiro para a tira de LEDs.
 * @param[in] timeout_ms: Tempo máximo de espera para a transmissão.
 * @return esp_err_t: ESP_OK em caso de sucesso, erro caso contrário.
 */
static esp_err_t ws2812_clear(led_strip_t *strip, uint32_t timeout_ms) {
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);

    //---preenche o buffer com zeros (desliga todos os LEDs)---
    memset(ws2812->buffer, 0, ws2812->strip_len * 3);

    //---atualiza a tira de LEDs---
    return ws2812_refresh(strip, timeout_ms);
}

// ========================================================================================================
/**
 * @brief Libera a memória alocada para a tira de LEDs.
 *
 * @param[in] strip: Ponteiro para a tira de LEDs.
 * @return esp_err_t: ESP_OK em caso de sucesso.
 */
static esp_err_t ws2812_del(led_strip_t *strip) {
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    free(ws2812);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Cria uma nova instância de tira de LEDs WS2812.
 *
 * @param[in] config: Configuração da tira de LEDs.
 * @return led_strip_t*: Ponteiro para a tira de LEDs criada, ou NULL em caso de erro.
 */
led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config) {
    led_strip_t *ret = NULL;
    STRIP_CHECK(config, "configuration can't be null", err, NULL);

    //---aloca memória para a estrutura ws2812_t---
    uint32_t ws2812_size = sizeof(ws2812_t) + config->max_leds * 3;
    ws2812_t *ws2812 = calloc(1, ws2812_size);
    STRIP_CHECK(ws2812, "request memory for ws2812 failed", err, NULL);

    //---configura os ticks do RMT com base no clock---
    uint32_t counter_clk_hz = 0;
    STRIP_CHECK(rmt_get_counter_clock((rmt_channel_t)config->dev, &counter_clk_hz) == ESP_OK,
                "get rmt counter clock failed", err, NULL);

    float ratio = (float)counter_clk_hz / 1e9;
    ws2812_t0h_ticks = (uint32_t)(ratio * WS2812_T0H_NS);
    ws2812_t0l_ticks = (uint32_t)(ratio * WS2812_T0L_NS);
    ws2812_t1h_ticks = (uint32_t)(ratio * WS2812_T1H_NS);
    ws2812_t1l_ticks = (uint32_t)(ratio * WS2812_T1L_NS);

    //---inicializa o tradutor RMT---
    rmt_translator_init((rmt_channel_t)config->dev, ws2812_rmt_adapter);

    //---configura a estrutura ws2812_t---
    ws2812->rmt_channel = (rmt_channel_t)config->dev;
    ws2812->strip_len = config->max_leds;

    //---configura as funções da tira de LEDs---
    ws2812->parent.set_pixel = ws2812_set_pixel;
    ws2812->parent.refresh = ws2812_refresh;
    ws2812->parent.clear = ws2812_clear;
    ws2812->parent.del = ws2812_del;

    return &ws2812->parent;
err:
    return ret;
}

// ========================================================================================================
/**
 * @brief Inicializa uma tira de LEDs WS2812.
 *
 * @param[in] channel: Canal RMT a ser usado.
 * @param[in] gpio: Pino GPIO conectado à tira de LEDs.
 * @param[in] led_num: Número de LEDs na tira.
 * @return led_strip_t*: Ponteiro para a tira de LEDs inicializada, ou NULL em caso de erro.
 */
led_strip_t *led_strip_init(uint8_t channel, uint8_t gpio, uint16_t led_num) {
    static led_strip_t *pStrip;

    //---configura o RMT---
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(gpio, channel);
    config.clk_div = 2;  // Define o divisor de clock para 40 MHz

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    //---configura a tira de LEDs---
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(led_num, (led_strip_dev_t)config.channel);
    pStrip = led_strip_new_rmt_ws2812(&strip_config);

    if (!pStrip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
        return NULL;
    }

    //---desliga todos os LEDs---
    ESP_ERROR_CHECK(pStrip->clear(pStrip, 100));

    return pStrip;
}

// ========================================================================================================
/**
 * @brief Desinicializa uma tira de LEDs.
 *
 * @param[in] strip: Ponteiro para a tira de LEDs.
 * @return esp_err_t: ESP_OK em caso de sucesso.
 */
esp_err_t led_strip_denit(led_strip_t *strip) {
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    ESP_ERROR_CHECK(rmt_driver_uninstall(ws2812->rmt_channel));
    return strip->del(strip);
}