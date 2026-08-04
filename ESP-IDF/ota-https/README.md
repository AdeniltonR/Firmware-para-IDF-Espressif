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

Firmware para ESP32 que combina um **Wi-Fi Manager** (Access Point com portal cativo para configuração de rede) com um **OTA Manager** (atualização de firmware via HTTPS). O dispositivo pode ser configurado para se conectar a uma rede Wi-Fi através de uma página web servida pelo próprio ESP32, e pode receber atualizações de firmware remotamente através de uma URL HTTPS, com validação de versão, verificação de certificado TLS e suporte a rollback automático em caso de falha.

## Objetivo

Fornecer uma base reutilizável de firmware para projetos ESP-IDF que precisem de:

- Configuração de Wi-Fi sem hardcode de credenciais, via Access Point + portal cativo (captive portal) com página HTML de configuração.
- Persistência das credenciais de rede na NVS.
- Reconexão automática em modo Station (STA), com fallback para modo Access Point após múltiplas falhas de conexão.
- Sincronização de data/hora via NTP para validação de certificados TLS.
- Atualização remota de firmware (OTA) via HTTPS, com verificação de versão, validação de certificado do servidor, suporte a anti-rollback (secure version) e rollback automático caso o novo firmware seja reprovado por uma rotina de auto-teste da aplicação.

## Links para estudos

[**ESP-IDF Documentation**](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html)

[**ESP32 Wi-Fi Example**](https://github.com/espressif/esp-idf/tree/master/examples/wifi)

[**NTP Protocol**](https://en.wikipedia.org/wiki/Network_Time_Protocol)

[**ESP-IDF OTA over HTTPS (esp_https_ota)**](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_https_ota.html)

[**ESP-IDF App Rollback / Anti-rollback**](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html)

[**Captive Portal (ESP-IDF example)**](https://github.com/espressif/esp-idf/tree/master/examples/wifi/wifi_easy_connect)

## Pinos do projeto eletrônico

| Pino          | Função                                                                 |
|---------------|-------------------------------------------------------------------------|
| GPIO4         | Botão para iniciar/reiniciar o modo Access Point (pull-up interno)     |
| GPIO0         | Botão para iniciar atualização OTA — deve ser mantido pressionado por 3 segundos (pull-up interno) |

## Bibliotecas

Estrutura de componentes (`components/`) do projeto:

- **wifi_manager** — orquestra o modo de operação Wi-Fi (AP x STA), controla o timeout do modo Access Point e delega para os componentes `wifi` e `access_point`.
  - `wifi_manager.c`, `include/wifi_manager.h`

- **wifi** — conexão em modo Station (STA), reconexão automática (com limite de tentativas configurável), sincronização de hora via NTP e scan de redes.
  - `wifi.c`, `include/wifi.h`

- **access_point** — cria o Access Point, sobe um servidor HTTP e um servidor DNS (portal cativo), expõe endpoints para escanear redes, conectar-se a uma rede, salvar/carregar credenciais na NVS e responder às detecções de portal cativo do Android/iOS/Windows.
  - `access_point.c`, `include/access_point.h`

- **html** — páginas HTML servidas pelo Access Point (formulário de configuração e página de status "conectado").
  - `html.c`, `include/html.h`, `index.html`, `connected.html`

- **ota_manager** — atualização de firmware via HTTPS (`esp_https_ota`), com validação de versão, verificação de certificado (crt bundle), suporte a download parcial e anti-rollback via eFuse.
  - `ota_manager.c`, `include/ota_manager.h`, `server_certs/ca_cert.pem`

- **main** — inicialização da aplicação, tasks de monitoramento dos botões físicos (AP e OTA) e callback de auto-validação de firmware pós-OTA.
  - `main.c`, `Kconfig.projbuild`

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

### Configuração do OTA Manager

As opções de atualização OTA são definidas via `menuconfig`, no menu **OTA Manager** (`components/ota_manager/Kconfig.projbuild`).

#### Passo a passo (menuconfig)

1. Abra um terminal com o ambiente do ESP-IDF ativado, na pasta do projeto (`ota-https`):
   - Windows (PowerShell/CMD): execute o `export.bat` que fica na pasta de instalação do ESP-IDF (ex.: `C:\Users\<usuário>\esp\v5.4\esp-idf\export.bat`), ou abra o atalho **"ESP-IDF 5.4 CMD"** já configurado pelo instalador.
   - Linux/macOS: `. $HOME/esp/esp-idf/export.sh`

2. Rode o menu de configuração:

   ```
   idf.py menuconfig
   ```

3. Navegue com as setas ↑ ↓ até o item **"OTA Manager"** e pressione **Enter** para abrir o submenu.

4. Ajuste cada opção conforme a tabela abaixo (destaque para as principais):
   - **URL para atualização de firmware** → cole a URL HTTPS pública do `.bin` (ex.: link do Dropbox com `raw=1`, veja abaixo).
   - **Tempo de auto-teste antes de validar o firmware (s)** → tempo, em segundos, que o novo firmware fica em "teste" antes de ser confirmado (padrão: 10s). Veja [Validação pós-OTA e rollback](#validação-pós-ota-e-rollback).
   - Demais opções (timeouts, buffers, download parcial) podem ficar no padrão para a maioria dos casos.

5. Salve a configuração pressionando **S** (Save) e confirme o nome do arquivo (`sdkconfig`), depois pressione **Q** (Quit) para sair do menu.

6. Compile e grave o firmware normalmente:

   ```
   idf.py build
   idf.py -p <PORTA_SERIAL> flash monitor
   ```

   No Windows a porta costuma ser algo como `COM3`; no Linux/macOS, algo como `/dev/ttyUSB0`.

> As mesmas opções também podem ser definidas direto no arquivo `sdkconfig.defaults` (ex.: `CONFIG_OTA_MANAGER_FIRMWARE_URL="https://..."`), para não precisar repetir o passo a passo do menuconfig a cada novo clone/branch.

#### Opções disponíveis

| Opção                                             | Padrão                             | Descrição                                                                 |
|----------------------------------------------------|-------------------------------------|-----------------------------------------------------------------------------|
| `OTA_MANAGER_FIRMWARE_URL`                          | `https://example.com/firmware.bin` | URL HTTPS onde está hospedado o firmware `.bin`                            |
| `OTA_MANAGER_SKIP_COMMON_NAME_CHECK`                | `n`                                 | Ignora a validação do Common Name do certificado (reduz segurança, não recomendado em produção) |
| `OTA_MANAGER_SKIP_VERSION_CHECK`                    | `n`                                 | Permite instalar firmware com a mesma versão já em execução                |
| `OTA_MANAGER_VALIDATION_TIME_SEC`                   | `10` s                             | Tempo de auto-teste após o boot do novo firmware antes de confirmá-lo como válido (ver [Validação pós-OTA e rollback](#validação-pós-ota-e-rollback)) |
| `OTA_MANAGER_RECV_TIMEOUT`                          | `5000` ms                          | Timeout de recepção durante a operação OTA                                 |
| `OTA_MANAGER_RX_BUFFER_SIZE`                        | `8192`                             | Tamanho do buffer HTTP RX                                                   |
| `OTA_MANAGER_TX_BUFFER_SIZE`                        | `2048`                             | Tamanho do buffer HTTP TX                                                   |
| `OTA_MANAGER_ENABLE_PARTIAL_HTTP_DOWNLOAD`          | `n`                                 | Habilita download do firmware em múltiplas requisições HTTP                |
| `OTA_MANAGER_HTTP_REQUEST_SIZE`                     | `4096`                             | Tamanho máximo de cada requisição (quando o download parcial está habilitado) |

O firmware precisa estar hospedado em um servidor HTTPS válido (o certificado é validado através do crt bundle do ESP-IDF, `esp_crt_bundle_attach`). Ao usar o Dropbox como servidor de arquivos para testes, é necessário trocar o final do link compartilhado de `&dl=0` para `&raw=1`, para que o Dropbox entregue o arquivo diretamente (sem a página HTML intermediária):

```
https://www.dropbox.com/scl/fi/p6w7pkdantxpyiq27btt2/ota-https.bin?rlkey=cv5ktp81piacjugf5p6vvrbhk&st=skucstqw&raw=1
```

Para acionar a atualização, mantenha pressionado o botão OTA (GPIO0) por 3 segundos. O processo roda em uma task dedicada (`ota_manager_task`) e, ao finalizar com sucesso, o dispositivo reinicia automaticamente na nova imagem.

### Validação pós-OTA e rollback

Se o bootloader tiver o rollback habilitado (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`, já definido no `sdkconfig` do projeto), o novo firmware inicia em estado `ESP_OTA_IMG_PENDING_VERIFY` ("pendente de validação").

Ao detectar esse estado, o OTA Manager cria uma task de auto-teste (`ota_manager_validation_task`) que:

1. Aguarda `OTA_MANAGER_VALIDATION_TIME_SEC` segundos (padrão: 10s) **sem confirmar nada**.
2. Se o dispositivo reiniciar por qualquer motivo durante essa janela (crash, exception, watchdog, brownout, queda de energia, etc.), o firmware nunca chega a ser marcado como válido — no próximo boot, o **bootloader detecta isso automaticamente e faz rollback** para a partição anterior (esse comportamento é nativo do ESP-IDF, não precisa de código adicional).
3. Se a janela de tempo se esgotar sem reinicialização, a task executa o callback `app_firmware_validation()` (definido em `main.c` e passado para `ota_manager_init`) para rodar testes adicionais da aplicação (Wi-Fi, NVS, sensores, etc.).
4. Se o callback aprovar (`true`), o firmware é marcado como válido via `esp_ota_mark_app_valid_cancel_rollback()` e o rollback é cancelado.
5. Se o callback reprovar (`false`), o firmware é marcado como inválido e o dispositivo reinicia imediatamente na versão anterior via `esp_ota_mark_app_invalid_rollback_and_reboot()`.

[Preencha aqui quais testes reais você quer executar em `app_firmware_validation()` — hoje a função apenas retorna `true`.]

## Informações

| Info        | Modelo                                             |
|-------------|-----------------------------------------------------|
| uC          | [confirmar — o `sdkconfig` local está configurado para `esp32s3`, mas o README menciona ESP32-C3] |
| Placa       | [preencher]                                          |
| Arquitetura | [Xtensa LX7 se ESP32-S3, ou RISC-V se ESP32-C3]      |
| IDE         | IDF v5.4.0                                           |
| Tabela de partições | `partitions/partitions_ota_4mb.csv`, `partitions_ota_8mb.csv` ou `partitions_ota_16mb.csv` (dupla partição OTA — escolher conforme o tamanho da flash) |