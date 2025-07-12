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

static int uart_port = UART_NUM_1;

// Estrutura interna do sensor
typedef struct {
    uint8_t type;
    uint8_t id;
    uint16_t value;
} ibus_sensor_t;

static ibus_sensor_t ibus_sensors[IBUS_MAX_SENSORS];
static int ibus_sensor_count = 0;

// ========================================================================================================
/**
 * @brief Inicializa o barramento i-BUS via UART
 * 
 * @param uart_num UART a ser usada (ex: UART_NUM_1)
 * @param tx_pin GPIO conectado à porta SENS do receptor FlySky
 */
void ibus_init(int uart_num, int tx_pin) {
    uart_port = uart_num;

    uart_config_t uart_config = {
        .baud_rate = IBUS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(uart_port, 512, 0, 0, NULL, 0);
    uart_param_config(uart_port, &uart_config);
    uart_set_pin(uart_port, tx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "✅ IBUS UART inicializada na porta %d, TX GPIO %d", uart_port, tx_pin);
}

void ibus_add_sensor(uint8_t type, uint8_t id) {
    if (ibus_sensor_count >= IBUS_MAX_SENSORS) return;
    ibus_sensors[ibus_sensor_count++] = (ibus_sensor_t){ .type = type, .id = id, .value = 0 };
    ESP_LOGI(TAG, "📦 Sensor adicionado: tipo=0x%02X, id=%d", type, id);
}

void ibus_set_sensor_value(uint8_t id, uint16_t value) {
    for (int i = 0; i < ibus_sensor_count; i++) {
        if (ibus_sensors[i].id == id) {
            ibus_sensors[i].value = value;
            return;
        }
    }
}

void ibus_send_all(void) {
    int count = ibus_sensor_count;
    if (count > IBUS_MAX_SENSORS) count = IBUS_MAX_SENSORS;

    uint8_t payload_len = count * 4;
    uint8_t packet[2 + payload_len + 2];
    int idx = 0;

    packet[idx++] = IBUS_HEADER;
    packet[idx++] = payload_len;

    for (int i = 0; i < count; i++) {
        packet[idx++] = ibus_sensors[i].type;
        packet[idx++] = ibus_sensors[i].id;
        packet[idx++] = ibus_sensors[i].value & 0xFF;
        packet[idx++] = (ibus_sensors[i].value >> 8) & 0xFF;

        ESP_LOGD(TAG, "📡 Sensor %d → tipo=0x%02X, id=%d, valor=%d",
                 i, ibus_sensors[i].type, ibus_sensors[i].id, ibus_sensors[i].value);
    }

    uint16_t checksum = 0;
    for (int i = 0; i < idx; i++) {
        checksum += packet[i];
    }

    packet[idx++] = checksum & 0xFF;
    packet[idx++] = (checksum >> 8) & 0xFF;

    uart_write_bytes(uart_port, (const char *)packet, idx);
    ESP_LOGI(TAG, "📨 Enviado %d sensores (%d bytes)", count, idx);
}

// ========================================================================================================
/**
 * @brief Envia dados de até 14 sensores i-BUS para o receptor
 * 
 * @param types      Array com tipos de sensor (ex: 0xA1, 0x01, 0x02)
 * @param ids        Array com IDs dos sensores (0-13)
 * @param values     Array com os valores (16 bits) de cada sensor
 * @param count      Quantidade de sensores a enviar (máx: 14)
 */
void ibus_send(const uint8_t *types, const uint8_t *ids, const uint16_t *values, int count) {
    if (count > IBUS_MAX_SENSORS) count = IBUS_MAX_SENSORS;

    uint8_t payload_len = count * 4;
    uint8_t packet[2 + payload_len + 2];  // header + len + payload + checksum
    int idx = 0;

    packet[idx++] = IBUS_HEADER;
    packet[idx++] = payload_len;

    for (int i = 0; i < count; i++) {
        packet[idx++] = types[i];
        packet[idx++] = ids[i];
        packet[idx++] = values[i] & 0xFF;
        packet[idx++] = (values[i] >> 8) & 0xFF;

        ESP_LOGD(TAG, "🔅  Sensor %d (tipo 0x%02X, ID %d): valor = %d", i, types[i], ids[i], values[i]);
    }

    uint16_t checksum = 0;
    for (int i = 0; i < idx; i++) {
        checksum += packet[i];
    }

    packet[idx++] = checksum & 0xFF;
    packet[idx++] = (checksum >> 8) & 0xFF;

    uart_write_bytes(uart_port, (const char *)packet, idx);

    ESP_LOGI(TAG, "🗂️  Pacote i-BUS enviado com %d sensores (tamanho total: %d bytes)", count, idx);
}
