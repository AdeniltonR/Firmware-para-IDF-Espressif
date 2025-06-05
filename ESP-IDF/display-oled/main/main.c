/*
 * NOME: Adenilton Ribeiro
 * DATA: 04/06/2025
 * PROJETO: Display Oled
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Implementa interface HAL para displays OLED com controlador SSD1306/SSD1309/SSD1325/SSD1327/SSD1331/SSD1351/SSD1362
 *            - feat: Suporte para comunicação via I2C e SPI
 *            - feat: Configuração de pinos GPIO para reset e DC
 *            - docs: Compatível com ESP32-S3 - ESP-IDF v5.4.0
 *            - docs: Baseado na biblioteca u8g2 (https://github.com/olikraus/u8g2)
 * LINKS: - Documentação u8g2: https://github.com/olikraus/u8g2/wiki
 *        - Datasheet ESP32: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_pt.pdf
*/

// ========================================================================================================
//---BIBLIOTECAS---

#include <stdio.h>
#include <string.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <u8g2.h>
#include "display_oled.h"

// ========================================================================================================
//---MAPEAMENTO DE HARDWARE---

#define PIN_sda 8   // Pino de dados I2C (Serial Data)
#define PIN_scl 9   // Pino de clock I2C (Serial Clock)

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (main)
static const char *TAG = "main";

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

void task_test_SSD1306i2c(void* pvParameter);

// ========================================================================================================
/**
 * @brief Void main
 *
*/
void app_main(void) {

    //---criando a tarefa RTOS para exibir display oled---
    xTaskCreate(&task_test_SSD1306i2c, "oled_test", 4096, NULL, 5, NULL);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ========================================================================================================
/**
 * @brief Tarefa de teste para o display SSD1306 via I2C
 * 
 * @param ignore - Parâmetro não utilizado (necessário para assinatura da tarefa)
 * 
 * @note
 *   1. Configura a interface HAL com os pinos definidos
 *   2. Inicializa a estrutura u8g2 para display SSD1306 128x32
 *   3. Realiza diversas operações gráficas de teste
 *   4. Exibe uma mensagem no display
*/
void task_test_SSD1306i2c(void* pvParameter) {
    //---configuração inicial da HAL com valores padrão---
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    
    //---define os pinos I2C---
    u8g2_esp32_hal.bus.i2c.sda = PIN_sda;      // Configura pino SDA
    u8g2_esp32_hal.bus.i2c.scl = PIN_scl;      // Configura pino SCL
    
    //---inicializa a HAL com as configurações---
    u8g2_esp32_hal_init(u8g2_esp32_hal);

    //---estrutura principal do u8g2---
    u8g2_t u8g2;
    
    //---configura o display SSD1306 128x32 com comunicação I2C---
    u8g2_Setup_ssd1306_i2c_128x32_univision_f(
        &u8g2,                                 // Ponteiro para estrutura u8g2
        U8G2_R0,                               // Orientação (0 graus)
        u8g2_esp32_i2c_byte_cb,                // Callback para comunicação I2C
        u8g2_esp32_gpio_and_delay_cb           // Callback para GPIO e delays
    );
    
    //---define endereço I2C do display (0x3C << 1 = 0x78)---
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);

    //---sequência de inicialização do display---
    ESP_LOGI(TAG, "✅ Iniciando display...");
    u8g2_InitDisplay(&u8g2);                   // Inicializa hardware
    u8g2_SetPowerSave(&u8g2, 0);               // Desativa modo de economia de energia
    
    //---operações gráficas---
    ESP_LOGI(TAG, "⚙️  Preparando buffer gráfico...");
    u8g2_ClearBuffer(&u8g2);                   // Limpa o buffer interno
    
    //---desenha elementos gráficos---
    u8g2_DrawBox(&u8g2, 0, 26, 80, 6);         // Caixa preenchida
    u8g2_DrawFrame(&u8g2, 0, 26, 100, 6);      // Moldura
    
    //---configura fonte e texto---
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr); // Fonte medium
    u8g2_DrawStr(&u8g2, 2, 17, "CEDEPS"); // Texto na posição (2,17)
    
    //---envia o buffer para o display---
    u8g2_SendBuffer(&u8g2);
    ESP_LOGI(TAG, "✅ Teste concluído com sucesso!");

    //---finaliza a tarefa---
    vTaskDelete(NULL);
}
