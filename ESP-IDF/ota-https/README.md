# _OTA HTTPS_

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
| 1.0.0  | 26/03/2026 | Adenilton R   | Inicio do projeto  |

---

## Resumo

[Descrição resumida do projeto.]

## Objetivo

[Descrição resumida do projeto.]

## Links para estudos

[**ESP-IDF Documentation**](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html)

[**ESP32 Wi-Fi Example**](https://github.com/espressif/esp-idf/tree/master/examples/wifi)

[**NTP Protocol**](https://en.wikipedia.org/wiki/Network_Time_Protocol)

## Pinos do projeto eletrônico

Este projeto não utiliza pinos específicos do ESP32, pois foca na configuração de Wi-Fi e sincronização de horário.

## Bibliotecas

main.c

wifi.c

wifi.h

CMakeLists.txt

Kconfig.projbuild

[Firmware - Wi-Fi](https://www.notion.so/Firmware-Wi-Fi-333cbcaa7ba981e7a387c4244925d02d?pvs=21)

## Configuração do Firmware

O Wi-Fi é configurado com os seguintes parâmetros no arquivo `wifi.c`:

![code_1.png](attachment:48d5b452-698a-4fa8-ae4f-2fcc780dd1fa:code_1.png)

Para poder testar a conexão de internet pode chamar as funções para puxar hora e data, as funções estão no arquivo `wifi.c`:

![code_2.png](attachment:d40f0a6e-5cbe-4d09-8329-8ab9e91e6a8f:code_2.png)

Configura o fuso horário:

```c
initialize_hora();
```

Testa a conexão com a internet e obtém a hora:

```c
test_ntp_connection();
```

Dados do monitor serial:

![code_3.png](attachment:d69fae4c-d151-45e6-801e-5611b4ac213d:code_3.png)

Importande adicionar o arquivo dentro da pasta main Kconfig.projbuild:

## Informações

| Info        | Modelo           |
|-------------|------------------|
| uC          | ESP32 C3         |
| Placa       | ESP32-C3 Module  |
| Arquitetura | RISC-V           |
| IDE         | IDF v5.4.0       |