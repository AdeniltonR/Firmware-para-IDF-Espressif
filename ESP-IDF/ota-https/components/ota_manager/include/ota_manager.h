#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback usado para validar a saúde do firmware.
 *
 * A aplicação define quais testes devem ser executados.
 *
 * @return
 *      - true: firmware funcional
 *      - false: firmware com falha
 */
typedef bool (*ota_manager_validation_cb_t)(void);

/**
 * @brief Inicializa o OTA Manager.
 *
 * Registra os eventos internos necessários para o processo OTA
 * e verifica se o firmware atual aguarda validação de rollback.
 *
 * @param validation_cb Callback de validação da aplicação.
 *                      Pode ser NULL se não houver validação pendente.
 *
 * @return ESP_OK em caso de sucesso.
 */
esp_err_t ota_manager_init(
    ota_manager_validation_cb_t validation_cb
);

/**
 * @brief Inicia uma atualização OTA em uma tarefa FreeRTOS.
 *
 * A URL, TLS, timeout e demais parâmetros são obtidos
 * das configurações do menuconfig.
 *
 * @return
 *      - ESP_OK: OTA iniciado
 *      - ESP_ERR_INVALID_STATE: OTA já está em andamento
 *      - Outro erro ESP-IDF em caso de falha
 */
esp_err_t ota_manager_start(void);

/**
 * @brief Informa se existe uma atualização OTA em andamento.
 *
 * @return true se OTA estiver ativo.
 */
bool ota_manager_is_running(void);

#ifdef __cplusplus
}
#endif

#endif