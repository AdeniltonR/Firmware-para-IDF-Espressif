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
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial, sans-serif; background-color: #2D3340; color: #fff; display: flex; flex-direction: column; justify-content: center; align-items: center; min-height: 100vh; }
        .container { background-color: #3A3F4B; padding: 20px; border-radius: 10px; box-shadow: 0 4px 10px rgba(0,0,0,0.3); text-align: center; width: 90%; max-width: 400px; }
        h1 { margin-bottom: 20px; font-size: 24px; }
        input { width: 100%; padding: 10px; margin-bottom: 10px; border-radius: 5px; border: 1px solid #ccc; font-size: 16px; }
        button { background-color: #BDD959; border: none; color: #2D3340; padding: 15px 30px; font-size: 18px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s ease; width: 100%; margin-bottom: 10px; }
        button:hover { background-color: #A8C34A; }
        .btn-scan { background-color: #4A90E2; color: white; }
        .btn-scan:hover { background-color: #357ABD; }
        .network-list { margin-top: 20px; text-align: left; max-height: 300px; overflow-y: auto; }
        .network-item { background-color: #2D3340; padding: 10px; margin-bottom: 5px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s ease; }
        .network-item:hover { background-color: #BDD959; color: #2D3340; }
        .network-ssid { font-weight: bold; }
        .network-rssi { font-size: 12px; color: #BDD959; float: right; }
        .loading { text-align: center; padding: 10px; color: #BDD959; }
        footer { margin-top: 20px; font-size: 14px; color: #BDD959; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Configuração Wi-Fi</h1>
        <button class="btn-scan" onclick="scanNetworks()">Escanear Redes</button>
        <div id="networkList" class="network-list"></div>
        <div class="wifi-config">
            <input type="text" id="ssid" placeholder="Nome da rede Wi-Fi (SSID)">
            <input type="password" id="password" placeholder="Senha">
            <button onclick="connectWiFi()">Conectar</button>
        </div>
    </div>
    <footer>Desenvolvido por Adenilton R</footer>
    
    <script>
        async function scanNetworks() {
            const networkListDiv = document.getElementById('networkList');
            networkListDiv.innerHTML = '<div class="loading">Escaneando redes...</div>';
            try {
                const response = await fetch('/wifi/scan', { method: 'GET' });
                if (!response.ok) throw new Error('Erro no scan');
                const data = await response.json();
                // Passa os dados recebidos. Suporta tanto um array direto quanto um objeto {networks: []}
                displayNetworks(data.networks || data);
            } catch (error) {
                console.error('Erro:', error);
                networkListDiv.innerHTML = '<div class="loading">❌ Erro ao escanear redes. Tente novamente.</div>';
            }
        }

        function displayNetworks(networks) {
            const networkListDiv = document.getElementById('networkList');
            if (!networks || networks.length === 0) {
                networkListDiv.innerHTML = '<div class="loading">📡 Nenhuma rede encontrada.</div>';
                return;
            }
            
            // Ordena pelo sinal mais forte
            networks.sort((a, b) => b.rssi - a.rssi);
            
            const uniqueNetworks = [];
            const seenSSIDs = new Set();
            
            // Remove duplicadas usando o Set
            for (let i = 0; i < networks.length; i++) {
                const network = networks[i];
                if (network.ssid && network.ssid.trim() !== '') {
                    if (!seenSSIDs.has(network.ssid)) {
                        seenSSIDs.add(network.ssid);
                        uniqueNetworks.push(network);
                    }
                }
            }
            
            // Monta o HTML
            let html = '';
            for (let i = 0; i < uniqueNetworks.length; i++) {
                const network = uniqueNetworks[i];
                let signalStrength = getSignalStrength(network.rssi);
                
                html += '<div class="network-item" onclick="selectNetwork(\'' + escapeHtml(network.ssid) + '\')">';
                html += '<span class="network-ssid">' + escapeHtml(network.ssid) + '</span>';
                html += '<span class="network-rssi">' + signalStrength + ' (' + network.rssi + ' dBm)</span></div>';
            }
            networkListDiv.innerHTML = html || '<div class="loading">📡 Nenhuma rede disponível.</div>';
        }

        function getSignalStrength(rssi) {
            if (rssi > -50) return 'Excelente';
            if (rssi > -60) return 'Muito Bom';
            if (rssi > -70) return 'Bom';
            if (rssi > -80) return 'Regular';
            return 'Fraco';
        }

        function selectNetwork(ssid) {
            document.getElementById('ssid').value = ssid;
            document.getElementById('password').focus();
        }

        function escapeHtml(text) {
            return text.replace(/[&<>]/g, function(m) {
                if (m === '&') return '&amp;';
                if (m === '<') return '&lt;';
                if (m === '>') return '&gt;';
                return m;
            });
        }

        async function connectWiFi() {
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            
            if (ssid.trim() === "") {
                alert("Por favor, preencha o SSID.");
                return;
            }
            
            try {
                const response = await fetch('/wifi/connect', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ ssid: ssid, password: password })
                });
                
                if (!response.ok) throw new Error('Erro na requisicao');
                const data = await response.text();
                alert(data);
                
                // Redireciona de forma silenciosa para evitar o ERR_CONNECTION_REFUSED
                redirectToPage();
            } catch (error) {
                console.error('Erro:', error);
                alert("Erro ao conectar ao Wi-Fi. Tente novamente.");
            }
        }

        function redirectToPage() {
            // Substitui a interface da página sem recarregar
            document.body.innerHTML = `
                <div class="container">
                    <h1>Configuração Concluída!</h1>
                    <p style="margin-bottom: 15px;">O ESP32 salvou a rede e está reiniciando para se conectar à internet.</p>
                    <p><strong>Você já pode fechar esta página.</strong></p>
                </div>
            `;

            // Manda o ESP32 fechar o AP e reiniciar
            fetch('/close_ap', { method: 'GET' })
                .then(function(response) {
                    console.log("Comando de reinício enviado.");
                })
                .catch(function(error) {
                    console.log("Conexão encerrada pelo ESP32 (Reiniciando).");
                });
        }

        window.onload = function() { scanNetworks(); };
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