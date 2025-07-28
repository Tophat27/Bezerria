from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route('/temperature', methods=['POST'])
def handle_temperature():
    data = request.json
    print(f"Temperatura recebida: {data['temp']}°C")
    return jsonify({"status": "success"}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)  # Permite conexões locais
    