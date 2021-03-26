#!/usr/bin/env python3
from os import getcwd

from flask import Flask

app = Flask(__name__, static_folder=getcwd())


@app.route('/')
def index():
    return app.send_static_file('login.html')


if __name__ == '__main__':
    app.run(debug=True, host='127.0.0.1', port=5000)