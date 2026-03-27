/* Exemplo avançado de OTA via HTTPS

   Este código de exemplo está em domínio público (ou licenciado como CC0).

   Salvo exigido por lei ou acordado por escrito, este software é distribuído
   "COMO ESTÁ", SEM GARANTIAS OU CONDIÇÕES DE QUALQUER TIPO.
*/
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif

#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif

#if CONFIG_BT_BLE_ENABLED || CONFIG_BT_NIMBLE_ENABLED
#include "ble_api.h"
#endif

static const char *TAG = "advanced_https_ota_example";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define OTA_URL_SIZE 256

/* Handler de eventos do sistema */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == ESP_HTTPS_OTA_EVENT) {
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
                ESP_LOGD(TAG, "Gravando na flash: %d written", *(int *)event_data);
                break;
            case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
                ESP_LOGI(TAG, "Partição de boot atualizada. Próxima partição: %d", *(esp_partition_subtype_t *)event_data);
                break;
            case ESP_HTTPS_OTA_FINISH:
                ESP_LOGI(TAG, "OTA finalizado");
                break;
            case ESP_HTTPS_OTA_ABORT:
                ESP_LOGI(TAG, "OTA abortado");
                break;
        }
    }
}

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Versão atual do firmware: %s", running_app_info.version);
    }

#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        ESP_LOGW(TAG, "A versão atual é igual à nova. Atualização não será realizada.");
        return ESP_FAIL;
    }
#endif

#ifdef CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    /**
     * A verificação segura da versão no cabeçalho da imagem do firmware impede o download e a gravação subsequentes na memória flash de
     * toda a imagem do firmware. No entanto, isso é opcional, pois também é tratado na API
     * esp_https_ota_finish ao final do procedimento de atualização OTA.
     */
    const uint32_t hw_sec_version = esp_efuse_read_secure_version();
    if (new_app_info->secure_version < hw_sec_version) {
        ESP_LOGW(TAG, "Versão de segurança do firmware é menor que a do eFuse, %"PRIu32" < %"PRIu32, new_app_info->secure_version, hw_sec_version);
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}

static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Descomente para adicionar cabeçalhos personalizados à solicitação HTTP */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}

void advanced_ota_example_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Iniciando exemplo avançado de OTA");

    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t config = {
        .url = CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL,
        .cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT,
        .keep_alive_enable = true,
    };

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
    char url_buf[OTA_URL_SIZE];
    if (strcmp(config.url, "FROM_STDIN") == 0) {
        example_configure_stdin_stdout();
        fgets(url_buf, OTA_URL_SIZE, stdin);
        int len = strlen(url_buf);
        url_buf[len - 1] = '\0';
        config.url = url_buf;
    } else {
        ESP_LOGE(TAG, "Mismatch na configuração: URL da imagem de atualização incorreta");
        abort();
    }
#endif

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .http_client_init_cb = _http_client_init_cb, // Registra uma função de retorno de chamada a ser invocada após a inicialização do esp_http_client
#ifdef CONFIG_EXAMPLE_ENABLE_PARTIAL_HTTP_DOWNLOAD
        .partial_http_download = true,
        .max_http_request_size = CONFIG_EXAMPLE_HTTP_REQUEST_SIZE,
#endif
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar OTA via HTTPS");
        vTaskDelete(NULL);
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao obter descrição da imagem");
        goto ota_end;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha na verificação do cabeçalho da imagem");
        goto ota_end;
    }

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        // esp_https_ota_perform retorna após cada operação de leitura, o que permite ao usuário
        // monitorar o status da atualização OTA chamando esp_https_ota_get_image_len_read, que fornece o comprimento da imagem
        // dados lidos até o momento.
        ESP_LOGD(TAG, "Bytes da imagem recebidos: %d", esp_https_ota_get_image_len_read(https_ota_handle));
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // A imagem OTA não foi recebida completamente e o usuário pode personalizar a resposta para essa situação.
        ESP_LOGE(TAG, "Dados completos não foram recebidos.");
    } else {
        ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
            ESP_LOGI(TAG, "OTA via HTTPS concluído com sucesso. Reiniciando...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        } else {
            if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
                ESP_LOGE(TAG, "Validação da imagem falhou, imagem corrompida");
            }
            ESP_LOGE(TAG, "Falha no OTA via HTTPS 0x%x", ota_finish_err);
            vTaskDelete(NULL);
        }
    }

ota_end:
    esp_https_ota_abort(https_ota_handle);
    ESP_LOGE(TAG, "Falha no processo de OTA via HTTPS");
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Inicializando aplicação OTA");
    // Inicializa NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 1. A tabela de partições do aplicativo OTA tem um tamanho de partição NVS menor do que a tabela de partições não OTA.
        // Essa incompatibilidade de tamanho pode causar falha na inicialização do NVS.
        // 2. A partição NVS contém dados em um novo formato e não pode ser reconhecida por esta versão do código.
        // Se isso acontecer, apagamos a partição NVS e inicializamos o NVS novamente.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    /* Esta função auxiliar configura Wi-Fi ou Ethernet, conforme selecionado em menuconfig.
     * Leia a seção "Estabelecendo conexão Wi-Fi ou Ethernet" em
     * examples/protocols/README.md para obter mais informações sobre esta função.
     */
    ESP_ERROR_CHECK(example_connect());

#if defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE)
    /**
     * Estamos tratando a conexão Wi-Fi bem-sucedida como um ponto de verificação para cancelar o processo de reversão
     * e marcar a imagem de firmware recém-atualizada como ativa. Para casos de produção,
     * ajuste o comportamento do ponto de verificação de acordo com os requisitos da aplicação final.
     */
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
                ESP_LOGI(TAG, "Aplicação validada, rollback cancelado com sucesso");
            } else {
                ESP_LOGE(TAG, "Falha ao cancelar rollback");
            }
        }
    }
#endif

#if CONFIG_EXAMPLE_CONNECT_WIFI
#if !CONFIG_BT_ENABLED
    /* Certifique-se de desativar qualquer modo de economia de energia do Wi-Fi. Isso permite a melhor taxa de transferência
     * e, consequentemente, os melhores tempos para a operação geral do OTA.
     */
    esp_wifi_set_ps(WIFI_PS_NONE);
#else
    /* WIFI_PS_MIN_MODEM é o modo padrão para economia de energia do Wi-Fi. 
     * Quando tanto o Wi-Fi quanto o Bluetooth estão em execução, o modem Wi-Fi 
     * precisa ser desligado, portanto, precisamos do WIFI_PS_MIN_MODEM. E, à 
     * medida que o modem Wi-Fi é desligado, o tempo de download OTA aumenta.
     */
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
#endif // CONFIG_BT_ENABLED
#endif // CONFIG_EXAMPLE_CONNECT_WIFI

#if CONFIG_BT_CONTROLLER_ENABLED && (CONFIG_BT_BLE_ENABLED || CONFIG_BT_NIMBLE_ENABLED)
    ESP_ERROR_CHECK(esp_ble_helper_init());
#endif

    xTaskCreate(&advanced_ota_example_task, "advanced_ota_example_task", 1024 * 8, NULL, 5, NULL);
}
