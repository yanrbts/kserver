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
#ifndef __DB__
#define __DB__

typedef enum Kdbtype {
    REDIS_USER_REGISTER,    /* User registration */
    REDIS_USER_INFO,        /* Get individual user information */
    REDIS_USER_ALL_INFO
} Kdbtype;

typedef struct Ksyncredis {
    redisContext *context;
} Ksyncredis;

typedef void (*synccallback)(redisReply *c, sds *outdata);
struct action {
    Kdbtype type;
    char *cmdline;
    synccallback syncexec;
};

/** @brief Init redis context Create redis sync link object
 * 
 * @param addr redis-server ip address eg. : 127.0.0.1
 * @param port redis-server port eg. : 6790
 * @return Returns the created object, or NULL if failed
 */
Ksyncredis *redis_init(const char *addr, uint32_t port);

/** @brief Save registered user data
 * 
 * @param redis redis object 
 * @param data struct User object
 * @return Returns 0 on success, -1 otherwise
 */
int redis_user_register(Ksyncredis *redis, void *data);

#endif