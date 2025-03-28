/*
 * NOME: Adenilton Ribeiro
 * DATA: 19/03/2025
 * PROJETO: DFPlayer Mini
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Controle do módulo DFPlayer Mini.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS: Link de referência - https://github.com/nopnop2002/esp-idf-DFPlayerMini/tree/main
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "DFRobotDFPlayerMini.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define PIN_rx 4      // Pino RX do ESP32 conectado ao TX do DFPlayer Mini
#define PIN_tx 18     // Pino TX do ESP32 conectado ao RX do DFPlayer Mini
#define PIN_en 16     // Pino para ativar o módulo DFPlayer Mini

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

//// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";

//---controle de volume---
int DEFAULT_VOLUME = 15;

// ========================================================================================================
//---PROTOTIPO DA FUNCAO--- 

void enable_dfplayer();

// ========================================================================================================
/**
 * @brief Void main
 *
 */
void app_main() {
    //---ativa o módulo DFPlayer Mini---
    enable_dfplayer();

    //---configuração do modo de depuração---
    bool debug = false;
#if CONFIG_DEBUG_MODE
    debug = true;  // Habilita o modo de depuração se CONFIG_DEBUG_MODE estiver definido
#endif

    //---inicializa o DFPlayer Mini---
    bool ret = DF_begin(PIN_tx, PIN_rx, true, true, debug);
    ESP_LOGI(TAG, "✅ DF_begin=%d", ret);  // Log do resultado da inicialização

    //---verifica se a inicialização foi bem-sucedida---
    if (!ret) {
        ESP_LOGE(TAG, "🔇 DFPlayer Mini offline."); 
        while (1) {
            vTaskDelay(1); 
        }
    }
    ESP_LOGI(TAG, "✅ DFPlayer Mini online.");  

    //---define o volume (0 a 30)---
    DF_volume(DEFAULT_VOLUME); 
    ESP_LOGI(TAG, "🔊 Volume definido para %d", DEFAULT_VOLUME);

    //---lê o número de músicas disponíveis no cartão SD---
    int num_musicas = DF_readFileCounts(DFPLAYER_DEVICE_SD);
    if (num_musicas > 0) {
        ESP_LOGI(TAG, "🎵 Músicas encontradas: %d | 📂 SD Card OK", num_musicas);  
    } else {
        ESP_LOGE(TAG, "❌ Nenhuma música encontrada no cartão SD!");  
        return; 
    }

    ESP_LOGI(TAG, "✅ Sistema inicializado com sucesso!");

    /*
    Espera até que a reprodução termine.
    Por algum motivo, quando a reprodução termina, o evento "Play Finished" é notificado duas vezes.
    Não sei por quê.

    Exemplo de mensagem recebida:
    received:7E FF 6 3D 0 0 1 FE BD EF
    Number:1 Play Finished!
    received:7E FF 6 3D 0 0 1 FE BD EF
    Number:1 Play Finished!
    */

    //---toca a segunda música---
    DF_play(1);  
    ESP_LOGI(TAG, "▶️ Tocando a segunda música...");

    while (1) {
        if (DF_available()) {              // Verifica se há eventos disponíveis
            uint8_t type = DF_readType();  // Lê o tipo do evento
            int value = DF_read();         // Lê o valor associado ao evento

            //---exibe detalhes do evento (erros, término de reprodução, etc.)---
            DF_printDetail(type, value);
        }
        vTaskDelay(1);  
    }
}

// ========================================================================================================
/**
 * @brief Função para ativar o DFPlayer Mini
 *
 */
void enable_dfplayer() {
    //---configura o pino de energia como saída---
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;          // Desabilita interrupções
    io_conf.mode = GPIO_MODE_OUTPUT;                // Configura o pino como saída
    io_conf.pin_bit_mask = (1ULL << PIN_en);        // Define o pino PIN_en como saída
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;   // Desabilita pull-down
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;       // Desabilita pull-up
    gpio_config(&io_conf);                          // Aplica a configuração

    //---ativa o módulo (HIGH) e espera 1 segundo para estabilizar---
    gpio_set_level(PIN_en, 1);  
    ESP_LOGI(TAG, "✅ Enable ativado!"); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);  
}