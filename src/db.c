/*
 * Copyright (c) 2024-2024, Yanruibing <yanruibing@kxyk.com> All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "kserver.h"

static void kx_set_reply(redisReply *reply, sds *outdata);

struct action acs[] = {
    {.type = REDIS_USER_REGISTER, .cmdline = "HMSET nodekey:%s uuid %s", .syncexec = kx_set_reply},
};

#define ACSIZE sizeof(acs)/sizeof(acs[0])

static struct action *kx_search_action(Kdbtype type) {
    struct action *ac = NULL;
    for (int i = 0; i < ACSIZE; i++) {
        if (acs[i].type == type)
            ac = &(acs[i]);
    }
    return ac;
}

static void kx_set_reply(redisReply *reply, sds *outdata) {
    if (reply && reply->str) {
        printf("%s\n", reply->str);
    }
    freeReplyObject(reply);
}

static redisReply *kx_command(redisContext *c, const char *cmd) {
    redisReply  *reply = NULL;
    reply = redisCommand(c, cmd);

    if (reply == NULL) {
        log_error("redis error: %s (%s)", c->errstr ? c->errstr : "unknown error", cmd);
    }
    return reply;
}

static redisReply *kx_sync_send_cmd(redisContext *c, const char *fmt, ...) {
    int                 size = 0;
    va_list             ap;
    char                *ptr = NULL;
    redisReply          *reply = NULL;

    va_start(ap, fmt);
    size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (size < 0)
        goto ret;
    size++;

    ptr = zmalloc(size);
    if (ptr == NULL)
        goto ret;

    va_start(ap, fmt);
    size = vsnprintf(ptr, size, fmt, ap);
    va_end(ap);

    if (size < 0) {
        free(ptr);
        goto ret;
    }
    log_info("cmd : %s", ptr);
    reply = kx_command(c, ptr);

    zfree(ptr);
ret:
    if (ptr) zfree(ptr);
    return reply;
}

Ksyncredis *redis_init(const char *addr, uint32_t port) {
    struct timeval timeout = {1, 500000}; // 1.5 seconds

    Ksyncredis *redis = zmalloc(sizeof(*redis));
    if (redis == NULL) {
        log_error("zmalloc Ksyncredis error");
        exit(1);
    }

    redis->context = redisConnectWithTimeout(addr, port, timeout);
    if (redis->context == NULL || redis->context->err) {
        if (redis->context) {
            log_error("redis Connection error: %s", redis->context->errstr);
            redisFree(redis->context);
        } else {
            log_error("redis Connection error: can't allocate redis context");
        }
        exit(1);
    }
    return redis;
}

int redis_user_register(Ksyncredis *redis, void *data) {
    struct action   *ac = NULL;
    redisReply      *reply = NULL;
    Kuser           *u;
    sds             out;

    u = (Kuser*)data;
    if (u == NULL)
        return -1;
    
    ac = kx_search_action(REDIS_USER_REGISTER);
    if (ac) {
        reply = kx_sync_send_cmd(redis->context,
                            ac->cmdline, 
                            u->machine,
                            u->username);
        ac->syncexec(reply, &out);
        return 0;
    }
    return -1;
}