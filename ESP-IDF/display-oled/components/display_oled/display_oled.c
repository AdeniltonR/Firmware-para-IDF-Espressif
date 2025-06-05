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
// ---BIBLIOTECA---

#include "display_oled.h"

// ========================================================================================================
//---VARIAVEIS GLOBAIS---

/// @brief Tag para identificação dos logs deste módulo (u8g2_hal)
static const char *TAG = "u8g2_hal";
//---tempo máximo de espera para operações I2C (1 segundo)---
static const unsigned int I2C_TIMEOUT_MS = 1000;

//---handles para comunicação---
static spi_device_handle_t handle_spi;   // Handle para dispositivo SPI
static i2c_cmd_handle_t handle_i2c;      // Handle para comando I2C
static u8g2_esp32_hal_t u8g2_esp32_hal;  // Estrutura com configurações HAL

//---define o host SPI a ser utilizado (SPI2)---
#define HOST    SPI2_HOST

//---macro modificada para verificação de erros com log detalhado---
#undef ESP_ERROR_CHECK
#define ESP_ERROR_CHECK(x)                   \
  do {                                       \
    esp_err_t rc = (x);                      \
    if (rc != ESP_OK) {                      \
      ESP_LOGE("err", "esp_err_t = %d", rc); \
      assert(0 && #x);                       \
    }                                        \
  } while (0);

// ========================================================================================================
/**
 * @brief A camada HAL (Hardware Abstraction Layer) para o ESP32.
 * Armazena a configuração dos pinos na estrutura global u8g2_esp32_hal.
 *
 * @param u8g2_esp32_hal_param - estrutura contendo a configuração dos pinos
*/
void u8g2_esp32_hal_init(u8g2_esp32_hal_t u8g2_esp32_hal_param) {
  u8g2_esp32_hal = u8g2_esp32_hal_param;
}  // u8g2_esp32_hal_init

// ========================================================================================================
/**
 * @brief A função de callback para comunicação SPI conforme definido pela biblioteca U8G2.
 * Lida com a inicialização, configuração e transmissão de dados via SPI.
 *
 * @param
 *   u8x8     - ponteiro para a estrutura U8G2
 *   msg      - tipo de mensagem/operação a ser realizada
 *   arg_int  - argumento inteiro adicional
 *   arg_ptr  - ponteiro para dados adicionais
 *
 * @return
 *   Sempre retorna 0 (conforme contrato da U8G2)
*/
uint8_t u8g2_esp32_spi_byte_cb(u8x8_t* u8x8,
                               uint8_t msg,
                               uint8_t arg_int,
                               void* arg_ptr) {
  ESP_LOGD(TAG, "spi_byte_cb: Mensagem recebida: %d, arg_int: %d, arg_ptr: %p",
           msg, arg_int, arg_ptr);
  
  switch (msg) {
    //---configura o pino DC (Data/Command)---
    case U8X8_MSG_BYTE_SET_DC:
      if (u8g2_esp32_hal.dc != U8G2_ESP32_HAL_UNDEFINED) {
        gpio_set_level(u8g2_esp32_hal.dc, arg_int);
      }
      break;

    //---inicialização do barramento SPI---
    case U8X8_MSG_BYTE_INIT: {
      //---verifica se os pinos SPI estão definidos---
      if (u8g2_esp32_hal.bus.spi.clk == U8G2_ESP32_HAL_UNDEFINED ||
          u8g2_esp32_hal.bus.spi.mosi == U8G2_ESP32_HAL_UNDEFINED ||
          u8g2_esp32_hal.bus.spi.cs == U8G2_ESP32_HAL_UNDEFINED) {
        break;
      }

      //---configuração do barramento SPI---
      spi_bus_config_t bus_config;
      memset(&bus_config, 0, sizeof(spi_bus_config_t));
      bus_config.sclk_io_num = u8g2_esp32_hal.bus.spi.clk;   // Pino CLK
      bus_config.mosi_io_num = u8g2_esp32_hal.bus.spi.mosi;  // Pino MOSI
      bus_config.miso_io_num = GPIO_NUM_NC;                  // MISO não utilizado
      bus_config.quadwp_io_num = GPIO_NUM_NC;                // Não utilizado
      bus_config.quadhd_io_num = GPIO_NUM_NC;                // Não utilizado
      
      ESP_ERROR_CHECK(spi_bus_initialize(HOST, &bus_config, 1));

      //---configuração do dispositivo SPI---
      spi_device_interface_config_t dev_config;
      dev_config.address_bits = 0;
      dev_config.command_bits = 0;
      dev_config.dummy_bits = 0;
      dev_config.mode = 0;                   // Modo 0 (CPOL=0, CPHA=0)
      dev_config.duty_cycle_pos = 0;
      dev_config.cs_ena_posttrans = 0;
      dev_config.cs_ena_pretrans = 0;
      dev_config.clock_speed_hz = 10000;     // Frequência de 10kHz
      dev_config.spics_io_num = u8g2_esp32_hal.bus.spi.cs; // Pino CS
      dev_config.flags = 0;
      dev_config.queue_size = 200;           // Tamanho da fila de transações
      dev_config.pre_cb = NULL;
      dev_config.post_cb = NULL;
      
      ESP_ERROR_CHECK(spi_bus_add_device(HOST, &dev_config, &handle_spi));
      break;
    }

    //---envio de dados via SPI---
    case U8X8_MSG_BYTE_SEND: {
      spi_transaction_t trans_desc;
      trans_desc.addr = 0;
      trans_desc.cmd = 0;
      trans_desc.flags = 0;
      trans_desc.length = 8 * arg_int;  // Número de bits (não bytes)
      trans_desc.rxlength = 0;
      trans_desc.tx_buffer = arg_ptr;   // Buffer de dados a transmitir
      trans_desc.rx_buffer = NULL;

      ESP_ERROR_CHECK(spi_device_transmit(handle_spi, &trans_desc));
      break;
    }
  }
  return 0;
}  // u8g2_esp32_spi_byte_cb

// ========================================================================================================
/**
 * @brief Função de callback para comunicação I2C conforme definido pela biblioteca U8G2.
 * Lida com a inicialização, configuração e transmissão de dados via I2C.
 *
 * @param
 *   u8x8     - ponteiro para a estrutura U8G2
 *   msg      - tipo de mensagem/operação a ser realizada
 *   arg_int  - argumento inteiro adicional
 *   arg_ptr  - ponteiro para dados adicionais
 *
 * @return
 *   Sempre retorna 0 (conforme contrato da U8G2)
*/
uint8_t u8g2_esp32_i2c_byte_cb(u8x8_t* u8x8,
                               uint8_t msg,
                               uint8_t arg_int,
                               void* arg_ptr) {
  ESP_LOGD(TAG, "i2c_cb: Mensagem recebida: %d, arg_int: %d, arg_ptr: %p", msg,
           arg_int, arg_ptr);

  switch (msg) {
    //---configura o pino DC (Data/Command)---
    case U8X8_MSG_BYTE_SET_DC: {
      if (u8g2_esp32_hal.dc != U8G2_ESP32_HAL_UNDEFINED) {
        gpio_set_level(u8g2_esp32_hal.dc, arg_int);
      }
      break;
    }

    //---inicialização do barramento I2C---
    case U8X8_MSG_BYTE_INIT: {
      //---verifica se os pinos I2C estão definidos---
      if (u8g2_esp32_hal.bus.i2c.sda == U8G2_ESP32_HAL_UNDEFINED ||
          u8g2_esp32_hal.bus.i2c.scl == U8G2_ESP32_HAL_UNDEFINED) {
        break;
      }

      //---configuração do I2C---
      i2c_config_t conf = {0};
      conf.mode = I2C_MODE_MASTER;
      ESP_LOGI(TAG, "sda_io_num %d", u8g2_esp32_hal.bus.i2c.sda);
      conf.sda_io_num = u8g2_esp32_hal.bus.i2c.sda;          // Pino SDA
      conf.sda_pullup_en = GPIO_PULLUP_ENABLE;               // Pull-up ativado
      ESP_LOGI(TAG, "scl_io_num %d", u8g2_esp32_hal.bus.i2c.scl);
      conf.scl_io_num = u8g2_esp32_hal.bus.i2c.scl;          // Pino SCL
      conf.scl_pullup_en = GPIO_PULLUP_ENABLE;               // Pull-up ativado
      ESP_LOGI(TAG, "clk_speed %d", I2C_MASTER_FREQ_HZ);
      conf.master.clk_speed = I2C_MASTER_FREQ_HZ;            // Frequência do clock
      
      ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
      ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                                         I2C_MASTER_RX_BUF_DISABLE,
                                         I2C_MASTER_TX_BUF_DISABLE, 0));
      break;
    }

    //---envio de dados via I2C---
    case U8X8_MSG_BYTE_SEND: {
      uint8_t* data_ptr = (uint8_t*)arg_ptr;
      ESP_LOG_BUFFER_HEXDUMP(TAG, data_ptr, arg_int, ESP_LOG_VERBOSE);

      //---envia cada byte individualmente---
      while (arg_int > 0) {
        ESP_ERROR_CHECK(
            i2c_master_write_byte(handle_i2c, *data_ptr, ACK_CHECK_EN));
        data_ptr++;
        arg_int--;
      }
      break;
    }

    //---inicia uma transferência I2C---
    case U8X8_MSG_BYTE_START_TRANSFER: {
      uint8_t i2c_address = u8x8_GetI2CAddress(u8x8);
      handle_i2c = i2c_cmd_link_create();
      ESP_LOGD(TAG, "Iniciando transferência I2C para %02X.", i2c_address >> 1);
      ESP_ERROR_CHECK(i2c_master_start(handle_i2c));
      ESP_ERROR_CHECK(i2c_master_write_byte(
          handle_i2c, i2c_address | I2C_MASTER_WRITE, ACK_CHECK_EN));
      break;
    }

    //---finaliza uma transferência I2C---
    case U8X8_MSG_BYTE_END_TRANSFER: {
      ESP_LOGD(TAG, "Finalizando transferência I2C.");
      ESP_ERROR_CHECK(i2c_master_stop(handle_i2c));
      ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, handle_i2c,
                                           pdMS_TO_TICKS(I2C_TIMEOUT_MS)));
      i2c_cmd_link_delete(handle_i2c);
      break;
    }
  }
  return 0;
}  // u8g2_esp32_i2c_byte_cb

// ========================================================================================================
/**
 * @brief Função de callback para GPIO e delays conforme definido pela biblioteca U8G2.
 * Lida com a configuração de pinos GPIO e operações de delay.
 *
 * @param
 *   u8x8     - ponteiro para a estrutura U8G2
 *   msg      - tipo de mensagem/operação a ser realizada
 *   arg_int  - argumento inteiro adicional
 *   arg_ptr  - ponteiro para dados adicionais
 *
 * @return
 *   Sempre retorna 0 (conforme contrato da U8G2)
*/
uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t* u8x8,
                                     uint8_t msg,
                                     uint8_t arg_int,
                                     void* arg_ptr) {
  ESP_LOGD(TAG,
           "gpio_and_delay_cb: Mensagem recebida: %d, arg_int: %d, arg_ptr: %p",
           msg, arg_int, arg_ptr);

  switch (msg) {
    //---inicialização dos GPIOs---
    case U8X8_MSG_GPIO_AND_DELAY_INIT: {
      uint64_t bitmask = 0;
      
      //---configura máscara de bits com os pinos definidos---
      if (u8g2_esp32_hal.dc != U8G2_ESP32_HAL_UNDEFINED) {
        bitmask = bitmask | (1ull << u8g2_esp32_hal.dc);
      }
      if (u8g2_esp32_hal.reset != U8G2_ESP32_HAL_UNDEFINED) {
        bitmask = bitmask | (1ull << u8g2_esp32_hal.reset);
      }
      if (u8g2_esp32_hal.bus.spi.cs != U8G2_ESP32_HAL_UNDEFINED) {
        bitmask = bitmask | (1ull << u8g2_esp32_hal.bus.spi.cs);
      }

      if (bitmask == 0) {
        break;
      }
      
      //---configura os GPIOs como saída---
      gpio_config_t gpioConfig;
      gpioConfig.pin_bit_mask = bitmask;
      gpioConfig.mode = GPIO_MODE_OUTPUT;
      gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
      gpioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
      gpioConfig.intr_type = GPIO_INTR_DISABLE;
      gpio_config(&gpioConfig);
      break;
    }

    //---controle do pino RESET---
    case U8X8_MSG_GPIO_RESET:
      if (u8g2_esp32_hal.reset != U8G2_ESP32_HAL_UNDEFINED) {
        gpio_set_level(u8g2_esp32_hal.reset, arg_int);
      }
      break;
      
    //---controle do pino CS (Chip Select)---
    case U8X8_MSG_GPIO_CS:
      if (u8g2_esp32_hal.bus.spi.cs != U8G2_ESP32_HAL_UNDEFINED) {
        gpio_set_level(u8g2_esp32_hal.bus.spi.cs, arg_int);
      }
      break;
      
    //---controle do pino SCL (I2C Clock)---
    case U8X8_MSG_GPIO_I2C_CLOCK:
      if (u8g2_esp32_hal.bus.i2c.scl != U8G2_ESP32_HAL_UNDEFINED) {
        gpio_set_level(u8g2_esp32_hal.bus.i2c.scl, arg_int);
      }
      break;
      
    //---controle do pino SDA (I2C Data)---
    case U8X8_MSG_GPIO_I2C_DATA:
      if (u8g2_esp32_hal.bus.i2c.sda != U8G2_ESP32_HAL_UNDEFINED) {
        gpio_set_level(u8g2_esp32_hal.bus.i2c.sda, arg_int);
      }
      break;

    //---delay em milissegundos---
    case U8X8_MSG_DELAY_MILLI:
      vTaskDelay(arg_int / portTICK_PERIOD_MS);
      break;
  }
  return 0;
}  // u8g2_esp32_gpio_and_delay_cb