# _Led Strip_

![Firmware version](https://img.shields.io/badge/Firmware_version-1.0.0-blue)

---

## Sumário

- [Histórico de Versão](#histórico-de-versão)
- [Resumo](#resumo)
- [Objetivo](#objetivo)
- [Links para estudos](#links-para-estudos)
- [Pinos do projeto eletrônico](#pinos-do-projeto-eletrônico)
- [Bibliotecas](#bibliotecas)
- [Configuração do Firmware](#configuração-do-firmware)
- [Informações](#informações)


## Histórico de versão

| Versão | Data       | Autor         | Descrição          |
|--------|------------|---------------|--------------------|
| 1.0.0  | 20/03/2025 | Adenilton R   | Inicio do projeto  |

---

## Resumo

A biblioteca led_strip foi desenvolvida para controlar tiras de LEDs WS2812B utilizando o periférico RMT (Remote Control Transceiver) do ESP32. Ela permite o controle individual de cada LED na tira, definindo cores RGB e atualizando a tira de LEDs com novas cores.

## Objetivo

O objetivo principal desta biblioteca é fornecer uma interface simples e eficiente para controlar tiras de LEDs WS2812B no ESP32. Os objetivos específicos incluem:

1. **Inicialização da Tira de LEDs**:
    - Configurar o ESP32 para enviar dados aos LEDs WS2812B via RMT.
    - Suportar diferentes modelos de tiras de LED (WS2812B e WS2815).
2. **Controle de Cores**:
    - Permitir o controle individual de cada LED na tira.
    - Definir cores RGB para cada LED.
3. **Atualização da Tira de LEDs**:
    - Atualizar a tira de LEDs com novas cores enviadas pelo ESP32.
4. **Exemplo de Uso**:
    - Demonstrar o uso da biblioteca para acender LEDs em diferentes cores (vermelho, verde, azul, amarelo, roxo).

## Links para estudos

[**Link de Referência**](https://www.youtube.com/watch?v=xdxsDxw2iOc)

[**ESP-IDF Documentation**](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html)

[**WS2812B Datasheet**](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)

[**RMT Protocol**](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/rmt.html)

## Pinos do projeto eletrônico

| Nome      | Pino |
|-----------|------|
| PIN_rgb_1 | D19  |
| PIN_rgb_1 | D23  |

## Bibliotecas

[main.c](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/led-strip/main/main.c)

[led_strip.c](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/led-strip/components/led_strip/led_strip.c)

[led_strip.h](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/led-strip/components/led_strip/include/led_strip.h)

[CMakeLists.txt](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/led-strip/components/led_strip/CMakeLists.txt)

## Configuração do Firmware

O controle dos LEDs WS2812B é configurado com os seguintes parâmetros no arquivo led_strip.c:

```c
static spi_settings_t spi_settings = {
    .host = SPI2_HOST,           // Host SPI (SPI2)
    .dma_chan = SPI_DMA_CH_AUTO, // Canal DMA automático
    .buscfg = {
        .miso_io_num = -1,       // Pino MISO não utilizado
        .sclk_io_num = -1,       // Pino SCLK não utilizado
        .quadwp_io_num = -1,     // Pino QUADWP não utilizado
        .quadhd_io_num = -1,     // Pino QUADHD não utilizado
    },
    .devcfg = {
        .clock_speed_hz = 3.2 * 1000 * 1000, // Clock de 3.2 MHz
        .mode = 0,                           // Modo SPI 0
        .spics_io_num = -1,                  // Pino CS não utilizado
        .queue_size = 1,                     // Tamanho da fila de transmissão
        .command_bits = 0,                   // Sem bits de comando
        .address_bits = 0,                   // Sem bits de endereço
        .flags = SPI_DEVICE_TXBIT_LSBFIRST,  // Transmissão LSB primeiro
    },
};
```

Como isso led altera de cores:

![Led-strip.png](Docs/Led-strip.png)

## Informações

| Info        | Modelo           |
|-------------|------------------|
| uC          | ESP32 32D        |
| Placa       | ESP32 Module     |
| Arquitetura | Xtensa / RISC    |
| IDE         | IDF v5.4.0       |