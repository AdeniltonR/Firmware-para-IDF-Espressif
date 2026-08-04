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

// ========================================================================================================
//---BIBLIOTECAS AUXILIARES---

#include "ota_manager.h"

#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

static bool s_ota_running = false;
static bool s_initialized = false;

static ota_manager_validation_cb_t s_validation_cb = NULL;

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (ota_manager)
static const char *TAG = "ota_manager";

// ========================================================================================================
/**
 * @brief Handler de eventos do processo OTA (ESP_HTTPS_OTA_EVENT)
 * @param arg Não utilizado
 * @param event_base Base do evento
 * @param event_id Identificador do evento
 * @param event_data Dados associados ao evento
 */
static void ota_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base != ESP_HTTPS_OTA_EVENT) {
        return;
    }

    switch (event_id) {

        case ESP_HTTPS_OTA_START:
            ESP_LOGI(TAG, "OTA iniciado");
            break;

        case ESP_HTTPS_OTA_CONNECTED:
            ESP_LOGI(TAG, "Conectado ao servidor");
            break;

        case ESP_HTTPS_OTA_GET_IMG_DESC:
            ESP_LOGI(TAG, "Lendo descrição da imagem");
            break;

        case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
            ESP_LOGI(TAG, "Verificando ID do chip da nova imagem: %d", *(esp_chip_id_t *)event_data);
            break;

        case ESP_HTTPS_OTA_DECRYPT_CB:
            ESP_LOGI(TAG, "Callback de descriptografia chamado");
            break;

        case ESP_HTTPS_OTA_WRITE_FLASH:
            ESP_LOGD(TAG, "Gravando na flash: %d bytes", *(int *)event_data);
            break;

        case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
            ESP_LOGI(TAG, "Partição de boot atualizada. Próxima partição: %d", *(esp_partition_subtype_t *)event_data);
            break;

        case ESP_HTTPS_OTA_FINISH:
            ESP_LOGI(TAG, "OTA finalizado");
            break;

        case ESP_HTTPS_OTA_ABORT:
            ESP_LOGW(TAG, "OTA abortado");
            break;

        default:
            break;
    }
}


// ========================================================================================================
/**
 * @brief Valida a imagem nova antes de aplicar o OTA (versão e secure version)
 * @param new_app_info Descrição da nova imagem de firmware
 * @return
 *      - ESP_OK: imagem válida
 *      - ESP_FAIL: versão igual à atual ou secure version inválida
 *      - ESP_ERR_INVALID_ARG: new_app_info é NULL
 */
static esp_err_t validate_image_header(const esp_app_desc_t *new_app_info) {
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running =
        esp_ota_get_running_partition();

    esp_app_desc_t running_app_info;

    esp_err_t err = esp_ota_get_partition_description(running, &running_app_info);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao obter descrição do firmware atual");

        return err;
    }

    ESP_LOGI(TAG, "Versão atual: %s", running_app_info.version);

    ESP_LOGI(TAG, "Nova versão: %s", new_app_info->version);


#ifndef CONFIG_OTA_MANAGER_SKIP_VERSION_CHECK

    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        ESP_LOGW(TAG, "A versão atual é igual à nova");

        return ESP_FAIL;
    }

#endif


#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK

    const uint32_t hw_sec_version = esp_efuse_read_secure_version();

    if (new_app_info->secure_version < hw_sec_version) {
        ESP_LOGW(TAG, "Secure version inválida: %" PRIu32 " < %" PRIu32, new_app_info->secure_version, hw_sec_version);

        return ESP_FAIL;
    }

#endif

    return ESP_OK;
}


// ========================================================================================================
/**
 * @brief Callback de inicialização do cliente HTTP usado pelo esp_https_ota
 * @param http_client Handle do cliente HTTP
 * @return ESP_OK
 */
static esp_err_t http_client_init_cb(esp_http_client_handle_t http_client) {
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Task interna que executa o download e a gravação do firmware via HTTPS OTA
 * @param pvParameter Parâmetro da tarefa (não utilizado)
 */
static void ota_manager_task(void *pvParameter) {
    ESP_LOGI(TAG, "Iniciando atualização OTA");

    esp_err_t err = ESP_FAIL;
    esp_err_t ota_finish_err = ESP_OK;

    esp_https_ota_handle_t https_ota_handle = NULL;

    /* --------------------------------------------------------
     * Configuração HTTP/HTTPS
     * -------------------------------------------------------- */

    esp_http_client_config_t http_config = {
        .url = CONFIG_OTA_MANAGER_FIRMWARE_URL,

        .crt_bundle_attach = esp_crt_bundle_attach,

        .timeout_ms = CONFIG_OTA_MANAGER_RECV_TIMEOUT,

        .keep_alive_enable = true,

        .buffer_size = CONFIG_OTA_MANAGER_RX_BUFFER_SIZE,

        .buffer_size_tx = CONFIG_OTA_MANAGER_TX_BUFFER_SIZE,
    };


#ifdef CONFIG_OTA_MANAGER_SKIP_COMMON_NAME_CHECK

    http_config.skip_cert_common_name_check = true;

#endif

    /* --------------------------------------------------------
     * Configuração OTA
     * -------------------------------------------------------- */

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
        .http_client_init_cb = http_client_init_cb,
    };


#ifdef CONFIG_OTA_MANAGER_ENABLE_PARTIAL_HTTP_DOWNLOAD

    ota_config.partial_http_download = true;

    ota_config.max_http_request_size =
        CONFIG_OTA_MANAGER_HTTP_REQUEST_SIZE;

#endif

    /* --------------------------------------------------------
     * Inicia OTA
     * -------------------------------------------------------- */

    err = esp_https_ota_begin(&ota_config, &https_ota_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar OTA: %s", esp_err_to_name(err));

        goto ota_cleanup;
    }

    /* --------------------------------------------------------
     * Obtém descrição da nova imagem
     * -------------------------------------------------------- */

    esp_app_desc_t app_desc;

    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao obter descrição da imagem");

        goto ota_abort;
    }

    /* --------------------------------------------------------
     * Valida imagem
     * -------------------------------------------------------- */

    err = validate_image_header(&app_desc);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha na validação do firmware");

        goto ota_abort;
    }

    /* --------------------------------------------------------
     * Download e gravação
     * -------------------------------------------------------- */

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);

        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }

        ESP_LOGD(TAG, "Bytes recebidos: %d", esp_https_ota_get_image_len_read(https_ota_handle));
    }

    /* --------------------------------------------------------
     * Verifica download completo
     * -------------------------------------------------------- */

    if (!esp_https_ota_is_complete_data_received(https_ota_handle)) {
        ESP_LOGE(TAG, "Firmware não foi recebido completamente");

        goto ota_abort;
    }

    /* --------------------------------------------------------
     * Finaliza OTA
     * -------------------------------------------------------- */

    ota_finish_err = esp_https_ota_finish(https_ota_handle);

    https_ota_handle = NULL;

    if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
        ESP_LOGI(TAG, "OTA concluído com sucesso");

        ESP_LOGI(TAG, "Reiniciando dispositivo...");

        vTaskDelay(pdMS_TO_TICKS(1000));

        esp_restart();
    }

    if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
        ESP_LOGE(TAG, "Validação final da imagem falhou");
    }

    ESP_LOGE(TAG, "Falha ao finalizar OTA: %s", esp_err_to_name(ota_finish_err));

    goto ota_cleanup;

ota_abort:

    if (https_ota_handle != NULL) {
        esp_https_ota_abort(https_ota_handle);

        https_ota_handle = NULL;
    }

ota_cleanup:

    s_ota_running = false;

    ESP_LOGW(TAG, "Processo OTA encerrado");

    vTaskDelete(NULL);
}

// ========================================================================================================
/**
 * @brief Aguarda a janela de auto-teste e decide se o firmware
 *        deve ser confirmado como válido ou revertido.
 *
 * Enquanto esta task aguarda, o firmware permanece no estado
 * ESP_OTA_IMG_PENDING_VERIFY. Se o dispositivo reiniciar por
 * qualquer motivo (crash, watchdog, brownout, etc.) antes do
 * tempo configurado, a confirmação nunca acontece e o bootloader
 * faz rollback automático para a partição anterior no próximo boot.
 *
 * Só se o tempo de auto-teste se esgotar sem reinicialização é que
 * o callback de validação da aplicação é executado.
 *
 * @param pvParameter Parâmetro da tarefa (não utilizado)
 */
static void ota_manager_validation_task(void *pvParameter) {
    ESP_LOGI(TAG, "Aguardando %d s de auto-teste antes de validar o firmware", CONFIG_OTA_MANAGER_VALIDATION_TIME_SEC);

    vTaskDelay(pdMS_TO_TICKS(CONFIG_OTA_MANAGER_VALIDATION_TIME_SEC * 1000));

    ESP_LOGI(TAG, "Janela de auto-teste concluída sem reinicializações");

    /* --------------------------------------------------------
     * Executa teste da aplicação
     * -------------------------------------------------------- */

    bool firmware_ok = true;

    if (s_validation_cb == NULL) {
        ESP_LOGW(TAG, "Nenhum callback de validação registrado, aprovando por padrão");
    } else {
        ESP_LOGI(TAG, "Executando callback de validação");

        firmware_ok = s_validation_cb();
    }

    /* --------------------------------------------------------
     * Firmware aprovado
     * -------------------------------------------------------- */

    if (firmware_ok) {
        ESP_LOGI(TAG, "Firmware aprovado");

        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Firmware marcado como válido");

            ESP_LOGI(TAG, "Rollback cancelado");
        } else {
            ESP_LOGE(TAG, "Falha ao marcar firmware como válido: %s", esp_err_to_name(err));
        }

        vTaskDelete(NULL);
        return;
    }

    /* --------------------------------------------------------
     * Firmware reprovado
     * -------------------------------------------------------- */

    ESP_LOGE(TAG,"Firmware reprovado pela aplicação");

    ESP_LOGW(TAG, "Executando rollback para firmware anterior");

    /*
     * Esta função marca a imagem atual como inválida
     * e reinicia automaticamente o dispositivo.
     */
    esp_ota_mark_app_invalid_rollback_and_reboot();

    vTaskDelete(NULL);
}

// ========================================================================================================
/**
 * @brief Verifica se o firmware atual aguarda validação de rollback e, se
 *        necessário, inicia a task de auto-teste
 * @return
 *      - ESP_OK: verificação concluída (com ou sem rollback pendente)
 *      - ESP_FAIL: falha ao obter partição/estado da imagem
 *      - ESP_ERR_NO_MEM: falha ao criar a task de validação
 */
static esp_err_t ota_manager_check_rollback(void) {
#if CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE

    const esp_partition_t *running = esp_ota_get_running_partition();

    if (running == NULL) {
        ESP_LOGE(TAG, "Falha ao obter partição atual");

        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Partição atual: %s", running->label);

    esp_ota_img_states_t ota_state;

    esp_err_t err = esp_ota_get_state_partition(running, &ota_state);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao obter estado da imagem OTA: %s", esp_err_to_name(err));

        return err;
    }

    ESP_LOGI(TAG, "Estado da imagem OTA: %d", ota_state);

    /* --------------------------------------------------------
     * Novo firmware aguardando validação
     * -------------------------------------------------------- */

    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGW(TAG, "Novo firmware aguardando validação");

        /* ----------------------------------------------------
         * Inicia a janela de auto-teste em background.
         *
         * A confirmação (ou rollback) só acontece ao final da
         * task, então uma reinicialização inesperada durante a
         * espera deixa o firmware como PENDING_VERIFY e o
         * bootloader reverte automaticamente no próximo boot.
         * ---------------------------------------------------- */

        BaseType_t result = xTaskCreate(ota_manager_validation_task, "ota_manager_validation_task", 4096, NULL, 5, NULL);

        if (result != pdPASS){
            ESP_LOGE(TAG, "Falha ao criar task de validação do firmware");

            return ESP_ERR_NO_MEM;
        }

        ESP_LOGI(TAG, "Task de validação do firmware criada");

        return ESP_OK;
    }

    /* --------------------------------------------------------
     * Firmware não necessita validação
     * -------------------------------------------------------- */

    ESP_LOGI(TAG, "Firmware não possui validação pendente");

    return ESP_OK;

#else

    ESP_LOGW(TAG, "Rollback não está habilitado no bootloader");

    return ESP_OK;

#endif
}

// ========================================================================================================
/**
 * @brief Inicializa o OTA Manager
 * @param validation_cb Callback de validação da aplicação (pode ser NULL)
 * @return ESP_OK em caso de sucesso
 */
esp_err_t ota_manager_init(ota_manager_validation_cb_t validation_cb) {
    if (s_initialized){
        ESP_LOGW(TAG, "OTA Manager já inicializado");

        return ESP_OK;
    }

    /* --------------------------------------------------------
     * Salva callback da aplicação
     * -------------------------------------------------------- */

    s_validation_cb = validation_cb;

    /* --------------------------------------------------------
     * Registra eventos OTA
     * -------------------------------------------------------- */

    esp_err_t err = esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, ota_event_handler, NULL);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao registrar eventos OTA: %s", esp_err_to_name(err));

        return err;
    }

    /* --------------------------------------------------------
     * Marca componente como inicializado
     * -------------------------------------------------------- */

    s_initialized = true;

    ESP_LOGI(TAG, "OTA Manager inicializado");

    /* --------------------------------------------------------
     * Verifica se firmware aguarda validação
     * -------------------------------------------------------- */

    err = ota_manager_check_rollback();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha na verificação de rollback: %s", esp_err_to_name(err));

        return err;
    }

    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Inicia uma atualização OTA em uma tarefa FreeRTOS
 * @return
 *      - ESP_OK: OTA iniciado
 *      - ESP_ERR_INVALID_STATE: OTA Manager não inicializado ou OTA já em andamento
 *      - ESP_ERR_NO_MEM: falha ao criar a task OTA
 */
esp_err_t ota_manager_start(void) {
    if (!s_initialized) {
        ESP_LOGE(TAG, "OTA Manager não inicializado");

        return ESP_ERR_INVALID_STATE;
    }

    if (s_ota_running) {
        ESP_LOGW(TAG, "OTA já está em andamento");

        return ESP_ERR_INVALID_STATE;
    }

    s_ota_running = true;

    BaseType_t result = xTaskCreate(ota_manager_task, "ota_manager_task", 8192, NULL, 5, NULL);

    if (result != pdPASS) {
        s_ota_running = false;

        ESP_LOGE(TAG, "Falha ao criar task OTA");

        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Task OTA criada");

    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Informa se existe uma atualização OTA em andamento
 * @return true se OTA estiver ativo
 */
bool ota_manager_is_running(void) {
    return s_ota_running;
}