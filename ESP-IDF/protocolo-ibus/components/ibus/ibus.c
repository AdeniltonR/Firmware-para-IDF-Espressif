/*
 * NOME: Adenilton Ribeiro
 * DATA: 31/01/2025
 * PROJETO: i-BUS
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Envio de dados de múltiplos sensores via protocolo i-BUS (FlySky) usando UART.
 *            - docs: ESP32-S3 - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include "ibus.h"
#include "esp_timer.h"
#include "esp_log.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

/// @brief Tamanho máximo do pacote iBUS
#define PROTOCOL_LENGTH 0x20
/// @brief Overhead do protocolo (bytes adicionais além dos dados)
#define PROTOCOL_OVERHEAD 3
/// @brief Tempo mínimo entre pacotes (ms)
#define PROTOCOL_TIMEGAP 3
/// @brief Comando para controle de canais
#define PROTOCOL_COMMAND40 0x40
/// @brief Comando para descoberta de sensores
#define PROTOCOL_COMMAND_DISCOVER 0x80
/// @brief Comando para tipo de sensor
#define PROTOCOL_COMMAND_TYPE 0x90
/// @brief Comando para valor de sensor
#define PROTOCOL_COMMAND_VALUE 0xA0
// Estados da máquina de estados
#define STATE_GET_LENGTH 0
#define STATE_GET_DATA 1
#define STATE_GET_CHKSUML 2
#define STATE_GET_CHKSUMH 3
#define STATE_DISCARD 4

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (ibus)
static const char *TAG = "ibus";

// ========================================================================================================
/**
 * @brief Inicializa a interface iBUS
 * @param ibus Ponteiro para a estrutura de controle
 * @param uart_num Número da UART (UART_NUM_1, UART_NUM_2, etc)
 * @param rx_pin Pino RX
 * @param tx_pin Pino TX
 * @note Configura a UART com baudrate 115200, 8N1
*/
void ibusbm_init(ibusbm_t *ibus, uart_port_t uart_num, int rx_pin, int tx_pin) {
    ESP_LOGI(TAG, "✅ Inicializando iBUS na UART%d (RX: %d, TX: %d)", uart_num, rx_pin, tx_pin);
    
    //---inicializa estrutura---
    ibus->uart_num = uart_num;
    ibus->rx_pin = rx_pin;
    ibus->tx_pin = tx_pin;
    ibus->ptr = 0;
    ibus->len = 0;
    ibus->chksum = 0;
    ibus->lchksum = 0;
    ibus->state = STATE_DISCARD;
    ibus->last_millis = esp_timer_get_time() / 1000;
    ibus->cnt_poll = 0;
    ibus->cnt_rec = 0;
    ibus->cnt_sensor = 0;
    ibus->num_sensors = 0;

    //---limpa buffers---
    memset(ibus->channels, 0, sizeof(ibus->channels));
    memset(ibus->sensors, 0, sizeof(ibus->sensors));

    //---configura UART---
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    esp_err_t ret = uart_param_config(uart_num, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌  Falha na configuração da UART: %s", esp_err_to_name(ret));
        return;
    }

    ret = uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌  Falha ao configurar pinos da UART: %s", esp_err_to_name(ret));
        return;
    }

    ret = uart_driver_install(uart_num, 256, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌  Falha ao instalar driver UART: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "✅ iBUS inicializado com sucesso");
}

// ========================================================================================================
/**
 * @brief Processa os dados recebidos e envia respostas
 * @param ibus Ponteiro para a estrutura de controle
 * @note Esta função deve ser chamada periodicamente (a cada 1ms)
*/
void ibusbm_loop(ibusbm_t *ibus) {
    uint8_t v;
    int len;
    uint8_t t;
    uint32_t now = esp_timer_get_time() / 1000;

    //---limita a leitura para evitar bloqueio---
    int max_bytes = 32;  
    while (max_bytes-- > 0 && (len = uart_read_bytes(ibus->uart_num, &v, 1, 10 / portTICK_PERIOD_MS)) > 0) {
        //---verifica gap entre pacotes---
        if (now - ibus->last_millis >= PROTOCOL_TIMEGAP) {
            ibus->state = STATE_GET_LENGTH;
        }
        ibus->last_millis = now;

        //---Máquina de estados do protocolo---
        switch (ibus->state) {
            case STATE_GET_LENGTH:
                if (v <= PROTOCOL_LENGTH && v > PROTOCOL_OVERHEAD) {
                    ibus->ptr = 0;
                    ibus->len = v - PROTOCOL_OVERHEAD;
                    ibus->chksum = 0xFFFF - v;
                    ibus->state = STATE_GET_DATA;
                    ESP_LOGD(TAG, "Pacote recebido, tamanho: %d", v);
                } else {
                    ibus->state = STATE_DISCARD;
                    ESP_LOGW(TAG, "Tamanho de pacote inválido: %d", v);
                }
                break;

            case STATE_GET_DATA:
                ibus->buffer[ibus->ptr++] = v;
                ibus->chksum -= v;
                if (ibus->ptr == ibus->len) {
                    ibus->state = STATE_GET_CHKSUML;
                }
                break;

            case STATE_GET_CHKSUML:
                ibus->lchksum = v;
                ibus->state = STATE_GET_CHKSUMH;
                break;

            case STATE_GET_CHKSUMH:
                if (ibus->chksum == ((v << 8) + ibus->lchksum)) {
                    uint8_t adr = ibus->buffer[0] & 0x0f;

                    //---comando de canal---
                    if (ibus->buffer[0] == PROTOCOL_COMMAND40) {
                        for (uint8_t i = 1; i < PROTOCOL_CHANNELS * 2 + 1; i += 2) {
                            ibus->channels[i / 2] = ibus->buffer[i] | (ibus->buffer[i + 1] << 8);
                        }
                        ibus->cnt_rec++;
                        ESP_LOGD(TAG, "Dados de canal recebidos");
                    } 
                    //---comandos de sensor---
                    else if (adr <= ibus->num_sensors && adr > 0 && ibus->len == 1) {
                        sensorinfo_t *s = &ibus->sensors[adr - 1];

                        switch (ibus->buffer[0] & 0xf0) {
                            case PROTOCOL_COMMAND_DISCOVER:
                                ibus->cnt_poll++;
                                t = 0x04;
                                uart_write_bytes(ibus->uart_num, (const char[]){t, PROTOCOL_COMMAND_DISCOVER + adr}, 2);
                                ibus->chksum = 0xFFFF - (t + PROTOCOL_COMMAND_DISCOVER + adr);
                                ESP_LOGD(TAG, "Respondendo a discover para sensor %d", adr);
                                break;

                            case PROTOCOL_COMMAND_TYPE:
                                t = 0x06;
                                uart_write_bytes(ibus->uart_num, (const char[]){t, PROTOCOL_COMMAND_TYPE + adr, s->sensorType, s->sensorLength}, 4);
                                ibus->chksum = 0xFFFF - (t + PROTOCOL_COMMAND_TYPE + adr + s->sensorType + s->sensorLength);
                                ESP_LOGD(TAG, "Enviando tipo do sensor %d", adr);
                                break;

                            case PROTOCOL_COMMAND_VALUE: {
                                ibus->cnt_sensor++;
                                uint8_t payload[8] = {0};
                                uint8_t idx = 0;

                                payload[idx++] = t = 0x04 + s->sensorLength;
                                ibus->chksum = 0xFFFF - t;
                                payload[idx++] = t = PROTOCOL_COMMAND_VALUE + adr;
                                ibus->chksum -= t;
                                payload[idx++] = t = (s->sensorValue & 0xff);
                                ibus->chksum -= t;
                                payload[idx++] = t = (s->sensorValue >> 8) & 0xff;
                                ibus->chksum -= t;

                                if (s->sensorLength == 4) {
                                    payload[idx++] = t = (s->sensorValue >> 16) & 0xff;
                                    ibus->chksum -= t;
                                    payload[idx++] = t = (s->sensorValue >> 24) & 0xff;
                                    ibus->chksum -= t;
                                }

                                uart_write_bytes(ibus->uart_num, (const char *)payload, idx);
                                ESP_LOGD(TAG, "Enviando valor do sensor %d: %ld", adr, s->sensorValue);
                                break;
                            }

                            default:
                                adr = 0;
                                ESP_LOGW(TAG, "Comando desconhecido: 0x%02X", ibus->buffer[0]);
                                break;
                        }

                        if (adr > 0) {
                            uint8_t cks[2] = {ibus->chksum & 0xff, ibus->chksum >> 8};
                            uart_write_bytes(ibus->uart_num, (const char *)cks, 2);
                        }
                    }
                } else {
                    ESP_LOGW(TAG, "Checksum inválido (esperado: 0x%04X, recebido: 0x%02X%02X)", 
                            ibus->chksum, ibus->lchksum, v);
                }
                ibus->state = STATE_DISCARD;
                break;

            case STATE_DISCARD:
            default:
                break;
        }
    }
}

// ========================================================================================================
/**
 * @brief Lê o valor de um canal
 * @param ibus Ponteiro para a estrutura de controle
 * @param channel Número do canal (0-based)
 * @return Valor do canal ou 0 se inválido
*/
uint16_t ibusbm_read_channel(ibusbm_t *ibus, uint8_t channel) {
    if (channel < PROTOCOL_CHANNELS) {
        return ibus->channels[channel];
    }
    ESP_LOGW(TAG, "🚫  Tentativa de leitura de canal inválido: %d", channel);
    return 0;
}

// ========================================================================================================
/**
 * @brief Adiciona um sensor à lista
 * @param ibus Ponteiro para a estrutura de controle
 * @param type Tipo do sensor (IBUSS_*)
 * @param len Tamanho dos dados (2 ou 4 bytes)
 * @return Número do sensor (1-based) ou 0 em caso de erro
*/
uint8_t ibusbm_add_sensor(ibusbm_t *ibus, uint8_t type, uint8_t len) {
    if (len != 2 && len != 4) {
        len = 2;
        ESP_LOGW(TAG, "🚫  Tamanho de sensor inválido, usando padrão (2 bytes)");
    }

    if (ibus->num_sensors < SENSORMAX) {
        sensorinfo_t *s = &ibus->sensors[ibus->num_sensors];
        s->sensorType = type;
        s->sensorLength = len;
        s->sensorValue = 0;
        ibus->num_sensors++;
        
        ESP_LOGI(TAG, "⚙️  Sensor %d adicionado - Tipo: 0x%02X, Tamanho: %d", 
                ibus->num_sensors, type, len);
        return ibus->num_sensors;
    }

    ESP_LOGE(TAG, "🚫  Número máximo de sensores atingido (%d)", SENSORMAX);
    return 0;
}

// ========================================================================================================
/**
 * @brief Define o valor de um sensor
 * @param ibus Ponteiro para a estrutura de controle
 * @param addr Endereço do sensor (1-based)
 * @param value Valor a ser definido
*/
void ibusbm_set_sensor_value(ibusbm_t *ibus, uint8_t addr, int32_t value) {
    if (addr > 0 && addr <= ibus->num_sensors) {
        ibus->sensors[addr - 1].sensorValue = value;
        ESP_LOGD(TAG, "✅ Sensor %d atualizado: %ld", addr, value);
    } else {
        ESP_LOGW(TAG, "🚫  Tentativa de atualizar sensor inválido: %d", addr);
    }
}