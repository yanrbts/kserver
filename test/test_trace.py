import requests
import json
import random
import string

def random_string(length):
    letters_and_digits = string.ascii_lowercase + string.digits
    return ''.join(random.choice(letters_and_digits) for i in range(length))

def setfile():
    url = 'http://127.0.0.1:8099/fileset'

    data = {
        "filename":"file7",
        "uuid":"fileuuid7",
        "filepath":"/path/to/file7.txt",
        "machine":"f526255265340d994510f8d1652e1eb3"
    }

    json_data = json.dumps(data)

    response = requests.post(url, data=json_data, headers={'Content-Type': 'application/json'})

    if response.status_code == 200:
        print('Response:', response.json())
    else:
        print(f'Request failed with status code {response.status_code}')
        print('Response:', response.text)

def settrace():
    url = 'http://127.0.0.1:8099/filesettrace'

    data = {
        "machine":"f526255265340d994510f8d1652e1eb3",
        "uuid":"fileuuid7",
        "username":random_string(11),
        "time":"2024-05-23",
        "action":0
    }

    json_data = json.dumps(data)

    response = requests.post(url, data=json_data, headers={'Content-Type': 'application/json'})

    if response.status_code == 200:
        print('Response:', response.json())
    else:
        print(f'Request failed with status code {response.status_code}')
        print('Response:', response.text)

def gettracespage(page):
    url = 'http://127.0.0.1:8099/filegettrace'

    data = {
        "uuid":"fileuuid7",
        "page":page
    }

    json_data = json.dumps(data)

    response = requests.post(url, data=json_data, headers={'Content-Type': 'application/json'})

    if response.status_code == 200:
        return response.json()
    else:
        print(f'Request failed with status code {response.status_code}')
        print('Response:', response.text)
    

def gettraces():
    page = 0

    while True:
        response =  gettracespage(page)
        page = response["page"]
        if page != 0:
            # print('Response:', response)
            n = len(response['traces'])
            print(f"Number of traces: {n}")
            gettracespage(page)
        else:
            break

if __name__ == "__main__":
    setfile()
    for _ in range(30):
        settrace()
    gettraces()