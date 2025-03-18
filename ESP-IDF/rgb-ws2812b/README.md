# _RGB WS2812B_

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
| 1.0.0  | 18/03/2025 | Adenilton R   | Inicio do projeto  |

---

## Resumo

Este projeto tem como objetivo controlar uma tira de LEDs WS2812B utilizando o protocolo SPI no ESP32. O ESP32 é configurado para enviar dados de cor aos LEDs via SPI, permitindo o controle individual de cada LED na tira.

O projeto utiliza o framework ESP-IDF e inclui funcionalidades como:

- Inicialização da tira de LEDs WS2812B.
- Controle de cores RGB para cada LED.
- Atualização da tira de LEDs com novas cores.
- Suporte para diferentes modelos de tiras de LED (WS2812B e WS2815).

## Objetivo

O objetivo principal deste projeto é demonstrar como controlar uma tira de LEDs WS2812B utilizando o protocolo SPI no ESP32. Os objetivos específicos incluem:

1. **Inicialização da Tira de LEDs**:
   - Configurar o ESP32 para enviar dados aos LEDs WS2812B via SPI.
   - Suportar diferentes modelos de tiras de LED (WS2812B e WS2815).

2. **Controle de Cores**:
   - Permitir o controle individual de cada LED na tira.
   - Definir cores RGB para cada LED.

3. **Atualização da Tira de LEDs**:
   - Atualizar a tira de LEDs com novas cores enviadas pelo ESP32.

4. **Exemplo de Uso**:
   - Demonstrar o uso da biblioteca para acender LEDs em diferentes cores (vermelho, verde, azul, amarelo, roxo).

## Links para estudos

[**Link de Referência**](https://github.com/okhsunrog/esp_ws28xx/tree/main)

[**ESP-IDF Documentation**](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html)

[**WS2812B Datasheet**](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)

[**SPI Protocol**](https://en.wikipedia.org/wiki/Serial_Peripheral_Interface)

## Pinos do projeto eletrônico

| Pino ESP32 | Função          |
|------------|-----------------|
| GPIO 48    | MOSI (Dados SPI)|

## Bibliotecas

[main.c](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/rgb-ws2812b/main/main.c)

[ws2812b.c](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/rgb-ws2812b/components/ws2812b/ws2812b.c)

[ws2812b.h](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/rgb-ws2812b/components/ws2812b/include/ws2812b.h)

[CMakeLists.txt](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/rgb-ws2812b/components/ws2812b/CMakeLists.txt)

## Configuração do Firmware

O controle dos LEDs WS2812B é configurado com os seguintes parâmetros no arquivo `ws2812b.c`:

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

![RGB.png](Docs/RGB.png)

## Informações

| Info        | Modelo           |
|-------------|------------------|
| uC          | ESP32-S3         |
| Placa       | ESP32-S3 Module  |
| Arquitetura | Xtensa / RISC    |
| IDE         | IDF v5.4.0       |