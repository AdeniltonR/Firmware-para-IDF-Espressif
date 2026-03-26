/*
 * NOME: Adenilton Ribeiro
 * DATA: 19/03/2026
 * PROJETO: ESP32-CAM Web Server
 * VERSAO: 1.0.0
 * DESCRICAO:
 *  - feat: Implementação de câmera IP utilizando ESP32-CAM com stream MJPEG via HTTP.
 *  - feat: Conexão Wi-Fi com suporte a DHCP ou IP fixo configurável via menuconfig.
 *  - feat: Inicialização e configuração do sensor de câmera (OV2640).
 *  - feat: Servidor HTTP embarcado para transmissão de vídeo em tempo real.
 *  - feat: Suporte a configuração de parâmetros via menuconfig (Wi-Fi, rede, placa).
 *  - docs: Firmware desenvolvido utilizando ESP-IDF v5.4 com FreeRTOS.
 * LINKS:
 *  - ESP-IDF: https://docs.espressif.com/projects/esp-idf/en/v5.4/
 *  - Driver câmera: https://github.com/espressif/esp32-camera
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include <esp_system.h>
#include <nvs_flash.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_netif.h"

#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "camera_pins.h"
#include "connect_wifi.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";

// Definições para stream MJPEG
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// Frequência do clock da câmera
#define CONFIG_XCLK_FREQ 20000000 

// ========================================================================================================
/**
 * @brief Inicializa a câmera
 * @note Certifique-se de configurar os pinos corretamente no arquivo "camera_pins.h" para o seu modelo de câmera específico.
 * @return esp_err_t 
 */
static esp_err_t init_camera(void) {
    camera_config_t camera_config = {
        // Pinos de controle
        .pin_pwdn  = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,

        // Pinos de dados
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        // Configurações de clock
        .xclk_freq_hz = CONFIG_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        // Formato e resolução
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_VGA,

        // Qualidade da imagem (menor = melhor qualidade)
        .jpeg_quality = 10,

        // Número de buffers (1 = menor uso de memória)
        .fb_count = 1,

        // Modo de captura
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY //CAMERA_GRAB_LATEST. Define quando os buffers devem ser preenchidos.
    };

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌  Erro ao inicializar a câmera");
        return err;
    }

    ESP_LOGI(TAG, "✅ Câmera inicializada com sucesso...");
    return ESP_OK;
}

// ========================================================================================================
/**
 * @brief Handler para o stream MJPEG
 * @param req Ponteiro para a requisição HTTP
 * @return esp_err_t 
 */
esp_err_t jpg_stream_httpd_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;

    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;

    char part_buf[64];
    static int64_t last_frame = 0;

    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    // Define tipo de conteúdo HTTP (stream MJPEG)
    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) {
        return res;
    }

    // Loop contínuo de captura e envio de frames
    while(true) {
        // Captura frame da câmera
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "❌  Falha ao capturar imagem da câmera");
            res = ESP_FAIL;
            break;
        }

        // Se não estiver em JPEG, converte
        if(fb->format != PIXFORMAT_JPEG) {
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);

            if(!jpeg_converted) {
                ESP_LOGE(TAG, "❌  Falha na compressão JPEG");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        // Envia boundary
        if(res == ESP_OK) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }

        // Envia cabeçalho da imagem
        if(res == ESP_OK) {
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }

        // Envia imagem
        if(res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }

        // Libera memória se necessário
        if(fb->format != PIXFORMAT_JPEG) {
            free(_jpg_buf);
        }

        esp_camera_fb_return(fb);

        if(res != ESP_OK) {
            break;
        }

        // Cálculo de FPS
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;

        frame_time /= 1000; // microsegundos → ms

        ESP_LOGI(TAG, "Frame: %luKB | Tempo: %lums | FPS: %.1f",
            (uint32_t)(_jpg_buf_len/1024),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}

// ========================================================================================================
/**
 * @brief Configura e inicia o servidor HTTP para streaming MJPEG
 * 
 */
httpd_uri_t uri_get = {
    .uri = "/", // endpoint da câmera
    .method = HTTP_GET,
    .handler = jpg_stream_httpd_handler,
    .user_ctx = NULL};

// ========================================================================================================
/**
 * @brief Configura e inicia o servidor HTTP
 */
httpd_handle_t setup_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t stream_httpd  = NULL;

    if (httpd_start(&stream_httpd , &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd , &uri_get);
        ESP_LOGI(TAG, "✅ Servidor HTTP iniciado...");
    } else {
        ESP_LOGE(TAG, "❌  Erro ao iniciar servidor HTTP");
    }

    return stream_httpd;
}

// ========================================================================================================
/**
 * @brief Função principal do aplicativo
 */
void app_main() {
    esp_err_t err;

    // Inicializa memória NVS (necessária para Wi-Fi)
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    // Conecta no Wi-Fi
    connect_wifi();

    if (wifi_connect_status) {
        ESP_LOGI(TAG, "📶  Wi-Fi conectado com sucesso...");

        // Inicializa câmera
        err = init_camera();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "❌  Falha na inicialização da câmera: %s", esp_err_to_name(err));
            return;
        }

        // Inicia servidor
        setup_server();
        
        ESP_LOGI(TAG, "🌐  Servidor da câmera iniciado com sucesso...");
        esp_netif_ip_info_t ip_info;
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

        if (netif != NULL) {
            esp_netif_get_ip_info(netif, &ip_info);

            ESP_LOGI(TAG, "🌐  Acesse via navegador: http://" IPSTR, IP2STR(&ip_info.ip));
        }
    } else {
        ESP_LOGE(TAG, "❌  Falha ao conectar no Wi-Fi. Verifique SSID e senha.");
    }
}