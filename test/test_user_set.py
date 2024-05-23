import requests
import json
import random
import string
import time

# 目标 URL，请替换为实际的服务器 URL
url = 'http://127.0.0.1:8099/userregister'

# 发送请求的函数
def send_request():
    data = {
        "machine": "f526255265340d994510f8d1652e1eb8",
        "username": "15727311933",
        "flag": 0
    }
    json_data = json.dumps(data)
    response = requests.post(url, data=json_data, headers={'Content-Type': 'application/json'})

    if response.status_code == 200:
        print(f'Request was successful')
        print('Response:', response.json())
    else:
        print(f'Request failed with status code {response.status_code}')
        print('Response:', response.text)

if __name__ == "__main__":
    send_request()
