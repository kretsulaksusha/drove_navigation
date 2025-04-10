from flask import Flask, request
import json
import os
import numpy as np 

app = Flask(__name__)

SAVE_FOLDER = 'received_images'
if not os.path.exists(SAVE_FOLDER):
    os.makedirs(SAVE_FOLDER)

def next_file_name():
    base_name = "image"
    extension = ".jpg"
    i = 1
    while True:
        file_name = f"{base_name}{i:04d}{extension}"
        if not os.path.exists(os.path.join(SAVE_FOLDER, file_name)):
            return file_name
        i += 1

@app.route('/', methods=['POST'])
def receive_image():
    if 'file' not in request.files:
        return "No file in request", 400
    file = request.files['file']

    if file.filename == '':
        return "No selected file", 400
    
    if file:
        filename = next_file_name()
        file.save(os.path.join(SAVE_FOLDER, filename))
        print(f"File saved as {os.path.join(SAVE_FOLDER, filename)}")
        
        return json.dumps( {'x': 0, 'y':0, 'w' : 0, 'h': 0} ), 200

    return "Error in saving file", 500

if __name__ == '__main__':
    app.run(port=20000)
