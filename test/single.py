import requests
import json
import threading
import time
import random

# 目标 URL
urls = [
    'http://127.0.0.1:8099/userregister',  # 请替换为实际的服务器 URL
    'http://127.0.0.1:8099/userlogin'
]

# 要发送的 JSON 数据
data = {
    "action": "ENCRYPT",
    "msg": "f526255265340d994510f8d1652e1eb1"
}

# 将字典转换为 JSON 字符串
json_data = json.dumps(data)

# 发送请求的函数
def send_request():
    try:
        # 发送 POST 请求
        url = random.choice(urls)
        response = requests.post(url, data=json_data, headers={'Content-Type': 'application/json'})

        # 检查响应状态码
        if response.status_code == 200:
            print('Request was successful')
            print('Response:', response.json())
        else:
            print(f'Request failed with status code {response.status_code}')
            print('Response:', response.text)
    except Exception as e:
        print(f'An error occurred: {e}')

        # 等待一段时间再发送下一次请求（例如，1秒）
        #time.sleep(1)

# 创建并启动多个线程
threads = []
num_threads = 1  # 设定线程数，根据需要调整

for _ in range(num_threads):
    thread = threading.Thread(target=send_request)
    thread.start()
    threads.append(thread)

# 等待所有线程完成（实际情况下，可以使用其他方式停止循环）
for thread in threads:
    thread.join()