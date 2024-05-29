import requests
import ssl

# 创建自定义的 SSLContext 对象
ssl_context = ssl.create_default_context()

# 设置所需的加密套件和协议版本
ssl_context.set_ciphers('ECDHE-RSA-AES256-GCM-SHA384:DES-CBC3-SHA:AES128-SHA:AES128-GCM-SHA256')
ssl_context.options |= ssl.OP_NO_SSLv3
ssl_context.options |= ssl.OP_NO_SSLv2

# 创建会话
session = requests.Session()
adapter = requests.adapters.HTTPAdapter(pool_connections=10, pool_maxsize=10)
session.mount('https://', adapter)

# 设置自定义的 SSLContext 到适配器
adapter.init_poolmanager(ssl_context=ssl_context,connections=10, maxsize=100)

# 发送请求
response = session.get('https://localhost/fileget')

print(response.status_code)
