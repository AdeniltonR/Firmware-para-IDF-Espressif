# _Comunicação API_

![Firmware version](https://img.shields.io/badge/Firmware_version-1.0.0-blue)

---

## Sumário

- [Histórico de Versão](#histórico-de-versão)
- [Resumo](#resumo)
- [Objetivo](#objetivo)
- [Fluxograma](#fluxograma)
- [Links para estudos](#links-para-estudos)
- [Pinos do projeto eletrônico](#pinos-do-projeto-eletrônico)
- [Bibliotecas](#bibliotecas)
- [Configuração do Firmware](#configuração-do-firmware)
- [Configuração do Ambiente](#configuração-do-ambiente)
- [Informações](#informações)


## Histórico de versão

| Versão | Data       | Autor         | Descrição          |
|--------|------------|---------------|--------------------|
| 1.0.0  | 17/04/2025 | Adenilton R   | Inicio do projeto  |

---

## Resumo

Sistema completo para monitoramento remoto de equipamentos industriais, composto por:
- **Firmware ESP32**: Coleta e envia dados de velocidade
- **API Python**: Recebe e armazena dados dos dispositivos
- **Interface REST**: Documentação automática e endpoints para consulta

## Objetivo

**Firmware ESP32**

- Conexão WiFi dual-mode (STA/AP)
- Envio periódico de dados via HTTP
- Controle por botão físico (GPIO0)
- Sistema de logs detalhado

**API Python**

- Endpoints REST para:
  - Recebimento de dados (`POST /api/device`)
  - Consulta de dispositivos (`GET /api/device/<id>`)
  - Documentação interativa (`GET /api/docs`)
- Validação de dados
- Armazenamento em memória

## Links para estudos

[**Documentação ESP-IDF**](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/index.html)

[**Exemplos Oficiais Wi-Fi**](https://github.com/espressif/esp-idf/tree/v5.4/examples/wifi)

## Pinos do projeto eletrônico

| Função          | Pino ESP32 |
|-----------------|------------|
| Botão Modo AP   | GPIO_NUM_0 |

## Bibliotecas

[main.c](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/comunicacao-api/main/main.c)

[Kconfig.projbuild](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/comunicacao-api/main/Kconfig.projbuild)

[components](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/tree/main/ESP-IDF/comunicacao-api/components)

## Configuração do Firmware

**Parâmetros Ajustáveis:**

![access_point.png](Docs/access_point.png)

**Estrutura do Projeto:**

```c
components/
├── api_client/            # Lógica API
│   ├── include/
│   │   └── api_client.h
│   └── api_client.c
├── wifi_manager/          # Lógica central
│   ├── include/
│   │   └── wifi_manager.h
│   └── wifi_manager.c
├── access_point/          # Modo AP
│   ├── include/
│   │   └── access_point.h  
│   └── access_point.c
├── wifi/                  # Modo STA
│   ├── include/
│   │   └── wifi.h
│   └── wifi.c
└── html/                  # Interface Web
    ├── include/
    │   └── html.h
    └── html.c
```

**Como Usar:**

1. **Primeira inicialização**:
    - O ESP32 inicia em modo AP
    - Conecte-se à rede `ESP-IDF`
    - Acesse `http://192.168.4.1`
    - Insira as credenciais da sua rede Wi-Fi
2. **Conexão automática**:
    - Após configuração, o ESP32 reinicia em modo STA
    - Conecta-se automaticamente à rede salva
3. **Forçar modo AP**:
    - Pressione o botão GPIO0 por 1 segundo
    - Útil para reconfiguração

Importande adicionar o arquivo dentro da pasta main [Kconfig.projbuild](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/comunicacao-api/main/Kconfig.projbuild):

Página web:

![wi-fi.png](Docs/wi-fi.png)

**Configuração da API**

Para usar outra API mude URL e dependendo das mensagens que forem enviados tem que mudar a **struct** no **api_client.h**:

![url_api.png](Docs/url_api.png)

**Para usar API HTTPS precisa ajustar as seguintes configurações**

No arquivo components/api_clent/api_clent.c descomente a linha:

```c
.crt_bundle_attach = esp_crt_bundle_attach, // 🔒 Usa o bundle de certificados da Espressif para validar HTTPS (sem precisar de .pem manual)
.disable_auto_redirect = true,              // 🔄 Se true, desativa redirecionamento automático (útil para debug)
.buffer_size = 1024,                        // 📥 Tamanho do buffer de leitura (recepção da resposta)
.buffer_size_tx = 1024,                     // 📤 Tamanho do buffer de escrita (envio do corpo da requisição)
```

Em:

![Config_API.png](Docs/Config_API.png)

No arquivo components/api_clent/api_clent.h descomente a linha:

```basic
#include "esp_crt_bundle.h”
```

Em:

![Biblioteca.png](Docs/Biblioteca.png)

Agora abra o Open ESP-IDF terminal e digite seguinte comandos:

```basic
idf.py menuconfig 
```

Navegue até `Enable trusted root certificate bundle` e ative:

```basic
(Top) → Component config → mbedTLS → Certificate Bundle                                                                                                                                                                  Espressif IoT Development Framework Configuration                                                        
[*] Enable trusted root certificate bundle                                                                                                                       
        Default certificate bundle options (Use the full default certificate bundle)  --->                                                                       
[ ]     Add custom certificates to the default bundle                                                                                                            
[ ]     Add deprecated root certificates                                                                                                                         
(200)   Maximum no of certificates allowed in certificate bundle                                                                                                 
                
```

Navegue até `Use the full default certificate bundle` e ative:

```basic
(Top) → Component config → mbedTLS → Certificate Bundle → Enable trusted root certificate bundle → Default certificate bundle options                                                                                    Espressif IoT Development Framework Configuration

(X) Use the full default certificate bundle

( ) Use only the most common certificates from the default bundles

( ) Do not use the default certificate bundle
```

Quando geral buid e der erro de memoria, navegue até `Two large size OTA partitions` e ative:

```basic
(Top) → Partition Table → Partition Table                                                                                                                                                                                Espressif IoT Development Framework Configuration                                                        
( ) Single factory app, no OTA                                                                                                                                   
( ) Single factory app (large), no OTA                                                                                                                           
( ) Factory app, two OTA definitions                                                                                                                             
(X) Two large size OTA partitions                                                                                                                                
( ) Custom partition table CSV    
```

Com isso vai estar configurado para usar API HTTPS.

## Configuração do Ambiente

Recomendado para isolamento de dependências:

**Criar ambiente (Linux/Mac)**

```bash
python3 -m venv venv
source venv/bin/activate
```

**Criar ambiente (Windows)**

```bash
python -m venv venv
.\venv\Scripts\activate
```

**Instalação de Dependências**

[Flask-cors](https://pypi.org/project/flask-cors/), v5.0.1

```bash
pip install flask flask-cors
```

[pytz](https://pypi.org/project/pytz/), v2025.2

```basic
pip install pytz
```

**Testando a API**

```bash
python api.py
```

Exemplo do [**código python**](https://github.com/AdeniltonR/Firmware-para-IDF-Espressif/blob/main/ESP-IDF/comunicacao-api/api.py):

**Testes com URL, abra terminal GIT Bash e execute o comando, pode mudar o localhost por http://172.17.57.14**

```bash
curl -X POST http://localhost:5000/api/device \
-H "Content-Type: application/json" \
-d '{"id":"ESP32_TEST","status":"produzindo","velocidade":1.5}'
```

![URL.png](Docs/URL.png)

**Consulta de devices**

```bash
http://localhost:5000/api/devices
```

**Consulta de device / ID**

```bash
http://localhost:5000/api/device/ESP32_001
```

**Consulta da documentação**

```bash
http://localhost:5000/api/docs
```

![web_api.png](Docs/web_api.png)

## Informações

| Info        | Modelo        |
|-------------|---------------|
| uC          | ESP32 32D     |
| Placa       | ESP32 Module  |
| Arquitetura | Xtensa / RISC |
| IDE         | IDF v5.4.0    |