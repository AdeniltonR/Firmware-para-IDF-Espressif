/*
 * NOME: Adenilton Ribeiro
 * DATA: 13/03/2025
 * PROJETO: html.c
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para criar um access point e abrir uma página HTML para exibir redes Wi-Fi disponíveis.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include "html.h"

// ========================================================================================================
/**
 * @brief Define o conteúdo HTML como uma string constante
 * 
 */
const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Redes Wi-Fi Disponíveis</title>
    <style>
        /* Reset básico */
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: Arial, sans-serif;
            background-color: #2D3340;
            color: #fff;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
        }

        .container {
            background-color: #3A3F4B;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.3);
            text-align: center;
            width: 90%;
            max-width: 400px;
        }

        h1 {
            margin-bottom: 20px;
            font-size: 24px;
        }

        .wifi-list {
            margin-top: 20px;
        }

        .wifi-list ul {
            list-style-type: none;
            padding: 0;
        }

        .wifi-list li {
            background-color: #4A5568;
            padding: 10px;
            margin: 5px 0;
            border-radius: 5px;
        }

        button {
            background-color: #BDD959;
            border: none;
            color: #2D3340;
            padding: 15px 30px;
            font-size: 18px;
            border-radius: 5px;
            cursor: pointer;
            transition: background-color 0.3s ease;
        }

        button:hover {
            background-color: #A8C34A;
        }

        footer {
            margin-top: 20px;
            font-size: 14px;
            color: #BDD959;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Redes Wi-Fi Disponíveis</h1>
        <div class="wifi-list">
            <button onclick="updateWifiList()">Atualizar Lista</button>
            <ul id="wifi-list"></ul>
        </div>
    </div>
    <footer>
        Desenvolvido por Adenilton R
    </footer>
    <script>
        function updateWifiList() {
            fetch('/wifi/list')
                .then(response => response.json())
                .then(data => {
                    const wifiList = document.getElementById('wifi-list');
                    wifiList.innerHTML = ''; // Limpa a lista atual
                    data.forEach(ssid => {
                        const li = document.createElement('li');
                        li.textContent = ssid;
                        wifiList.appendChild(li);
                    });
                })
                .catch(error => console.error('Erro ao atualizar a lista de redes:', error));
        }
    </script>
</body>
</html>
)rawliteral";