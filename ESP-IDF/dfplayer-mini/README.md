# _DFPlayer Mini_

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
| 1.0.0  | 19/03/2025 | Adenilton R   | Inicio do projeto  |

---

## Resumo

Este firmware foi desenvolvido para controlar o módulo DFPlayer Mini utilizando o ESP32. Ele permite:

- Reprodução de arquivos de áudio
- Controle de volume
- Leitura de informações do cartão SD
- Monitoramento de eventos do player

## Objetivo

O objetivo principal é fornecer uma interface robusta para controle do DFPlayer Mini via ESP32, com:

1. **Inicialização segura**:
    - Controle do pino de enable
    - Configuração automática da comunicação serial
2. **Controle de reprodução**:
    - Seleção de músicas por índice
    - Ajuste de volume (0-30)
3. **Monitoramento**:
    - Detecção de eventos (término de reprodução, erros)
    - Verificação de arquivos no cartão SD

## Links para estudos

[**Referência do Projeto**](https://github.com/nopnop2002/esp-idf-DFPlayerMini/tree/main)

[**Documentação ESP-IDF**](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html)

[**Datasheet DFPlayer Mini**](https://www.dfrobot.com/product-1121.html)

[**Protocolo Serial DFPlayer**](https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299)

## Pinos do projeto eletrônico

| Função       | Pino ESP32 | Pino DFPlayer |
|--------------|------------|---------------|
| TX (saída)   | GPIO18     | RX            |
| RX (entrada) | GPIO4      | TX            |
| Enable       | GPIO16     | VCC/Enable    |

## Bibliotecas

[main.c](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/dfplayer-mini/main/main.c)

[DFRobotDFPlayerMini.c](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/dfplayer-mini/components/DFRobotDFPlayerMini/DFRobotDFPlayerMini.c)

[DFRobotDFPlayerMini.h]()

[serial.c](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/dfplayer-mini/components/DFRobotDFPlayerMini/serial.c)

[serial.h]()

[CMakeLists.txt](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/dfplayer-mini/components/DFRobotDFPlayerMini/CMakeLists.txt)

[Kconfig.projbuild](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/dfplayer-mini/components/DFRobotDFPlayerMini/Kconfig.projbuild)

## Configuração do Firmware

Inicialização Serial:

```c
//---inicializa o DFPlayer Mini---
    bool ret = DF_begin(PIN_tx, PIN_rx, true, true, debug);
    ESP_LOGI(TAG, "DF_begin=%d", ret);  // Log do resultado da inicialização
```

Parâmetros:

- `PIN_tx`: Pino TX do ESP32;
- `PIN_rx`: Pino RX do ESP32;
- `ACK`: Habilita confirmação de comandos;
- `Reset`: Executa reset no inicialização;
- `Debug`: Habilita modo de depuração.

Controle de Energia:

```c
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
    ESP_LOGI(TAG, "Enable ativado!"); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);  
}
```

Fluxo Principal:

1. Ativação do módulo via pino GPIO16
2. Inicialização da comunicação serial (9600 baud)
3. Verificação do cartão SD
4. Reprodução da música selecionada
5. Monitoramento contínuo de eventos

Tratamento de Eventos:

O firmware detecta automaticamente:

- Término de reprodução (evento duplo)
- Erros no cartão SD
- Problemas de comunicação

Dados do monitor serial:

![RGB.png](Docs/RGB.png)

## Informações

| Info        | Modelo           |
|-------------|------------------|
| uC          | ESP32 32D        |
| Placa       | ESP32 Module     |
| Arquitetura | Xtensa / RISC    |
| IDE         | IDF v5.4.0       |