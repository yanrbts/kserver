import redis

def hscan_with_cursor(redis_client, key, match, count):
    cursor = 0
    while True:
        cursor, data = redis_client.hscan(key, cursor=cursor, match=match, count=count)
        if data:
            # for field, value in data.items():
            #     print(f"Field: {field}, Value: {value}")
            # print(f"cur : {cursor}")
            pass
        if cursor == 0:
            print("end")
            break

def main():
    # 连接 Redis 服务器
    redis_client = redis.StrictRedis(host='127.0.0.1', port=6379, db=0)
    
    # 执行 HSCAN
    key = "filekey:fileuuid7"
    match = "trace:*"
    count = 100
    
    for i in range(100):
        print(f"start {i}")
        hscan_with_cursor(redis_client, key, match, count)

if __name__ == "__main__":
    main()
