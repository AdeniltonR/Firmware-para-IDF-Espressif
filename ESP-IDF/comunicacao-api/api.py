"""
NOME: Adenilton Ribeiro
DATA: 17/04/2025
PROJETO: Sistema de Comunicação API para Dados de Velocidade
VERSAO: 1.0.0

DESCRICAO:
- feat: API REST para recebimento de dados de equipamentos industriais
        - Processamento de dados de velocidade linear (m/s)
        - Armazenamento temporário em memória
        - Validação de dados recebidos
        - Documentação automática da API
- docs: ESP32 32D - ESP-IDF v5.4.0
        Protocolo HTTP/1.1
        Flask 2.3.2

HARDWARE COMPATÍVEL:
- Encoder de 200 PPR
- Polia de 160mm de diâmetro
- Sensores de velocidade linear

ENDPOINTS PRINCIPAIS:
- POST /api/device      -> Recebe dados do dispositivo
- GET  /api/device/<id> -> Consulta dados específicos
- GET  /api/docs        -> Documentação interativa
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
from datetime import datetime
import time

app = Flask(__name__)
CORS(app)  # Isso permite acesso público à API (apenas para desenvolvimento)

# Banco de dados em memória para armazenar os dados recebidos
devices_db = {}

@app.route('/api/device', methods=['POST'])
def receive_device_data():
    """
    Endpoint para receber dados do dispositivo ESP32
    Formato esperado:
    {
        "id": "string (identificador único do dispositivo)",
        "status": "string (produzindo, setup, manutenção, parado)",
        "velocidade": "float (metros/segundos)"
    }
    """
    try:
        data = request.get_json()
        
        # Validação dos dados recebidos
        if not data or 'id' not in data or 'status' not in data or 'velocidade' not in data:
            return jsonify({"error": "Dados incompletos"}), 400
        
        valid_statuses = ['produzindo', 'setup', 'manutenção', 'parado']
        if data['status'] not in valid_statuses:
            return jsonify({"error": f"Status inválido. Valores permitidos: {valid_statuses}"}), 400
        
        try:
            velocidade = float(data['velocidade'])
        except ValueError:
            return jsonify({"error": "Velocidade deve ser um número"}), 400
        
        # Adiciona timestamp aos dados recebidos
        data['timestamp'] = datetime.now().isoformat()
        
        # Armazena os dados no "banco de dados"
        device_id = data['id']
        devices_db[device_id] = data
        
        return jsonify({"message": "Dados recebidos com sucesso", "device_id": device_id}), 200
    
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/api/device/<device_id>', methods=['GET'])
def get_device_data(device_id):
    """
    Endpoint para recuperar os dados de um dispositivo específico
    """
    if device_id in devices_db:
        return jsonify(devices_db[device_id]), 200
    else:
        return jsonify({"error": "Dispositivo não encontrado"}), 404

@app.route('/api/devices', methods=['GET'])
def get_all_devices():
    """
    Endpoint para listar todos os dispositivos e seus dados
    """
    return jsonify(devices_db), 200

@app.route('/api/docs', methods=['GET'])
def get_documentation():
    """
    Endpoint que retorna a documentação da API
    """
    docs = {
        "documentation": {
            "description": "API para integração com dispositivos ESP32",
            "endpoints": {
                "/api/device": {
                    "method": "POST",
                    "description": "Envia dados do dispositivo",
                    "request_body": {
                        "id": "string (identificador do dispositivo)",
                        "status": "string (produzindo, setup, manutenção, parado)",
                        "velocidade": "float (metros/segundos)"
                    },
                    "response": "Confirmação de recebimento ou mensagem de erro"
                },
                "/api/device/<device_id>": {
                    "method": "GET",
                    "description": "Recupera os dados de um dispositivo específico",
                    "response": "Dados do dispositivo ou mensagem de erro"
                },
                "/api/devices": {
                    "method": "GET",
                    "description": "Lista todos os dispositivos e seus dados",
                    "response": "Dicionário com todos os dispositivos registrados"
                }
            },
            "recommendations": {
                "frequency": "Recomenda-se enviar dados a cada 5-10 segundos",
                "timeout": "Timeout de conexão de 5 segundos",
                "retry": "Em caso de falha, tentar novamente após 30 segundos"
            }
        }
    }
    return jsonify(docs), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)