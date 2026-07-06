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

static const char *TAG = "ota_manager";

static bool s_ota_running = false;
static bool s_initialized = false;

static ota_manager_validation_cb_t s_validation_cb = NULL;


/* ============================================================
 * Handler de eventos OTA
 * ============================================================ */

static void ota_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
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
            ESP_LOGI(
                TAG,
                "Verificando ID do chip da nova imagem: %d",
                *(esp_chip_id_t *)event_data
            );
            break;

        case ESP_HTTPS_OTA_DECRYPT_CB:
            ESP_LOGI(TAG, "Callback de descriptografia chamado");
            break;

        case ESP_HTTPS_OTA_WRITE_FLASH:
            ESP_LOGD(
                TAG,
                "Gravando na flash: %d bytes",
                *(int *)event_data
            );
            break;

        case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
            ESP_LOGI(
                TAG,
                "Partição de boot atualizada. Próxima partição: %d",
                *(esp_partition_subtype_t *)event_data
            );
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


/* ============================================================
 * Validação da imagem
 * ============================================================ */

static esp_err_t validate_image_header(
    const esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running =
        esp_ota_get_running_partition();

    esp_app_desc_t running_app_info;

    esp_err_t err =
        esp_ota_get_partition_description(
            running,
            &running_app_info
        );

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Falha ao obter descrição do firmware atual"
        );

        return err;
    }

    ESP_LOGI(
        TAG,
        "Versão atual: %s",
        running_app_info.version
    );

    ESP_LOGI(
        TAG,
        "Nova versão: %s",
        new_app_info->version
    );


#ifndef CONFIG_OTA_MANAGER_SKIP_VERSION_CHECK

    if (memcmp(
            new_app_info->version,
            running_app_info.version,
            sizeof(new_app_info->version)
        ) == 0)
    {
        ESP_LOGW(
            TAG,
            "A versão atual é igual à nova"
        );

        return ESP_FAIL;
    }

#endif


#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK

    const uint32_t hw_sec_version =
        esp_efuse_read_secure_version();

    if (new_app_info->secure_version < hw_sec_version)
    {
        ESP_LOGW(
            TAG,
            "Secure version inválida: %" PRIu32 " < %" PRIu32,
            new_app_info->secure_version,
            hw_sec_version
        );

        return ESP_FAIL;
    }

#endif

    return ESP_OK;
}


/* ============================================================
 * Callback HTTP
 * ============================================================ */

static esp_err_t http_client_init_cb(
    esp_http_client_handle_t http_client)
{
    return ESP_OK;
}


/* ============================================================
 * Task OTA interna
 * ============================================================ */

static void ota_manager_task(void *pvParameter)
{
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

    err = esp_https_ota_begin(
        &ota_config,
        &https_ota_handle
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Falha ao iniciar OTA: %s",
            esp_err_to_name(err)
        );

        goto ota_cleanup;
    }


    /* --------------------------------------------------------
     * Obtém descrição da nova imagem
     * -------------------------------------------------------- */

    esp_app_desc_t app_desc;

    err = esp_https_ota_get_img_desc(
        https_ota_handle,
        &app_desc
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Falha ao obter descrição da imagem"
        );

        goto ota_abort;
    }


    /* --------------------------------------------------------
     * Valida imagem
     * -------------------------------------------------------- */

    err = validate_image_header(&app_desc);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Falha na validação do firmware"
        );

        goto ota_abort;
    }


    /* --------------------------------------------------------
     * Download e gravação
     * -------------------------------------------------------- */

    while (1)
    {
        err = esp_https_ota_perform(
            https_ota_handle
        );

        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
        {
            break;
        }

        ESP_LOGD(
            TAG,
            "Bytes recebidos: %d",
            esp_https_ota_get_image_len_read(
                https_ota_handle
            )
        );
    }


    /* --------------------------------------------------------
     * Verifica download completo
     * -------------------------------------------------------- */

    if (!esp_https_ota_is_complete_data_received(
            https_ota_handle))
    {
        ESP_LOGE(
            TAG,
            "Firmware não foi recebido completamente"
        );

        goto ota_abort;
    }


    /* --------------------------------------------------------
     * Finaliza OTA
     * -------------------------------------------------------- */

    ota_finish_err =
        esp_https_ota_finish(
            https_ota_handle
        );

    https_ota_handle = NULL;


    if ((err == ESP_OK) &&
        (ota_finish_err == ESP_OK))
    {
        ESP_LOGI(
            TAG,
            "OTA concluído com sucesso"
        );

        ESP_LOGI(
            TAG,
            "Reiniciando dispositivo..."
        );

        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );

        esp_restart();
    }


    if (ota_finish_err ==
        ESP_ERR_OTA_VALIDATE_FAILED)
    {
        ESP_LOGE(
            TAG,
            "Validação final da imagem falhou"
        );
    }

    ESP_LOGE(
        TAG,
        "Falha ao finalizar OTA: %s",
        esp_err_to_name(ota_finish_err)
    );

    goto ota_cleanup;


ota_abort:

    if (https_ota_handle != NULL)
    {
        esp_https_ota_abort(
            https_ota_handle
        );

        https_ota_handle = NULL;
    }


ota_cleanup:

    s_ota_running = false;

    ESP_LOGW(
        TAG,
        "Processo OTA encerrado"
    );

    vTaskDelete(NULL);
}

/* ============================================================
 * Validação do firmware após OTA
 * ============================================================ */

static esp_err_t ota_manager_check_rollback(void)
{
#if CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE

    const esp_partition_t *running =
        esp_ota_get_running_partition();

    if (running == NULL)
    {
        ESP_LOGE(
            TAG,
            "Falha ao obter partição atual"
        );

        return ESP_FAIL;
    }

    ESP_LOGI(
        TAG,
        "Partição atual: %s",
        running->label
    );

    esp_ota_img_states_t ota_state;

    esp_err_t err =
        esp_ota_get_state_partition(
            running,
            &ota_state
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Falha ao obter estado da imagem OTA: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    ESP_LOGI(
        TAG,
        "Estado da imagem OTA: %d",
        ota_state
    );


    /* --------------------------------------------------------
     * Novo firmware aguardando validação
     * -------------------------------------------------------- */

    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        ESP_LOGW(
            TAG,
            "Novo firmware aguardando validação"
        );


        /* ----------------------------------------------------
         * Verifica callback
         * ---------------------------------------------------- */

        if (s_validation_cb == NULL)
        {
            ESP_LOGE(
                TAG,
                "Nenhum callback de validação registrado"
            );

            ESP_LOGW(
                TAG,
                "Firmware não será marcado como válido"
            );

            return ESP_ERR_INVALID_STATE;
        }


        /* ----------------------------------------------------
         * Executa teste da aplicação
         * ---------------------------------------------------- */

        ESP_LOGI(
            TAG,
            "Executando callback de validação"
        );

        bool firmware_ok =
            s_validation_cb();


        /* ----------------------------------------------------
         * Firmware aprovado
         * ---------------------------------------------------- */

        if (firmware_ok)
        {
            ESP_LOGI(
                TAG,
                "Firmware aprovado pela aplicação"
            );

            err =
                esp_ota_mark_app_valid_cancel_rollback();

            if (err == ESP_OK)
            {
                ESP_LOGI(
                    TAG,
                    "Firmware marcado como válido"
                );

                ESP_LOGI(
                    TAG,
                    "Rollback cancelado"
                );
            }
            else
            {
                ESP_LOGE(
                    TAG,
                    "Falha ao marcar firmware como válido: %s",
                    esp_err_to_name(err)
                );
            }

            return err;
        }


        /* ----------------------------------------------------
         * Firmware reprovado
         * ---------------------------------------------------- */

        ESP_LOGE(
            TAG,
            "Firmware reprovado pela aplicação"
        );

        ESP_LOGW(
            TAG,
            "Executando rollback para firmware anterior"
        );

        /*
         * Esta função marca a imagem atual como inválida
         * e reinicia automaticamente o dispositivo.
         */
        return
            esp_ota_mark_app_invalid_rollback_and_reboot();
    }


    /* --------------------------------------------------------
     * Firmware não necessita validação
     * -------------------------------------------------------- */

    ESP_LOGI(
        TAG,
        "Firmware não possui validação pendente"
    );

    return ESP_OK;

#else

    ESP_LOGW(
        TAG,
        "Rollback não está habilitado no bootloader"
    );

    return ESP_OK;

#endif
}

/* ============================================================
 * API pública
 * ============================================================ */

esp_err_t ota_manager_init(
    ota_manager_validation_cb_t validation_cb)
{
    if (s_initialized)
    {
        ESP_LOGW(
            TAG,
            "OTA Manager já inicializado"
        );

        return ESP_OK;
    }


    /* --------------------------------------------------------
     * Salva callback da aplicação
     * -------------------------------------------------------- */

    s_validation_cb = validation_cb;


    /* --------------------------------------------------------
     * Registra eventos OTA
     * -------------------------------------------------------- */

    esp_err_t err =
        esp_event_handler_register(
            ESP_HTTPS_OTA_EVENT,
            ESP_EVENT_ANY_ID,
            ota_event_handler,
            NULL
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Falha ao registrar eventos OTA: %s",
            esp_err_to_name(err)
        );

        return err;
    }


    /* --------------------------------------------------------
     * Marca componente como inicializado
     * -------------------------------------------------------- */

    s_initialized = true;

    ESP_LOGI(
        TAG,
        "OTA Manager inicializado"
    );


    /* --------------------------------------------------------
     * Verifica se firmware aguarda validação
     * -------------------------------------------------------- */

    err = ota_manager_check_rollback();

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Falha na verificação de rollback: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    return ESP_OK;
}


esp_err_t ota_manager_start(void)
{
    if (!s_initialized)
    {
        ESP_LOGE(
            TAG,
            "OTA Manager não inicializado"
        );

        return ESP_ERR_INVALID_STATE;
    }

    if (s_ota_running)
    {
        ESP_LOGW(
            TAG,
            "OTA já está em andamento"
        );

        return ESP_ERR_INVALID_STATE;
    }

    s_ota_running = true;

    BaseType_t result =
        xTaskCreate(
            ota_manager_task,
            "ota_manager_task",
            8192,
            NULL,
            5,
            NULL
        );

    if (result != pdPASS)
    {
        s_ota_running = false;

        ESP_LOGE(
            TAG,
            "Falha ao criar task OTA"
        );

        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(
        TAG,
        "Task OTA criada"
    );

    return ESP_OK;
}


bool ota_manager_is_running(void)
{
    return s_ota_running;
}