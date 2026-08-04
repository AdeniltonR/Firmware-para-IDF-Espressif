/*
 * NOME: Adenilton Ribeiro
 * DATA: 12/06/2026
 * PROJETO: Wi-Fi Manager + OTA Manager
 * VERSAO: 1.1.0
 * DESCRICAO:
 *          - feat: OTA Manager via HTTPS com verificação de certificado.
 *          - feat: Validação de imagem (versão e secure version anti-rollback).
 *          - feat: Janela de auto-teste com rollback automático.
 *          - docs: ESP32 - ESP-IDF v5.4.0
 * LINKS:
*/

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

// ========================================================================================================
//---BIBLIOTECAS AUXILIARES---

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// ========================================================================================================
//---API PUBLICA---

typedef bool (*ota_manager_validation_cb_t)(void);
esp_err_t ota_manager_init(ota_manager_validation_cb_t validation_cb);
esp_err_t ota_manager_start(void);
bool ota_manager_is_running(void);

#ifdef __cplusplus
}
#endif

#endif