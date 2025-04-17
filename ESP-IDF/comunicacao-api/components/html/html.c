/*
 * NOME: Adenilton Ribeiro
 * DATA: 14/03/2025
 * PROJETO: access point
 * VERSAO: 1.0.0
 * DESCRICAO: - feat: Biblioteca atualizada para criar um access point e conexão de internet.
 *            - docs: ESP32 32D - ESP-IDF v5.4.0
 * LINKS:
*/

// ========================================================================================================
// ---BIBLIOTECA---

#include "html.h"

// ========================================================================================================
/**
 * @brief Define o conteúdo index como uma string constante
 * 
 */
const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">

<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuração Wi-Fi</title>
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

        input {
            width: 100%;
            padding: 10px;
            margin-bottom: 10px;
            border-radius: 5px;
            border: 1px solid #ccc;
            font-size: 16px;
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
            width: 100%;
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
        <h1>Configuração Wi-Fi</h1>
        <div class="wifi-config">
            <input type="text" id="ssid" placeholder="Nome da rede Wi-Fi (SSID)">
            <input type="password" id="password" placeholder="Senha">
            <button onclick="connectWiFi()">Conectar</button>
        </div>
    </div>
    <footer>
        Desenvolvido por Adenilton R
    </footer>

    <script>
        function connectWiFi() {
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;

            // Verifica se o SSID e a senha foram preenchidos
            if (ssid.trim() === "" || password.trim() === "") {
                alert("Por favor, preencha o SSID e a senha.");
                return;
            }

            // Envia os dados para o servidor
            fetch('/wifi/connect', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ ssid: ssid, password: password }),
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Erro na requisição');
                }
                return response.text(); // Use .text() em vez de .json() se a resposta for uma string
            })
            .then(data => {
                alert(data); // Exibe a mensagem de sucesso
                redirectToPage(); // Redireciona para a página "Conectado" após sucesso
            })
            .catch((error) => {
                console.error('Erro:', error);
                alert("Erro ao conectar ao Wi-Fi. Tente novamente."); // Exibe uma mensagem de erro
            });
        }

        function redirectToPage() {
            // Redireciona para a página "Conectado"
            window.location.href = "/connected_html";

            // Faz uma requisição para fechar o AP e o servidor HTTP
            fetch('/close_ap', {
                method: 'GET',
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Erro ao fechar o AP');
                }
                return response.text();
            })
            .then(data => {
                console.log(data); // Exibe a mensagem de sucesso
            })
            .catch((error) => {
                console.error('Erro:', error);
            });
        }
    </script>
</body>
</html>
)rawliteral";

// ========================================================================================================
/**
 * @brief Define o conteúdo connected como uma string constante
 * 
 */
const char connected_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">

<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Conectado</title>
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

        footer {
            margin-top: 20px;
            font-size: 14px;
            color: #BDD959;
        }
    </style>
</head>

<body>
    <div class="container">
        <h1>Conectado com sucesso!</h1>
        <p>O ESP32 está conectado à rede Wi-Fi.</p>
    </div>
    <footer>
    </footer>
</body>
</html>
)rawliteral";