/*
 * NOME: Adenilton Ribeiro
 * DATA: 13/03/2025
 * PROJETO: access point.c
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para criar um access point e abrir uma página HTML para configuração de Wi-Fi.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include "access_point.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (wifi-softAP)
static const char *TAG = "wifi-softAP";
//---variável htpp---
static httpd_handle_t server = NULL; 
//---variável para travar botão---
extern volatile bool ap_started;

// ========================================================================================================
/**
 * @brief Manipulador de eventos Wi-Fi
 * @param arg Argumento passado para o manipulador de eventos
 * @param event_base Base do evento (Wi-Fi)
 * @param event_id ID do evento (conexão ou desconexão de um dispositivo)
 * @param event_data Dados do evento (informações sobre o dispositivo conectado/desconectado)
 */
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "📶 Estação " MACSTR " conectada, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "🌐 Estação " MACSTR " desconectada, AID=%d, motivo=%d", MAC2STR(event->mac), event->aid, event->reason);
    }
}

// ========================================================================================================
/**
 * @brief Manipulador de requisições HTTP para a raiz
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Type", "text/html; charset=UTF-8");
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Manipulador de requisições HTTP para /connected_html
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t connected_html_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, connected_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Manipulador de requisições HTTP para /favicon.ico
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t favicon_handler(httpd_req_t *req) {
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Manipulador para Captive Portal Detection (Android)
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t captive_portal_detection_android_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found"); 
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Manipulador para Captive Portal Detection (iOS)
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t captive_portal_detection_ios_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Manipulador para Captive Portal Detection (Windows)
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t captive_portal_detection_windows_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Iniciar o servidor HTTP com suporte a Captive Portal Detection
 */
void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        //---registra o manipulador para a raiz---
        httpd_uri_t uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri);

        //---registra o manipulador para /wifi/connect---
        httpd_uri_t wifi_connect_uri = {
            .uri = "/wifi/connect",
            .method = HTTP_POST,
            .handler = wifi_connect_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &wifi_connect_uri);

        //---registra o manipulador para /connected_html---
        httpd_uri_t connected_html_uri = {
            .uri = "/connected_html",
            .method = HTTP_GET,
            .handler = connected_html_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &connected_html_uri);

        //---registra o manipulador para /close_ap---
        httpd_uri_t close_ap_uri = {
            .uri = "/close_ap",
            .method = HTTP_GET,
            .handler = close_ap_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &close_ap_uri);

        //---registra o manipulador para /favicon.ico---
        httpd_uri_t favicon_uri = {
            .uri = "/favicon.ico",
            .method = HTTP_GET,
            .handler = favicon_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &favicon_uri);

        //---registra o manipulador para /generate_204 (Android)---
        httpd_uri_t captive_portal_android_uri = {
            .uri = "/generate_204",
            .method = HTTP_GET,
            .handler = captive_portal_detection_android_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &captive_portal_android_uri);

        //---registra o manipulador para /hotspot-detect.html (iOS)---
        httpd_uri_t captive_portal_ios_uri = {
            .uri = "/hotspot-detect.html",
            .method = HTTP_GET,
            .handler = captive_portal_detection_ios_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &captive_portal_ios_uri);

        //---registra o manipulador para /connecttest.txt (Windows)---
        httpd_uri_t captive_portal_windows_uri = {
            .uri = "/connecttest.txt",
            .method = HTTP_GET,
            .handler = captive_portal_detection_windows_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &captive_portal_windows_uri);

        ESP_LOGI(TAG, "✅ Servidor HTTP com Captive Portal Detection iniciado");
    }
}

// ========================================================================================================
/**
 * @brief Iniciar o servidor DNS
 */
void start_dns_server(void) {
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

    if (ap_netif == NULL) {
        ESP_LOGE(TAG, "❌ Falha ao obter a interface de rede AP");
        return;
    }

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(ap_netif, &ip_info);

    //---configura o servidor DNS---
    ip_addr_t dns_addr;
    dns_addr.type = IPADDR_TYPE_V4;                            // Define o tipo de endereço como IPv4
    dns_addr.u_addr.ip4.addr = ip4_addr_get_u32(&ip_info.ip);  // Configura o endereço IP

    dns_setserver(0, &dns_addr);                               // Configura o servidor DNS

    ESP_LOGI(TAG, "✅ Servidor DNS iniciado");
}

// ========================================================================================================
/**
 * @brief Configurar e iniciar o Wi-Fi softAP
 */
// ========================================================================================================
/**
 * @brief Configurar e iniciar o Wi-Fi softAP
 */
void wifi_init_softap(void) {
    //---mostra o endereço de memória do server antes de tentar parar---
    ESP_LOGI(TAG, "💾 Endereço atual do server: %p", (void*)server);
    
    ESP_ERROR_CHECK(esp_netif_init());

    //---verifica se o loop de eventos já foi criado antes de criar um novo---
    esp_err_t err = esp_event_loop_create_default();
    if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "⚠️  Loop de eventos já criado, ignorando...");
    } else {
        ESP_ERROR_CHECK(err);
    }

    //---remover interfaces existentes antes de criar uma nova---
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif) {
        ESP_LOGW(TAG, "⚠️  Removendo interface de rede existente...");
        esp_netif_destroy(netif);
    }

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    //---configuração da rede Wi-Fi---
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = {0},                             // Inicializa o array ssid com zeros
            .password = {0},                         // Inicializa o array password com zeros
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .ssid_hidden = 0,                        // Certifique-se de que está definido como 0
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                .required = true,
            },
        },
    };

    //---copia o SSID e a senha para a configuração Wi-Fi---
    strncpy((char *)wifi_config.ap.ssid, EXAMPLE_ESP_WIFI_SSID, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid[sizeof(wifi_config.ap.ssid) - 1] = '\0'; // Garante terminação nula

    strncpy((char *)wifi_config.ap.password, EXAMPLE_ESP_WIFI_PASS, sizeof(wifi_config.ap.password) - 1);
    wifi_config.ap.password[sizeof(wifi_config.ap.password) - 1] = '\0'; // Garante terminação nula

    //---define o comprimento do SSID---
    wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);

    //---se a senha estiver em branco, define o modo de autenticação como aberto---
    if (strlen((char *)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    //---parar e reinicializar o Wi-Fi antes de configurar---
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "📶 wifi_init_softap finalizado. SSID:%s senha:%s canal:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);

    start_webserver();  // Iniciar o servidor HTTP após o Wi-Fi estar pronto
    start_dns_server(); // Iniciar o servidor DNS
}

// ========================================================================================================
/**
 * @brief Manipulador de requisições HTTP para /wifi/connect
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t wifi_connect_handler(httpd_req_t *req) {
    char buffer[128];
    int ret, remaining = req->content_len;

    //---lê os dados da requisição---
    if ((ret = httpd_req_recv(req, buffer, MIN(remaining, sizeof(buffer) - 1))) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req); // Responde com timeout se a leitura falhar
        }
        return ESP_FAIL;
    }

    //---cancela o timeout---
    wifi_manager_cancel_timeout();

    //---adiciona null-terminator ao buffer---
    buffer[ret] = '\0';

    //---converte os dados recebidos em uma string e imprime as informações---
    ESP_LOGI(TAG, "💾 Dados recebidos: %s", buffer);

    //---extrai o SSID e a senha do JSON recebido---
    cJSON *root = cJSON_Parse(buffer);
    if (root == NULL) {
        ESP_LOGE(TAG, "❌ Erro ao analisar JSON");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *password = cJSON_GetObjectItem(root, "password");

    if (ssid == NULL || password == NULL) {
        ESP_LOGE(TAG, "⚠️  JSON inválido: SSID ou senha ausentes");
        cJSON_Delete(root);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    //---salva o SSID e a senha na NVS---
    if (save_wifi_credentials(ssid->valuestring, password->valuestring) != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao salvar as credenciais na NVS");
        cJSON_Delete(root);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "📶 SSID: %s", ssid->valuestring);
    ESP_LOGI(TAG, "📶 Senha: %s", password->valuestring);

    cJSON_Delete(root);

    //---responde ao cliente que a conexão foi bem-sucedida---
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, "Credenciais salvas com sucesso!", HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Manipulador de requisições HTTP para /close_ap
 * @param req Requisição HTTP recebida
 * @return ESP_OK se a requisição foi tratada com sucesso
 */
esp_err_t close_ap_handler(httpd_req_t *req) {
    //---responde ao cliente antes de parar o servidor---
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, "AP e servidor fechados com sucesso!", HTTPD_RESP_USE_STRLEN);

    //---aguarda um curto período para garantir que a resposta seja enviada---
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "⚠️  Fechando Access Point e servidor HTTP...");

    //---reinicializa a variável ap_started---
    ap_started = false;
    ESP_LOGI(TAG, "✅ Tarefa do botão iniciada. Estado inicial de ap_started: %d", ap_started);

    //---para o servidor HTTP---
    stop_webserver();

    //---para a interface Wi-Fi---
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());

    //---aguarda 1 segundo para garantir que os sockets fecham corretamente---
    vTaskDelay(1000 / portTICK_PERIOD_MS); 

    ESP_LOGI(TAG, "⚠️  Access Point e servidor HTTP fechados.");

    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Para o servidor HTTP
 */
void stop_webserver(void) {
    if (server) {
        //---aguarda um curto período para garantir que a resposta seja enviada---
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "Parando o servidor HTTP...");

        //---mostra o endereço de memória do server antes de tentar parar---
        ESP_LOGI(TAG, "Endereço atual do server: %p", (void*)server);

        //---reiniciando em modo STA---
        reset_STA();

        //---para o servidor HTTP---
        httpd_stop(server);
        server = NULL;

        ESP_LOGI(TAG, "Servidor HTTP parado.");
    } else {
        ESP_LOGI(TAG, "Servidor HTTP já está parado.");
    }
}

// ========================================================================================================
/**
 * @brief Salva o SSID e a senha na NVS
 * @param ssid Nome da rede Wi-Fi (SSID)
 * @param password Senha da rede Wi-Fi
 * @return ESP_OK se os dados foram salvos com sucesso, ou um erro em caso de falha
 */
esp_err_t save_wifi_credentials(const char *ssid, const char *password) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    //---abre a partição NVS no modo de leitura e escrita---
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao abrir a NVS: %s", esp_err_to_name(err));
        return err;
    }

    //---apaga os dados antigos (se existirem)---
    nvs_erase_key(nvs_handle, "ssid");
    nvs_erase_key(nvs_handle, "password");

    //---salva o SSID na NVS---
    err = nvs_set_str(nvs_handle, "ssid", ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao salvar o SSID na NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    //---salva a senha na NVS---
    err = nvs_set_str(nvs_handle, "password", password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao salvar a senha na NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    //---confirma as alterações na NVS---
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao confirmar as alterações na NVS: %s", esp_err_to_name(err));
    }

    //---fecha a NVS---
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "✅ SSID e senha salvos na NVS com sucesso!");
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Recupera o SSID e a senha da NVS
 * @param ssid Buffer para armazenar o SSID recuperado
 * @param password Buffer para armazenar a senha recuperada
 * @return ESP_OK se os dados foram recuperados com sucesso, ou um erro em caso de falha
 */
esp_err_t load_wifi_credentials(char *ssid, size_t ssid_size, char *password, size_t password_size) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    //---abre a partição NVS no modo de leitura---
    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao abrir a NVS: %s", esp_err_to_name(err));
        return err;
    }

    //---recupera o SSID da NVS---
    err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao recuperar o SSID da NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    //---recupera a senha da NVS---
    err = nvs_get_str(nvs_handle, "password", password, &password_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Erro ao recuperar a senha da NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    //---Fecha a NVS---
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "✅ SSID e senha recuperados da NVS com sucesso!");
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Mostra o SSID e a senha salvos na NVS ao inicializar
 */
void show_saved_wifi_credentials(void) {
    char ssid[32] = {0};
    char password[64] = {0};

    //---recupera o SSID e a senha da NVS---
    if (load_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password)) == ESP_OK) {
        ESP_LOGI(TAG, "💾 SSID salvo: %s", ssid);
        ESP_LOGI(TAG, "💾 Senha salva: %s", password);
    } else {
        ESP_LOGI(TAG, "⚠️  Nenhum SSID ou senha salvo encontrado na NVS.");
    }
}