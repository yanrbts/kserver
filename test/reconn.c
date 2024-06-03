// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <hiredis/hiredis.h>

// gcc -ggdb reconn.c -o redisconn -lhiredis -L/lib/x86_64-linux-gnu/lib

#include <stdio.h>
#include <stdlib.h>
#include <hiredis/hiredis.h>

void hscan_with_pagination(redisContext *c, const char *key, const char *pattern, int count) {
    redisReply *reply;
    unsigned long long cursor = 0;
    char cursor_str[64];

    do {
        // 构建 HSCAN 命令
        snprintf(cursor_str, sizeof(cursor_str), "%llu", cursor);
        reply = (redisReply *)redisCommand(c, "HSCAN %s %s MATCH %s COUNT %d", key, cursor_str, pattern, count);

        if (reply == NULL) {
            printf("Command error: %s\n", c->errstr);
            redisFree(c);
            exit(EXIT_FAILURE);
        }

        if (reply->type != REDIS_REPLY_ARRAY || reply->elements != 2) {
            printf("Unexpected reply format\n");
            freeReplyObject(reply);
            redisFree(c);
            exit(EXIT_FAILURE);
        }

        // 获取新的游标
        cursor = strtoull(reply->element[0]->str, NULL, 10);

        // 处理返回的字段和值
        for (size_t i = 0; i < reply->element[1]->elements; i += 2) {
            // printf("Field: %s, Value: %lld\n", reply->element[1]->element[i]->str, cursor/*reply->element[1]->element[i + 1]->str*/);
        }
        printf("%ld, cursor : %lld\n", reply->element[1]->elements, cursor);

        // 释放回复对象
        freeReplyObject(reply);

    } while (cursor != 0);
}

int main(void) {
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    redisContext *c = redisConnectWithTimeout("127.0.0.1", 6379, timeout);

    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        return EXIT_FAILURE;
    }

    hscan_with_pagination(c, "filekey:fileuuid7", "trace:*", 100);

    redisFree(c);
    return EXIT_SUCCESS;
}
