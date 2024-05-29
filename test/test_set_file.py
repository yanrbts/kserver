import requests
import json
import random
import string

cert_file_path = "/home/yrb/kserver/cert/client.pem"
ca_path = "/home/yrb/kserver/cert/rootCA.pem"
# 生成随机机器码
def generate_random_machine_code(length=32):
    return ''.join(random.choices(string.ascii_lowercase + string.digits, k=length))

def request_setfile(index):
    url = 'https://localhost/fileset'

    data = {
        "filename":f"file{index}",
        "uuid":f"fileuuid{index}",
        "filepath":f"/path/to/file{index}.txt",
        "machine":"f526255265340d994510f8d1652e1eb3"
    }

    json_data = json.dumps(data)

    try:
        response = requests.post(
            url, 
            data=json_data, 
            headers={'Content-Type': 'application/json'},
            cert=cert_file_path,
            verify=ca_path
        )
        if response.status_code == 200:
            print('Response:', response.json())
        else:
            print(f'Request failed with status code {response.status_code}')
            print('Response:', response.text)
    except requests.exceptions.RequestException as e:
        print(f"An error occurred: {e}")


def request_getfile(index):
    url = 'https://localhost/fileget'

    data = {
        "uuid":f"fileuuid{index}"
    }

    json_data = json.dumps(data)

    try:
        response = requests.post(
            url, 
            data=json_data, 
            headers={'Content-Type': 'application/json'},
            cert=cert_file_path,
            verify=ca_path
        )
        if response.status_code == 200:
            print('Response:', response.json())
        else:
            print(f'Request failed with status code {response.status_code}')
            print('Response:', response.text)
    except requests.exceptions.RequestException as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    for i in range(30):
        request_setfile(i)
        request_getfile(i)

