from flask import Flask, request, jsonify, render_template

app = Flask(__name__)

stored_json_data = None

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/post_json', methods=['POST'])
def post_json():
    global stored_json_data
    json_data = request.get_json()

    if json_data:
        stored_json_data = json_data
        return jsonify({'message': 'JSON data received and stored'}), 200
    else:
        return jsonify({'error': 'No JSON data received'}), 400

@app.route('/get_json', methods=['GET'])
def get_json():
    global stored_json_data
    if stored_json_data:
        return stored_json_data, 200, {'Content-Type': 'application/json'}
    else:
        return jsonify({'error': 'No JSON data stored'}), 404

if __name__ == '__main__':
    app.run(debug=True)
