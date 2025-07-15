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

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (ibus)
static const char *TAG = "ibus";

// Comandos do protocolo
#define IBUS_CMD_SERVO       0x40
#define IBUS_CMD_DISCOVER    0x80
#define IBUS_CMD_TYPE        0x90
#define IBUS_CMD_VALUE       0xA0

// Estados da máquina de estados
typedef enum {
    IBUS_STATE_DISCARD,
    IBUS_STATE_GET_LENGTH,
    IBUS_STATE_GET_DATA,
    IBUS_STATE_GET_CHKSUM_L,
    IBUS_STATE_GET_CHKSUM_H
} ibus_state_t;

static void send_response(ibus_handle_t *handle, uint8_t *data, uint8_t len) {
    if (uart_write_bytes(handle->uart_num, (const char *)data, len) != len) {
        ESP_LOGE(TAG, "Falha ao enviar resposta");
    }
}

esp_err_t ibus_init(ibus_handle_t *handle, uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate) {
    if (!handle) {
        ESP_LOGE(TAG, "Handle inválido");
        return ESP_ERR_INVALID_ARG;
    }

    // Configuração da UART
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    esp_err_t ret = uart_param_config(uart_num, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha na configuração da UART: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar pinos da UART: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_driver_install(uart_num, 256, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao instalar driver UART: %s", esp_err_to_name(ret));
        return ret;
    }

    // Inicializa estrutura
    handle->uart_num = uart_num;
    handle->tx_pin = tx_pin;
    handle->rx_pin = rx_pin;
    handle->sensor_count = 0;
    handle->last_update_ms = 0;
    handle->poll_count = 0;
    handle->sensor_response_count = 0;
    handle->channel_update_count = 0;

    memset(handle->channels, 0, sizeof(handle->channels));
    memset(handle->sensors, 0, sizeof(handle->sensors));

    ESP_LOGI(TAG, "iBUS inicializado na UART%d (TX:GPIO%d, RX:GPIO%d)", 
             uart_num, tx_pin, rx_pin);
    return ESP_OK;
}

uint8_t ibus_add_sensor(ibus_handle_t *handle, uint8_t type, uint8_t length) {
    if (!handle) {
        ESP_LOGE(TAG, "Handle inválido ao adicionar sensor");
        return 0;
    }

    if (length != 2 && length != 4) {
        length = 2; // Padrão para 2 bytes
    }

    if (handle->sensor_count >= IBUS_MAX_SENSORS) {
        ESP_LOGE(TAG, "Número máximo de sensores atingido");
        return 0;
    }

    ibus_sensor_t *sensor = &handle->sensors[handle->sensor_count];
    sensor->type = type;
    sensor->length = length;
    sensor->value = 0;

    ESP_LOGI(TAG, "Sensor %d adicionado: tipo=0x%02X, tamanho=%d", 
             handle->sensor_count + 1, type, length);

    return ++handle->sensor_count;
}

esp_err_t ibus_set_sensor_value(ibus_handle_t *handle, uint8_t sensor_num, int32_t value) {
    if (!handle || sensor_num == 0 || sensor_num > handle->sensor_count) {
        ESP_LOGE(TAG, "Número de sensor inválido: %d", sensor_num);
        return ESP_ERR_INVALID_ARG;
    }

    handle->sensors[sensor_num - 1].value = value;
    return ESP_OK;
}

uint16_t ibus_read_channel(ibus_handle_t *handle, uint8_t channel_num) {
    if (!handle || channel_num >= IBUS_MAX_CHANNELS) {
        return 0;
    }
    return handle->channels[channel_num];
}

esp_err_t ibus_loop(ibus_handle_t *handle) {
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer[IBUS_PROTOCOL_LENGTH];
    ibus_state_t state = IBUS_STATE_DISCARD;
    uint8_t ptr = 0;
    uint8_t len = 0;
    uint16_t chksum = 0;
    uint8_t lchksum = 0;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // Processa dados disponíveis
    size_t available = 0;
    uart_get_buffered_data_len(handle->uart_num, &available);
    
    while (available > 0) {
        uint8_t v;
        int read = uart_read_bytes(handle->uart_num, &v, 1, 0);
        if (read != 1) break;
        available--;

        // Verifica gap entre pacotes
        if (now - handle->last_update_ms >= IBUS_PROTOCOL_TIMEGAP_MS) {
            state = IBUS_STATE_GET_LENGTH;
        }
        handle->last_update_ms = now;

        switch (state) {
            case IBUS_STATE_GET_LENGTH:
                if (v <= IBUS_PROTOCOL_LENGTH && v > IBUS_PROTOCOL_OVERHEAD) {
                    ptr = 0;
                    len = v - IBUS_PROTOCOL_OVERHEAD;
                    chksum = 0xFFFF - v;
                    state = IBUS_STATE_GET_DATA;
                } else {
                    state = IBUS_STATE_DISCARD;
                }
                break;

            case IBUS_STATE_GET_DATA:
                buffer[ptr++] = v;
                chksum -= v;
                if (ptr == len) {
                    state = IBUS_STATE_GET_CHKSUM_L;
                }
                break;

            case IBUS_STATE_GET_CHKSUM_L:
                lchksum = v;
                state = IBUS_STATE_GET_CHKSUM_H;
                break;

            case IBUS_STATE_GET_CHKSUM_H:
                // Verifica checksum
                if (chksum == (v << 8) + lchksum) {
                    uint8_t adr = buffer[0] & 0x0F;
                    
                    // Comando de servo
                    if (buffer[0] == IBUS_CMD_SERVO) {
                        for (uint8_t i = 1; i < IBUS_MAX_CHANNELS * 2 + 1; i += 2) {
                            handle->channels[i / 2] = buffer[i] | (buffer[i + 1] << 8);
                        }
                        handle->channel_update_count++;
                    } 
                    // Comandos de sensor (len==1 para evitar loopback)
                    else if (adr <= handle->sensor_count && adr > 0 && len == 1) {
                        ibus_sensor_t *sensor = &handle->sensors[adr - 1];
                        uint8_t response[6];
                        uint8_t resp_len = 0;
                        
                        switch (buffer[0] & 0xF0) {
                            case IBUS_CMD_DISCOVER:
                                handle->poll_count++;
                                response[0] = 0x04;
                                response[1] = IBUS_CMD_DISCOVER + adr;
                                chksum = 0xFFFF - (0x04 + IBUS_CMD_DISCOVER + adr);
                                resp_len = 2;
                                break;
                                
                            case IBUS_CMD_TYPE:
                                response[0] = 0x06;
                                response[1] = IBUS_CMD_TYPE + adr;
                                response[2] = sensor->type;
                                response[3] = sensor->length;
                                chksum = 0xFFFF - (0x06 + IBUS_CMD_TYPE + adr + sensor->type + sensor->length);
                                resp_len = 4;
                                break;
                                
                            case IBUS_CMD_VALUE:
                                handle->sensor_response_count++;
                                resp_len = 4 + sensor->length;
                                response[0] = resp_len;
                                response[1] = IBUS_CMD_VALUE + adr;
                                response[2] = sensor->value & 0xFF;
                                response[3] = (sensor->value >> 8) & 0xFF;
                                chksum = 0xFFFF - (resp_len + IBUS_CMD_VALUE + adr + (sensor->value & 0xFF) + ((sensor->value >> 8) & 0xFF));
                                
                                if (sensor->length == 4) {
                                    response[4] = (sensor->value >> 16) & 0xFF;
                                    response[5] = (sensor->value >> 24) & 0xFF;
                                    chksum -= response[4] + response[5];
                                }
                                break;
                                
                            default:
                                adr = 0; // Comando desconhecido
                                break;
                        }
                        
                        if (adr > 0) {
                            send_response(handle, response, resp_len);
                            uint8_t chksum_bytes[2] = {chksum & 0xFF, chksum >> 8};
                            send_response(handle, chksum_bytes, 2);
                        }
                    }
                }
                state = IBUS_STATE_DISCARD;
                break;
                
            default:
                state = IBUS_STATE_DISCARD;
                break;
        }
    }
    
    return ESP_OK;
}