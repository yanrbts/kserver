import requests
import json
import random
import string
import time

# 目标 URL，请替换为实际的服务器 URL
url = 'http://127.0.0.1:8099/userregister'

# 随机生成的值的长度
machine_id_length = 32
username_length = 11
password_length = 8

# 生成随机字符串
def random_string(length):
    letters_and_digits = string.ascii_lowercase + string.digits
    return ''.join(random.choice(letters_and_digits) for i in range(length))

# 发送请求的函数
def send_request():
    data = {
        "action": "REGISTER",
        "machine": random_string(machine_id_length),
        "username": random_string(username_length),
        "password": random_string(password_length)
    }
    json_data = json.dumps(data)
    response = requests.post(url, data=json_data, headers={'Content-Type': 'application/json'})
    
    if response.status_code == 200:
        print('Request was successful')
        print('Response:', response.json())
    else:
        print(f'Request failed with status code {response.status_code}')
        print('Response:', response.text)
    # time.sleep(0.0)

# 发送 10 次请求
for _ in range(100000):
    send_request()
