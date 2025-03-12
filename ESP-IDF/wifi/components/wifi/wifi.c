#include "wifi.h"

/* Grupo de eventos do FreeRTOS para sinalizar quando estamos conectados */
static EventGroupHandle_t s_wifi_event_group;

/* Número de tentativas de reconexão */
static int s_retry_num = 0;

/* Função de tratamento de eventos */
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    /* Se o evento for de início do Wi-Fi (WIFI_EVENT_STA_START), tenta conectar */
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    /* Se o evento for de desconexão (WIFI_EVENT_STA_DISCONNECTED), tenta reconectar */
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "tentando reconectar ao AP");
        } else {
            /* Se exceder o número máximo de tentativas, sinaliza falha */
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "falha ao conectar ao AP");
    } 
    /* Se o evento for de obtenção de IP (IP_EVENT_STA_GOT_IP), sinaliza sucesso */
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "obteve ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* Função para inicializar o Wi-Fi no modo estação (STA) */
void wifi_init_sta(void)
{
    /* Cria um grupo de eventos */
    s_wifi_event_group = xEventGroupCreate();

    /* Inicializa a pilha de rede */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Cria o loop de eventos padrão */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    /* Configuração inicial do Wi-Fi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Registra os manipuladores de eventos para eventos de Wi-Fi e IP */
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

    /* Configuração da rede Wi-Fi */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,  /* SSID da rede */
            .password = EXAMPLE_ESP_WIFI_PASS,  /* Senha da rede */
            /* O modo de autenticação padrão é WPA2 se a senha atender aos padrões WPA2 (tamanho >= 8).
             * Se você quiser conectar o dispositivo a redes WEP/WPA obsoletas, defina o valor de limite
             * para WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK e defina a senha com o comprimento e formato correspondentes
             * aos padrões WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    /* Define o modo Wi-Fi como estação (STA) */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    /* Aplica a configuração Wi-Fi */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    /* Inicia o Wi-Fi */
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finalizado.");

    /* Aguarda até que a conexão seja estabelecida (WIFI_CONNECTED_BIT) ou falhe após o número máximo de tentativas (WIFI_FAIL_BIT).
     * Os bits são definidos por event_handler() (veja acima) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() retorna os bits antes da chamada retornar, então podemos testar qual evento realmente aconteceu. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "conectado ao AP SSID:%s senha:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Falha ao conectar ao SSID:%s, senha:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "EVENTO INESPERADO");
    }
}