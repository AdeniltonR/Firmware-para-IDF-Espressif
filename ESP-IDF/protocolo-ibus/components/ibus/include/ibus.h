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
/**
 * @brief ibus.h
 * 
*/
#ifndef  IBUS_H
#define  IBUS_H

// ========================================================================================================
// ---BIBLIOTECA---

#include <stdint.h>
#include "string.h"
#include <inttypes.h>
#include "driver/uart.h"
#include <esp_err.h>
#include "esp_log.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// Tipos de sensor suportados (compatíveis com OpenTX/Turnigy)
#define IBUSS_INTV  0x00  // Tensão interna (em 0.01V)
#define IBUSS_TEMP  0x01  // Temperatura (em 0.1°C, onde 0=-40°C)
#define IBUSS_RPM   0x02  // RPM
#define IBUSS_EXTV  0x03  // Tensão externa (em 0.01V)
#define IBUS_PRESS  0x41  // Pressão (em Pa)
#define IBUS_SERVO  0xFD  // Valor do servo

// Constantes do protocolo
#define IBUS_PROTOCOL_LENGTH       0x20
#define IBUS_PROTOCOL_OVERHEAD     3
#define IBUS_PROTOCOL_TIMEGAP_MS   3
#define IBUS_MAX_CHANNELS          14
#define IBUS_MAX_SENSORS           10

// --------------------------
// Modos de voo (Flight Modes)
// --------------------------
#define STAB           0
#define ACRO           1
#define AHOLD          2
#define AUTO           3
#define GUIDED         4
#define LOITER         5
#define RTL            6
#define CIRCLE         7
#define PHOLD          8
#define LAND           9

// --------------------------
// Estado de armamento
// --------------------------
#define UNARMED        0
#define ARMED          1

// --------------------------
// Sensores de 2 bytes (uint16_t)
// --------------------------
#define IBUS_MEAS_TYPE_TEM            0x01 // Temperatura (décimos de °C)
#define IBUS_MEAS_TYPE_EXTV           0x03 // Tensão externa (décimos de V)
#define IBUS_MEAS_TYPE_CELL           0x04 // Média da tensão das células
#define IBUS_MEAS_TYPE_BAT_CURR       0x05 // Corrente da bateria (décimos de A)
#define IBUS_MEAS_TYPE_FUEL           0x06 // Bateria restante (%)
#define IBUS_MEAS_TYPE_RPM            0x07 // Rotação por minuto
#define IBUS_MEAS_TYPE_CMP_HEAD       0x08 // Direção da bússola
#define IBUS_MEAS_TYPE_CLIMB_RATE     0x09 // Taxa de subida
#define IBUS_MEAS_TYPE_COG            0x0A // Curso sobre o solo
#define IBUS_MEAS_TYPE_GPS_STATUS     0x0B // Status do GPS (2 valores codificados)
#define IBUS_MEAS_TYPE_ACC_X          0x0C // Aceleração eixo X
#define IBUS_MEAS_TYPE_ACC_Y          0x0D // Aceleração eixo Y
#define IBUS_MEAS_TYPE_ACC_Z          0x0E // Aceleração eixo Z
#define IBUS_MEAS_TYPE_ROLL           0x0F // Ângulo de rolamento
#define IBUS_MEAS_TYPE_PITCH          0x10 // Ângulo de inclinação
#define IBUS_MEAS_TYPE_YAW            0x11 // Ângulo de guinada
#define IBUS_MEAS_TYPE_VERTICAL_SPEED 0x12 // Velocidade vertical
#define IBUS_MEAS_TYPE_GROUND_SPEED   0x13 // Velocidade sobre o solo (m/s)
#define IBUS_MEAS_TYPE_GPS_DIST       0x14 // Distância ao ponto de origem
#define IBUS_MEAS_TYPE_ARMED          0x15 // Armado / desarmado
#define IBUS_MEAS_TYPE_FLIGHT_MODE    0x16 // Modo de voo

#define IBUS_MEAS_TYPE_PRES           0x41 // Pressão
#define IBUS_MEAS_TYPE_SPE            0x7E // Velocidade (km/h)

// --------------------------
// Sensores de 4 bytes (int32_t)
// --------------------------
#define IBUS_MEAS_TYPE_GPS_LAT        0x80 // Latitude (graus * 10^7)
#define IBUS_MEAS_TYPE_GPS_LON        0x81 // Longitude (graus * 10^7)
#define IBUS_MEAS_TYPE_GPS_ALT        0x82 // Altitude GPS
#define IBUS_MEAS_TYPE_ALT            0x83 // Altitude barométrica
#define IBUS_MEAS_TYPE_ALT_MAX        0x84 // Altitude máxima registrada
#define IBUS_MEAS_TYPE_S85            0x85 // Reservado
#define IBUS_MEAS_TYPE_S86            0x86
#define IBUS_MEAS_TYPE_S87            0x87
#define IBUS_MEAS_TYPE_S88            0x88
#define IBUS_MEAS_TYPE_S89            0x89
#define IBUS_MEAS_TYPE_S8A            0x8A

// ========================================================================================================
//---ESTRUTURAS DE DADOS---
typedef struct {
    uint8_t type;       // Tipo do sensor (IBUSS_*)
    uint8_t length;     // Tamanho dos dados (2 ou 4 bytes)
    int32_t value;      // Valor atual do sensor
} ibus_sensor_t;

typedef struct {
    uart_port_t uart_num;           // Número da UART (ex: UART_NUM_2)
    int tx_pin;                     // Pino TX
    int rx_pin;                     // Pino RX
    uint16_t channels[IBUS_MAX_CHANNELS];  // Canais recebidos
    ibus_sensor_t sensors[IBUS_MAX_SENSORS]; // Sensores configurados
    uint8_t sensor_count;           // Número de sensores ativos
    uint32_t last_update_ms;        // Última atualização
    uint32_t poll_count;            // Contador de polls recebidos
    uint32_t sensor_response_count; // Contador de respostas enviadas
    uint32_t channel_update_count;  // Contador de atualizações de canal
} ibus_handle_t;


// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

esp_err_t ibus_init(ibus_handle_t *handle, uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate);
uint8_t ibus_add_sensor(ibus_handle_t *handle, uint8_t type, uint8_t length);
esp_err_t ibus_set_sensor_value(ibus_handle_t *handle, uint8_t sensor_num, int32_t value);
uint16_t ibus_read_channel(ibus_handle_t *handle, uint8_t channel_num);
esp_err_t ibus_loop(ibus_handle_t *handle);

#endif //ibus.h
