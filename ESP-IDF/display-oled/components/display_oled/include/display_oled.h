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
/**
 * @brief display_oled.h
 * 
*/
#ifndef DISPLAY_OLED_H
#define DISPLAY_OLED_H

// ========================================================================================================
// ---BIBLIOTECA---

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "u8g2.h"

// ========================================================================================================
//---MAPEAMENTO DE ESTADO---

#define U8G2_ESP32_HAL_UNDEFINED GPIO_NUM_NC

#if SOC_I2C_NUM > 1
#define I2C_MASTER_NUM I2C_NUM_1     //  Número da porta I2C para dispositivo mestre
#else
#define I2C_MASTER_NUM I2C_NUM_0     //  Número da porta I2C para dispositivo mestre
#endif

#define I2C_MASTER_TX_BUF_DISABLE 0  //  Mestre I2C não precisa de buffer de transmissão
#define I2C_MASTER_RX_BUF_DISABLE 0  //  Mestre I2C não precisa de buffer de recepção
#define I2C_MASTER_FREQ_HZ 50000     //  Frequência de clock do mestre I2C
#define ACK_CHECK_EN 0x1             //  Mestre I2C verificará ACK do escravo
#define ACK_CHECK_DIS 0x0            //  Mestre I2C não verificará ACK do escravo

/** @public
 * Estrutura de configuração HAL.
*/
typedef struct {
  union {
    /* Configurações SPI. */
    struct {
      /* Número GPIO para clock. */
      gpio_num_t clk;
      /* Número GPIO para MOSI (Master Out Slave In). */
      gpio_num_t mosi;
      /* Número GPIO para slave/chip select. */
      gpio_num_t cs;
    } spi;
    /* Configurações I2C. */
    struct {
      /* Número GPIO para dados I2C. */
      gpio_num_t sda;
      /* Número GPIO para clock I2C. */
      gpio_num_t scl;
    } i2c;
  } bus;
  /* Número GPIO para reset. */
  gpio_num_t reset;
  /* Número GPIO para DC (Data/Command). */
  gpio_num_t dc;
} u8g2_esp32_hal_t;

/**
 * Constrói uma configuração HAL padrão com todos os campos indefinidos.
*/
#define U8G2_ESP32_HAL_DEFAULT                                        \
  {                                                                   \
    .bus = {.spi = {.clk = U8G2_ESP32_HAL_UNDEFINED,                  \
                    .mosi = U8G2_ESP32_HAL_UNDEFINED,                 \
                    .cs = U8G2_ESP32_HAL_UNDEFINED}},                 \
    .reset = U8G2_ESP32_HAL_UNDEFINED, .dc = U8G2_ESP32_HAL_UNDEFINED \
  }

// ========================================================================================================
//---PROTOTIPO DA FUNCAO---

/**
 * Inicializa o HAL com a configuração fornecida.
 *
 * @see u8g2_esp32_hal_t
 * @see U8G2_ESP32_HAL_DEFAULT
*/
void u8g2_esp32_hal_init(u8g2_esp32_hal_t u8g2_esp32_hal_param);
uint8_t u8g2_esp32_spi_byte_cb(u8x8_t* u8x8,
                               uint8_t msg,
                               uint8_t arg_int,
                               void* arg_ptr);
uint8_t u8g2_esp32_i2c_byte_cb(u8x8_t* u8x8,
                               uint8_t msg,
                               uint8_t arg_int,
                               void* arg_ptr);
uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t* u8x8,
                                     uint8_t msg,
                                     uint8_t arg_int,
                                     void* arg_ptr);
                                     
#endif //display_oled.h