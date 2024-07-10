from flask import Flask, jsonify
import os

app = Flask(__name__)

# Specify the folder where your JSON files are stored
JSON_FOLDER = 'json_files'  # Replace with the actual path to your folder

@app.route('/get_json/<filename>', methods=['GET'])
def get_json(filename):
    # Ensure the filename ends with .json
    if not filename.endswith('.json'):
        return jsonify({'error': 'Only JSON files are allowed'}), 400
    
    # Construct the full file path
    file_path = f"json_files/{filename}"
    # Check if the file exists
    if not os.path.exists(file_path):
        return jsonify({'error': 'File not found'}), 404

    # Read the file content
    with open(file_path, 'r') as f:
        content = f.read()

    # Return the content with the appropriate content type
    return content, 200, {'Content-Type': 'application/json'}

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
