# _Touch Pad_

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

O firmware Touch Pad foi desenvolvido para detectar toques em dois pinos capacitivos do ESP32 usando a ESP-IDF v5.4.0. Ele implementa um sistema de debounce para evitar múltiplas detecções de um único toque e utiliza FreeRTOS para leitura assíncrona dos sensores.

## Objetivo

O objetivo principal deste firmware é fornecer uma interface confiável para detecção de toques em sensores capacitivos do ESP32. Os objetivos específicos incluem:

1. **Configuração dos Touch Pads**:
    - Inicialização correta dos sensores capacitivos
    - Configuração de tensão e limiares de detecção
2. **Leitura Confiável**:
    - Implementação de debounce para evitar detecções múltiplas
    - Leitura assíncrona usando FreeRTOS
3. **Saída de Log**:
    - Registro claro dos eventos de toque
    - Detecção de erros na leitura dos sensores

## Links para estudos

[**ESP-IDF Touch Pad Documentation**](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/touch_pad.html)

[**FreeRTOS Documentation**](https://www.freertos.org/Documentation/RTOS_book.html)

[**ESP32 Capacitive Touch Sensing**](https://www.youtube.com/watch?v=JmFJz0H3J6o)

## Pinos do projeto eletrônico

| Nome        | Pino ESP32 | Touch Pad |
|-------------|------------|-----------|
| TOUCH_PAD_1 | GPIO13     | T4        |
| TOUCH_PAD_2 | GPIO27     | T7        |

**Tabela de Touch Pads do ESP32:**

| Touch Pad | TOUCH_PAD_NUM  | Pino GPIO | Observações                               |
|-----------|----------------|-----------|-------------------------------------------|
| T0        | TOUCH_PAD_NUM0 | GPIO4     | Disponível na maioria das placas          |
| T1        | TOUCH_PAD_NUM1 | GPIO0     | **Cuidado**: Pino de boot                 |
| T2        | TOUCH_PAD_NUM2 | GPIO2     | Conectado ao LED onboard em muitas placas |
| T3        | TOUCH_PAD_NUM3 | GPIO15    | Pode precisar de pull-down no boot        |
| T4        | TOUCH_PAD_NUM4 | GPIO13    | Disponível na maioria das placas          |
| T5        | TOUCH_PAD_NUM5 | GPIO12    | Disponível na maioria das placas          |
| T6        | TOUCH_PAD_NUM6 | GPIO14    | Disponível na maioria das placas          |
| T7        | TOUCH_PAD_NUM7 | GPIO27    | Disponível na maioria das placas          |
| T8        | TOUCH_PAD_NUM8 | GPIO33    | Disponível apenas em ESP32 com 36 pinos   |
| T9        | TOUCH_PAD_NUM9 | GPIO32    | Disponível apenas em ESP32 com 36 pinos   |

## Bibliotecas

[main.c](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/touch-pad/main/main.c)

## Configuração do Firmware

O firmware é configurado com os seguintes parâmetros:

![Touch-pad.png](Docs/Touch-pad.png)

O firmware opera da seguinte maneira:

1. Inicializa os sensores touch pad
2. Cria uma tarefa FreeRTOS para leitura contínua
3. Verifica se os valores lidos estão dentro do intervalo configurado
4. Implementa lógica de debounce para evitar detecções múltiplas
5. Registra eventos de toque no log quando detectados

## Informações

| Info        | Modelo           |
|-------------|------------------|
| uC          | ESP32 32D        |
| Placa       | ESP32 Module     |
| Arquitetura | Xtensa / RISC    |
| IDE         | IDF v5.4.0       |