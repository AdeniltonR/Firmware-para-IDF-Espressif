#include "html.h"

// Define o conteúdo HTML como uma string constante
const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">

<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Controle do LED</title>
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

        .buttons {
            display: flex;
            flex-direction: column;
            gap: 10px;
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
        <h1>Controle do LED</h1>
        <div class="buttons">
            <button onclick="fetch('/led/on')">Ligar LED</button>
            <button onclick="fetch('/led/off')">Desligar LED</button>
        </div>
    </div>
    <footer>
        Desenvolvido por Adenilton R
    </footer>
</body>
</html>
)rawliteral";