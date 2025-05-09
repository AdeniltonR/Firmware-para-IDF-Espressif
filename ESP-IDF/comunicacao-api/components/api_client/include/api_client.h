/*
 * NOME: Adenilton Ribeiro
 * DATA: 17/04/2025
 * PROJETO: Comunicação API REST
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Cliente HTTP para envio de dados de velocidade à API remota
 *                    Implementação de JSON parser para estruturação de dados
 *                    Controle de conexão WiFi com reconexão automática
 *                    Timeout configurável para requisições HTTP
 *                    Sistema de logs detalhado com emojis visuais
 *                    Tratamento robusto de erros de comunicação
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
 */

 // ========================================================================================================
/**
 * @brief api_client.h
 * 
 */
#ifndef API_CLIENT_H
#define API_CLIENT_H

// ========================================================================================================
//---BIBLIOTECAS---

#include "esp_err.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include <string.h>
//#include "esp_crt_bundle.h"

// ========================================================================================================
//---CONSTANTS---

typedef struct {
    char id[32];
    char status[32];
    float velocidade;
} device_data_t;

// ========================================================================================================
//---PROTÓTIPOS DE FUNÇÕES---

esp_err_t api_client_init(const char *url_base);
esp_err_t api_send_device_data(const device_data_t *data);

#endif //api_client.h
