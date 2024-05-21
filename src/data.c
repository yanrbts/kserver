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

const char *STROK = "{\"flag\":\"OK\", \"msg\":\"success\"}";
const char *STRFAIL = "{\"flag\":\"FAIL\", \"msg\":\"failed\"}";

const char *js_user_register_data(char *buf, size_t len, redis_data_save dbfunc) {
    cJSON *root;
    cJSON *jaction, *jmachine, *juname, *jpwd;
    Kuser user = {NULL};

    root = cJSON_ParseWithLength(buf, len);

    if (root == NULL) {
        log_error("user register json data parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    /* action */
    jaction = cJSON_GetObjectItem(root, "action");
    if (cJSON_IsString(jaction) && (jaction->valuestring != NULL)) {
        user.action = sdsnew(jaction->valuestring);
    } else {
        log_error("json get object 'action' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    /* machine */
    jmachine = cJSON_GetObjectItem(root, "machine");
    if (cJSON_IsString(jmachine) && (jmachine->valuestring != NULL)) {
        user.machine = sdsnew(jmachine->valuestring);
    } else {
        log_error("json get object 'machine' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    /* username */
    juname = cJSON_GetObjectItem(root, "username");
    if (cJSON_IsString(juname) && (juname->valuestring != NULL)) {
        user.username = sdsnew(juname->valuestring);
    } else {
        log_error("json get object 'username' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    /* password */
    jpwd = cJSON_GetObjectItem(root, "password");
    if (cJSON_IsString(jpwd) && (jpwd->valuestring != NULL)) {
        user.pwd = sdsnew(jpwd->valuestring);
    } else {
        log_error("json get object 'password' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    cJSON_Delete(root);

    /* save data to redis */
    if (dbfunc) {
        if (dbfunc(server.redis, (void*)&user) != 0)
            goto err;
    }

    if (user.action) sdsfree(user.action);
    if (user.machine) sdsfree(user.machine);
    if (user.username) sdsfree(user.username);
    if (user.pwd) sdsfree(user.pwd);

    return STROK;
err:
    if (user.action) sdsfree(user.action);
    if (user.machine) sdsfree(user.machine);
    if (user.username) sdsfree(user.username);
    if (user.pwd) sdsfree(user.pwd);

    return STRFAIL;
}

const char *js_user_login_data(char *buf, size_t len, redis_data_save dbfunc) {
    static uint32_t j = 0;
    printf("[%u] login : %s\n", ++j, buf);
    return STROK;
}