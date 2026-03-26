/*
 * NOME: Adenilton Ribeiro
 * DATA: 19/03/2026
 * PROJETO: WIFI
 * VERSAO: 1.0.0
 * DESCRICAO:
 *  - feat: Conexão Wi-Fi com suporte a DHCP ou IP fixo configurável via menuconfig.
 * LINKS:
 *  - ESP-IDF: https://docs.espressif.com/projects/esp-idf/en/v5.4/
 *  - Driver câmera: https://github.com/espressif/esp32-camera
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include "connect_wifi.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

// Status da conexão Wi-Fi (0 = desconectado, 1 = conectado)
int wifi_connect_status = 0;

/// @brief Tag para identificação dos logs deste módulo (WiFi)
static const char *TAG = "WiFi";

// Contador de tentativas de conexão
int s_retry_num = 0;

// Configurações de Wi-Fi (definidas no menuconfig)
// #define WIFI_SSID "replace_with_your_ssid"
// #define WIFI_PASSWORD "replace_with_your_password"
// #define MAXIMUM_RETRY 5

// Usando as configurações do menuconfig
#define WIFI_SSID CONFIG_ESPCAM_WIFI_SSID
#define WIFI_PASSWORD CONFIG_ESPCAM_WIFI_PASSWORD
#define MAXIMUM_RETRY CONFIG_ESPCAM_MAXIMUM_RETRY

/* Grupo de eventos do FreeRTOS para sinalizar status do Wi-Fi */
EventGroupHandle_t s_wifi_event_group;

/* O grupo de eventos permite múltiplos bits para cada evento, mas só nos interessam dois eventos:
 * - estamos conectados ao ponto de acesso com um endereço IP
 * - falhamos na conexão após o número máximo de tentativas */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// ========================================================================================================
/**
 * @brief Handler para eventos de Wi-Fi e IP
 * @param arg Argumento do evento (não utilizado)
 * @param event_base Base do evento (WIFI_EVENT ou IP_EVENT)
 * @param event_id ID do evento específico
 * @param event_data Dados adicionais do evento (ex: endereço IP obtido)
 */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    // Evento: Wi-Fi iniciado → tenta conectar
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "✅ Iniciando conexão com Wi-Fi...");
        esp_wifi_connect();

    // Evento: desconectado
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "🔄 Tentando reconectar... (%d/%d)", s_retry_num, MAXIMUM_RETRY);
        } else {
            // Falha após várias tentativas
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        wifi_connect_status = 0;
        ESP_LOGE(TAG, "❌  Falha na conexão com o Wi-Fi");

    // Evento: IP obtido com sucesso
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "📶  Wi-Fi conectado! IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));

        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        wifi_connect_status = 1;
    }
}

// ========================================================================================================
/**
 * @brief Configura e inicia a conexão Wi-Fi
 */
void connect_wifi(void) {
    // Cria grupo de eventos
    s_wifi_event_group = xEventGroupCreate();

    // Inicializa pilha de rede
    ESP_ERROR_CHECK(esp_netif_init());

    // Cria loop de eventos padrão
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Cria interface Wi-Fi (modo station)
    esp_netif_create_default_wifi_sta();

    // Configuração padrão do Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Registro de eventos
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // Configuração da rede Wi-Fi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            /* Definir uma senha implica que a estação se conectará a todos os modos de segurança, incluindo WEP/WPA.
             * No entanto, esses modos estão obsoletos e não é recomendável usá-los. Caso seu ponto de acesso
             * não suporte WPA2, esses modos podem ser ativados comentando a linha abaixo */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    // Define modo STA (cliente)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Aplica configuração
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

// Configura IP fixo (opcional, baseado em configurações do menuconfig)
#if CONFIG_ESPCAM_USE_STATIC_IP

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    // Para DHCP
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(netif));

    esp_netif_ip_info_t ip_info;

    // Conversão correta (ESP-IDF 5.x)
    ESP_ERROR_CHECK(esp_netif_str_to_ip4(CONFIG_ESPCAM_STATIC_IP, &ip_info.ip));
    ESP_ERROR_CHECK(esp_netif_str_to_ip4(CONFIG_ESPCAM_GATEWAY, &ip_info.gw));
    ESP_ERROR_CHECK(esp_netif_str_to_ip4(CONFIG_ESPCAM_NETMASK, &ip_info.netmask));

    // Aplica IP
    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip_info));

    ESP_LOGI(TAG, "IP fixo configurado: %s", CONFIG_ESPCAM_STATIC_IP);

#endif

    // Inicia Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "🔄 Wi-Fi inicializado, tentando conectar...");

    /* Aguarda até que a conexão seja estabelecida (WIFI_CONNECTED_BIT) ou que a conexão falhe pelo número máximo de tentativas
     * tentativas (WIFI_FAIL_BIT). Os bits são definidos por event_handler() (veja acima) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() retorna os bits anteriores ao retorno da chamada, portanto podemos testar qual evento realmente
     * ocorreu. */
     // Verifica resultado
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "📶  Conectado com sucesso ao Wi-Fi: %s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "❌  Falha ao conectar no Wi-Fi: %s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "❌  Evento inesperado durante conexão Wi-Fi");
    }

    // Libera grupo de eventos
    vEventGroupDelete(s_wifi_event_group);
}