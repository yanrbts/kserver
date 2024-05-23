import requests
import json
import random
import string

# 生成随机机器码
def generate_random_machine_code(length=32):
    return ''.join(random.choices(string.ascii_lowercase + string.digits, k=length))

# 目标 URL
url = 'http://127.0.0.1:8099/fileget'  # 请替换为实际的服务器 URL

# 要发送的 JSON 数据
data = {
    "uuid":"file1uuid2"
}

# 将字典转换为 JSON 字符串
json_data = json.dumps(data)

# 发送 POST 请求
response = requests.post(url, data=json_data, headers={'Content-Type': 'application/json'})

# 检查响应状态码
if response.status_code == 200:
    print('Request was successful')
    print('Response:', response.json())
else:
    print(f'Request failed with status code {response.status_code}')
    print('Response:', response.text)
