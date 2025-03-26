/*
 * NOME: Adenilton
 * DATA: 16/03/2025
 * PROJETO: Wi-Fi
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Conexão de wifi com teste de internet com horário.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS: 
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include "wifi.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

//---tag para identificação nos logs---
static const char *TAG_STA = "wifi station";
//---grupo de eventos do FreeRTOS para sinalizar quando estamos conectados---
static EventGroupHandle_t s_wifi_event_group;
//---número de tentativas de reconexão---
static int s_retry_num = 0;

// ========================================================================================================
/**
 * @brief Manipulador de eventos para o Wi-Fi.
 * 
 * @param arg Ponteiro para argumentos do evento.
 * @param event_base Tipo de evento (WIFI_EVENT ou IP_EVENT).
 * @param event_id ID do evento específico.
 * @param event_data Dados associados ao evento.
 * 
 * @note Este manipulador trata eventos de conexão, desconexão e obtenção de IP.
 */
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    //---se o evento for de início do Wi-Fi (WIFI_EVENT_STA_START), tenta conectar---
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 

    //---se o evento for de desconexão (WIFI_EVENT_STA_DISCONNECTED), tenta reconectar---
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_STA, "tentando reconectar ao AP");
        } else {
            //---se exceder o número máximo de tentativas, sinaliza falha---
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGW(TAG_STA, "Número máximo de tentativas excedido. Reiniciando o ESP32...");
            
            //---1 para abilitar ele entrar no modo AP, 0 para ESP32 só reiniciar---
            if (NUMERO_MAX_TENTATIVAS) {
                //---reiniciando em modo AP---
                reset_AP();
            } else {
                //---reinicia o ESP32---
                ESP_LOGW(TAG_MG, "Reiniciando o ESP32...");
                esp_restart();
            }
        }
        ESP_LOGI(TAG_STA, "falha ao conectar ao AP");
    } 
    //---se o evento for de obtenção de IP (IP_EVENT_STA_GOT_IP), sinaliza sucesso---
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_STA, "obteve ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ========================================================================================================
/**
 * @brief Inicializa o Wi-Fi no modo estação (STA).
 * 
 * @note A função configura o Wi-Fi, registra manipuladores de eventos e tenta conectar à rede.
 * @note O ESP32 tentará reconectar automaticamente em caso de falha, até atingir o número máximo de tentativas.
 */
void wifi_init_sta(void) {
    char ssid[32] = {0};      // Buffer para armazenar o SSID
    char password[64] = {0};  // Buffer para armazenar a senha

    //---recupera as credenciais da NVS---
    if (load_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password)) != ESP_OK) {
        ESP_LOGE(TAG_STA, "Falha ao carregar credenciais da NVS. Usando valores padrão.");
        strncpy(ssid, EXAMPLE_ESP_WIFI_SSID, sizeof(ssid));  
        strncpy(password, EXAMPLE_ESP_WIFI_PASS, sizeof(password));
    }

    //---erifica se as credenciais são válidas---
    if (strlen(ssid) == 0 || strlen(password) == 0) {
        ESP_LOGW(TAG_STA, "Credenciais Wi-Fi inválidas! SSID ou senha estão vazios.");
        //---reiniciando em modo STA---
        reset_STA();

        return;
    }

    //---cria um grupo de eventos para sincronizar a conexão Wi-Fi---
    s_wifi_event_group = xEventGroupCreate();

    //---inicializa a pilha TCP/IP---
    ESP_ERROR_CHECK(esp_netif_init());

    //---cria o loop de eventos padrão---
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //---cria a interface de rede Wi-Fi no modo estação (STA)---
    esp_netif_create_default_wifi_sta();

    //---configuração básica do Wi-Fi---
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //---registra os manipuladores de eventos para Wi-Fi e IP---
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

    //---configuração da rede Wi-Fi---
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",      
            .password = "",
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };

    //---copia as credenciais recuperadas para a estrutura wifi_config---
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    //---define o modo Wi-Fi como estação (STA)---
    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_STA, "Erro ao definir o modo Wi-Fi: %s", esp_err_to_name(ret));
        return;
    }

    //---aplica a configuração Wi-Fi---
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_STA, "Erro ao aplicar a configuração Wi-Fi: %s", esp_err_to_name(ret));
        return;
    }

    //---inicia o Wi-Fi---
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_STA, "Erro ao iniciar o Wi-Fi: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG_STA, "wifi_init_sta finalizado.");

    //---aguarda até que a conexão seja estabelecida (WIFI_CONNECTED_BIT) ou falhe após o número máximo de tentativas (WIFI_FAIL_BIT)---
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    //---verifica qual evento ocorreu---
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_STA, "Conectado ao AP SSID:%s senha:%s", ssid, password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG_STA, "Falha ao conectar ao SSID:%s, senha:%s", ssid, password);
    } else {
        ESP_LOGE(TAG_STA, "EVENTO INESPERADO");
    }
}

// ========================================================================================================
/**
 * @brief Inicialização do fuso horáio.
 * 
 */
void initialize_hora(void) {
    //---configura o fuso horário---
    set_timezone();

    //---inicializa o SNTP (apenas uma vez)---
    initialize_sntp();
}

// ========================================================================================================
/**
 * @brief Inicializa o SNTP para sincronização do tempo via internet.
 * @note O servidor utilizado é "pool.ntp.org".
 */
void initialize_sntp(void) {
    ESP_LOGI(TAG_STA, "Inicializando SNTP...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

// ========================================================================================================
/**
 * @brief Configura o fuso horário do sistema para Brasília (BRT/BRST).
 * @note O fuso horário segue o padrão TZ.
 */
void set_timezone(void) {
    //---configura o fuso horário para Brasília---
    setenv("TZ", "BRT3BRST,M10.3.0/0,M2.3.0/0", 1);  
    tzset();
}

// ========================================================================================================
/**
 * @brief Obtém a hora atual da internet, formatada como string.
 * 
 * @return Ponteiro para uma string contendo a hora atual (YYYY-MM-DD HH:MM:SS).
 * @note A memória alocada deve ser liberada pelo chamador usando `free()`.
 */
char* get_current_time(void) {
    time_t now;
    struct tm timeinfo;
    //---aloca memória para a string de tempo---
    char *time_str = (char *)malloc(64 * sizeof(char));  

    //---obtém o tempo atual---
    time(&now);
    localtime_r(&now, &timeinfo);

    //---verifica se o tempo foi sincronizado---
    if (timeinfo.tm_year < (2024 - 1900)) {
        ESP_LOGE(TAG_STA, "Tempo não sincronizado. Verifique a conexão com a internet.");
        strcpy(time_str, "Tempo não disponível");
        return time_str;
    }

    //---formata o tempo como uma string---
    strftime(time_str, 64, "%Y-%m-%d %H:%M:%S", &timeinfo);
    return time_str;
}

// ========================================================================================================
/**
 * @brief Testa a conexão com o NTP e obtém a hora sincronizada.
 * 
 * @note Aguarda a sincronização do tempo antes de exibir a hora.
 */
void test_ntp_connection(void) {
    ESP_LOGI(TAG_STA, "Aguardando sincronização do tempo...");
    int retry = 0;
    const int retry_count = 10;
    
    //---aguarda a sincronização do tempo, com limite de tentativas---
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG_STA, "Aguardando... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);  // Aguarda 2 segundos
    }

    //---obtém a hora atual---
    char *current_time = get_current_time();
    ESP_LOGI(TAG_STA, "Hora atual: %s", current_time);

    //---libera a memória alocada para a string de tempo---
    free(current_time);
}
