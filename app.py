from flask import Flask, render_template, request, jsonify
import requests

app = Flask(__name__)

# Endereço do ESP32
esp32_ip = "http://10.42.0.240"

# Variável para armazenar o valor da pressão
pressure_value = 0.0

# Rota principal que renderiza a interface web
@app.route('/')
def index():
    return render_template('index.html', pressure=pressure_value)

# Rota para controlar os compressores
@app.route('/control', methods=['POST'])
def control_compressors():
    compressor = request.form['compressor']
    state = request.form['state']
    
    # Enviar o comando para o ESP32
    url = f"{esp32_ip}/compressor"
    data = {'compressor': compressor, 'state': state}
    try:
        response = requests.post(url, data=data)
        return jsonify({"status": "success"}), 200
    except:
        return jsonify({"status": "error"}), 500

# Rota para receber o valor da pressão do ESP32
@app.route('/pressao', methods=['POST'])
def update_pressure():
    global pressure_value
    pressure_value = request.form['pressao']
    return jsonify({"status": "success"}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
