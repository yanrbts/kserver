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

static int kx_post_reply(redisReply *reply, sds *out);
static int kx_hgetall_userinfo(redisReply *reply, sds *out);

struct action acs[] = {
    /* redis HMSET key field value [field value ...]
     * Sets the specified fields to their respective values in the hash stored at key. 
     * This command overwrites any specified fields already existing in the hash.
     * If key does not exist, a new key holding a hash is created. */
    {.type = REDIS_USER_REGISTER, .cmdline = "HMSET userkey:%s uuid %s username %s", .syncexec = kx_post_reply},
    /* Returns all fields and values of the hash stored at key. In the returned value, 
     * every field name is followed by its value, so the length of the reply is twice
     * the size of the hash.*/
    {.type = REDIS_USER_GET_INFO, .cmdline = "HGETALL userkey:%s", .syncexec = kx_hgetall_userinfo},
    /* SCAN cursor [MATCH pattern] [COUNT count] [TYPE type]
     * SCAN is a cursor based iterator. This means that at every call of the command, 
     * the server returns an updated cursor that the user needs to use as the cursor 
     * argument in the next call.*/
    {.type = REDIS_USER_GET_ALL_INFO, .cmdline = "SCAN %d MATCH userkey:* COUNT %d", .syncexec = kx_post_reply},
    /* HSET key field value [field value ...]
     * Sets the specified fields to their respective values in the hash stored at key.
     * This command overwrites the values of specified fields that exist in the hash. 
     * If key doesn't exist, a new key holding a hash is created.
     * example:
     * HSET filekey:file1uuid file1uuid '{"uuid":"file1","filename":"file1.txt","filepath":"/path/to/file1.txt"}' */
    {.type = REDIS_SET_FILE, .cmdline = "HSET filekey:%s %s %s", .syncexec = kx_post_reply},
    /* HSET machine:machineuuid file1uuid '{"uuid":"file1","filename":"file1.txt","filepath":"/path/to/file1.txt"}' */
    {.type = REDIS_SET_MACHINE_FILE, .cmdline = "HSET machine:%s %s %s", .syncexec = kx_post_reply},
    /* HGET key field
     * Returns the value associated with field in the hash stored at key. 
     * example:
     * HGET filekey:machine file1uuid */
    {.type = REDIS_GET_FILE, .cmdline = "HGET filekey:%s %s", .syncexec = kx_post_reply},
    /* HSCAN key cursor [MATCH pattern] [COUNT count] [NOVALUES]
     * O(1) for every call. O(N) for a complete iteration, including enough 
     * command calls for the cursor to return back to 0. N is the number of 
     * elements inside the collection.
     * example:
     * HSCAN filekey:machine 0 count 10 */
    {.type = REDIS_GET_ALL_FILES, .cmdline = "HSCAN filekey:%s %d COUNT %d", .syncexec = kx_post_reply}
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

/* When inserting data using the post method, redis returns ‘OK’. 
 * This method is generally used to process redis replies.
 * Returns 0 on success, -1 otherwise */
static int kx_post_reply(redisReply *reply, sds *out) {
    int ret = -1;

    if (reply) {
        switch (reply->type) {
        case REDIS_REPLY_INTEGER:
            {
                if (reply->integer == 1) {
                    ret = 0;
                    *out = sdsnew(STROK);
                }
            }
            break;
        case REDIS_REPLY_STATUS:
            {
                if (strcmp("OK", reply->str) == 0) {
                    ret = 0;
                    *out = sdsnew(STROK);
                } 
            }
        }
    }

    freeReplyObject(reply);
    return ret;
}

/* Query single user information through uuid 
 * and obtain returned user data ,
 * Returns 0 on success, -1 otherwise*/
static int kx_hgetall_userinfo(redisReply *reply, sds *out) {
    int ret = -1;
    cJSON *json = NULL;
    /* It is just to determine whether the data is queried. 
     * If it is not queried, null is returned. In this way, 
     * failure data can be customized by judging NULL.*/
    int flag = 0;

    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        json = cJSON_CreateObject();

        if (json == NULL) {
            log_error("HGETALL user info to create JSON object\n");
            goto ret;
        }
        
        for (size_t i = 0; i < reply->elements; i += 2) {
            cJSON *n;
            flag++;
            n = cJSON_CreateString(reply->element[i+1]->str);
            cJSON_AddItemToObject(json, reply->element[i]->str, n);
        }
        /* The flag flag is a flag used to determine whether there is data. 
         * If there is data, data is returned. If there is no data, NULL 
         * is returned.*/
        if (flag) {
            *out = sdsnew(cJSON_Print(json));
            ret = 0;
        }   
    }
ret:
    if (json) cJSON_Delete(json);
    freeReplyObject(reply);
    return ret;
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
        goto ret;
    }
    log_info("%s", ptr);
    reply = kx_command(c, ptr);
ret:
    if (ptr) zfree(ptr);
    return reply;
}

/* Create a redis link context, create one for each 
 * link separately, and release it after use*/
static redisContext *create_redis_ctx() {
    redisContext *ctx = NULL;
    struct timeval timeout = {1, 500000}; // 1.5 seconds

    ctx = redisConnectWithTimeout(server.redisip, server.redisport, timeout);
    if (ctx == NULL || ctx->err) {
        if (ctx) {
            log_error("redis Connection error: %s", ctx->errstr);
            redisFree(ctx);
        } else {
            log_error("redis Connection error: can't allocate redis context");
        }
        exit(1);
    }
    return ctx;
}

Ksyncredis *redis_init(const char *addr, uint32_t port) {
    struct timeval timeout = {20, 500000}; // 1.5 seconds

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

int redis_user_register(void *data, sds *outdata) {
    struct action   *ac = NULL;
    redisReply      *reply = NULL;
    redisContext    *ctx;
    Kuser           *u;

    u = (Kuser*)data;
    if (u == NULL) {
        return -1;
    }
    
    ctx = create_redis_ctx();
    if (ctx) {
        ac = kx_search_action(REDIS_USER_REGISTER);
        if (ac) {
            reply = kx_sync_send_cmd(ctx,
                                ac->cmdline, 
                                u->machine,
                                u->machine,
                                u->username);
            if (ac->syncexec(reply, outdata) == 0) {
                redisFree(ctx);
                return 0;
            }
        }
        redisFree(ctx);
    }
    return -1;
}

int redis_get_user(void *data, sds *outdata) {
    struct action   *ac = NULL;
    redisReply      *reply = NULL;
    redisContext    *ctx;
    sds             machine;

    machine = (sds)data;
    if (machine == NULL) {
        return -1;
    }

    ctx = create_redis_ctx();
    if (ctx) {
        ac = kx_search_action(REDIS_USER_GET_INFO);
        if (ac) {
            reply = kx_sync_send_cmd(ctx,
                                ac->cmdline, 
                                machine);
            if (ac->syncexec(reply, outdata) == 0) {
                redisFree(ctx);
                return 0;
            } 
        }
        redisFree(ctx);
    }
    return -1;
}

int redis_upload_file(void *data, sds *outdata) {
    struct action   *ac = NULL;
    redisReply      *reply = NULL;
    redisContext    *ctx;
    Kfile           *f;

    f = (Kfile*)data;
    if (f == NULL) {
        return -1;
    }

    ctx = create_redis_ctx();
    if (ctx) {
        ac = kx_search_action(REDIS_SET_FILE);
        if (ac) {
            reply = kx_sync_send_cmd(ctx,
                                ac->cmdline, 
                                f->machine,
                                f->uuid,
                                f->data);
            if (ac->syncexec(reply, outdata) == 0) {
                redisFree(ctx);
                return 0;
            } 
        }
        redisFree(ctx);
    }
    return -1;
}