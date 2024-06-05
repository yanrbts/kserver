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
const char *STRNOFOUND = "{\"flag\":\"NOFOUND\", \"msg\":\"File not found\"}";
const char *STRERROR = "{\"flag\":\"ERROR\", \"msg\":\"Server Error\"}";

sds kx_user_register(char *buf, size_t len) {
    cJSON *root = NULL;
    cJSON *jmachine, *juname, *jflag;
    sds outdata = NULL;
    Kuser user;

    memset(&user, 0, sizeof(Kuser));

    root = cJSON_ParseWithLength(buf, len);

    if (root == NULL) {
        log_error("user register json data parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    /* machine */
    jmachine = cJSON_GetObjectItem(root, "machine");
    if (cJSON_IsString(jmachine) && (jmachine->valuestring != NULL)) {
        user.machine = sdsnew(jmachine->valuestring);
    } else {
        log_error("json user register object 'machine' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    /* username */
    juname = cJSON_GetObjectItem(root, "username");
    if (cJSON_IsString(juname) && (juname->valuestring != NULL)) {
        user.username = sdsnew(juname->valuestring);
    } else {
        log_error("json user register object 'username' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    /* flag */
    jflag = cJSON_GetObjectItem(root, "flag");
    if (cJSON_IsNumber(jflag)) {
        user.flag = jflag->valueint;
    } else {
        log_error("json user register object 'flag' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    /* Login logic 1. First determine whether the flag is 1. 
     * If flag=1, insert new user information directly. 
     * 
     * If flag=0, first check whether the user information exists. 
     * If it exists, it will be directly returned to the client. 
     * If it does not exist, insert it and send it to the client. user information*/
    if (user.flag == 1) {
        /* insert new user information directly */
        if (redis_user_register((void*)&user, &outdata) != 0) {
            goto err;
        } else {
            /* User data inserted successfully */
            sdsfree(outdata);
            outdata = NULL;
            cJSON_DeleteItemFromObject(root, "flag");
            char *jstr = cJSON_Print(root);
            outdata = sdsnew(jstr);
            free(jstr);
            log_info("(%s) User register successfully.", user.username);
        }
    } else {
        if (redis_get_user((void*)user.machine, &outdata) != 0) {
            /* No user data was found, insert user data */
            if (redis_user_register((void*)&user, &outdata) != 0) {
                log_error("(%s) User register failed.", user.username);
                goto err;
            } else {
                /* User data inserted successfully */
                sdsfree(outdata);
                outdata = NULL;
                cJSON_DeleteItemFromObject(root, "flag");
                char *jstr = cJSON_Print(root);
                outdata = sdsnew(jstr);
                free(jstr);
                log_info("(%s) User register successfully.", user.username);
            }
        } else {
            log_info("(%s) User already exists", user.username);
        }
    }

    if (user.machine) sdsfree(user.machine);
    if (user.username) sdsfree(user.username);
    cJSON_Delete(root);

    return outdata;
err:
    if (user.machine) sdsfree(user.machine);
    if (user.username) sdsfree(user.username);
    if (root) cJSON_Delete(root);

    outdata = sdsnew(STRFAIL);
    return outdata;
}

sds kx_user_get(char *buf, size_t len) {
    cJSON *root;
    cJSON *jm;
    sds outdata = NULL;
    sds sm = sdsempty();

    root = cJSON_ParseWithLength(buf, len);
    if (root == NULL) {
        log_error("user register json data parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    /* machine */
    jm = cJSON_GetObjectItem(root, "machine");
    if (cJSON_IsString(jm) && (jm->valuestring != NULL)) {
        sm = sdscat(sm, jm->valuestring);
    } else {
        log_error("json get object 'action' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    cJSON_Delete(root);

    if (redis_get_user((void*)sm, &outdata) != 0) {
        goto err;
    }

    sdsfree(sm);
    return outdata;
err:
    sdsfree(sm);
    outdata = sdsnew(STRFAIL);
    return outdata;
}

sds kx_file_set(char *buf, size_t len) {
    cJSON *root = NULL;
    cJSON *jm, *juuid;
    sds outdata = NULL;
    Kfile f;

    memset(&f, 0, sizeof(Kfile));

    root = cJSON_ParseWithLength(buf, len);
    if (root == NULL) {
        log_error("user register json data parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    /* machine */
    jm = cJSON_GetObjectItem(root, "machine");
    if (cJSON_IsString(jm) && (jm->valuestring != NULL)) {
        f.machine = sdsnew(jm->valuestring);
    } else {
        log_error("json file set 'action' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    /* file uuid */
    juuid = cJSON_GetObjectItem(root, "uuid");
    if (cJSON_IsString(juuid) && (juuid->valuestring != NULL)) {
        f.uuid = sdsnew(juuid->valuestring);
    } else {
        log_error("json file set 'action' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    
    char *jstr = cJSON_Print(root);
    f.data = sdsnew(jstr);
    free(jstr);
    
    if (redis_upload_file((void*)&f, &outdata) != 0) {
        goto err;
    } else {
        sdsfree(outdata);
        outdata = NULL;
    }

    /* Save file information to the hash table belonging 
     * to the machine for easy traversal*/
    if (redis_upload_machine_file((void*)&f, &outdata) != 0) {
        goto err;
    }

    if (f.data) sdsfree(f.data);
    if (f.machine) sdsfree(f.machine);
    if (f.uuid) sdsfree(f.uuid);
    cJSON_Delete(root);

    return outdata;
err:
    if (root) cJSON_Delete(root);
    if (f.data) sdsfree(f.data);
    if (f.machine) sdsfree(f.machine);
    if (f.uuid) sdsfree(f.uuid);
    if (outdata == NULL)
        outdata = sdsnew(STRFAIL);
    return outdata;
}

sds kx_file_get(char *buf, size_t len) {
    cJSON *root;
    cJSON *jm;
    sds outdata = NULL;
    sds sm = sdsempty();

    root = cJSON_ParseWithLength(buf, len);
    if (root == NULL) {
        log_error("file get json data parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    /* file uuid */
    jm = cJSON_GetObjectItem(root, "uuid");
    if (cJSON_IsString(jm) && (jm->valuestring != NULL)) {
        sm = sdscat(sm, jm->valuestring);
    } else {
        log_error("json file get object 'uuid' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    cJSON_Delete(root);

    if (redis_get_file((void*)sm, &outdata) != 0) {
        goto err;
    }

    sdsfree(sm);
    return outdata;
err:
    sdsfree(sm);
    if (outdata == NULL)
        outdata = sdsnew(STRFAIL);
    return outdata;
}

sds kx_file_getall(char *buf, size_t len) {
    cJSON *root = NULL;
    cJSON *jm, *jp;
    sds outdata = NULL;
    Kfileall fs;

    memset(&fs, 0, sizeof(Kfileall));

    root = cJSON_ParseWithLength(buf, len);
    if (root == NULL) {
        log_error("file getall json data parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    /* machine uuid */
    jm = cJSON_GetObjectItem(root, "machine");
    if (cJSON_IsString(jm) && (jm->valuestring != NULL)) {
        fs.machine = sdsnew(jm->valuestring);
    } else {
        log_error("json file getall object 'machine' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    /* page number */
    jp = cJSON_GetObjectItem(root, "page");
    if (cJSON_IsNumber(jp)) {
        fs.page = jp->valueint;
    } else {
        log_error("json file getall object 'page' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    
    if (redis_get_fileall((void*)&fs, &outdata) != 0) {
        goto err;
    }

    if (fs.machine) sdsfree(fs.machine);
    cJSON_Delete(root);

    return outdata;

err:
    if (root) cJSON_Delete(root);
    if (fs.machine) sdsfree(fs.machine);
    if (outdata == NULL)
        outdata = sdsnew(STRFAIL);
    return outdata;
}

sds kx_trace_set(char *buf, size_t len) {
    cJSON *root = NULL;
    cJSON *ju;
    sds outdata = NULL;
    Ktrace ft;

    memset(&ft, 0, sizeof(Ktrace));

    root = cJSON_ParseWithLength(buf, len);
    if (root == NULL) {
        log_error("file trace set, json data parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    /* file uuid */
    ju = cJSON_GetObjectItem(root, "uuid");
    if (cJSON_IsString(ju) && (ju->valuestring != NULL)) {
        ft.uuid = sdsnew(ju->valuestring);
    } else {
        log_error("file trace set, get object 'uuid' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    /* HSET filekey:fileuuid trace:1798000,*/
    ft.tracefield = sdsnew("trace:");
    ft.tracefield = sdscatfmt(ft.tracefield, "%U", ustime());

    char *jstr = cJSON_Print(root);
    ft.data = sdsnew(jstr);
    free(jstr);

    if (redis_set_trace((void*)&ft, &outdata) != 0)
        goto err;

    if (ft.uuid) sdsfree(ft.uuid);
    if (ft.tracefield) sdsfree(ft.tracefield);
    if (ft.data) sdsfree(ft.data);
    cJSON_Delete(root);
    return outdata;

err:
    if (root) cJSON_Delete(root);
    if (ft.uuid) sdsfree(ft.uuid);
    if (ft.tracefield) sdsfree(ft.tracefield);
    if (outdata == NULL)
        outdata = sdsnew(STRFAIL);
    return outdata;
}

sds kx_trace_get(char *buf, size_t len) {
    cJSON *root = NULL;
    cJSON *jt, *jp;
    sds outdata = NULL;
    Kgettrace fg;

    memset(&fg, 0, sizeof(Kgettrace));

    root = cJSON_ParseWithLength(buf, len);
    if (root == NULL) {
        log_error("trace json data parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    /* file uuid */
    jt = cJSON_GetObjectItem(root, "uuid");
    if (cJSON_IsString(jt) && (jt->valuestring != NULL)) {
        fg.uuid = sdsnew(jt->valuestring);
    } else {
        log_error("json trace object 'uuid' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }

    /* page number */
    jp = cJSON_GetObjectItem(root, "page");
    if (cJSON_IsNumber(jp)) {
        fg.page = jp->valueint;
    } else {
        log_error("json trace object 'page' parse error (%s).", cJSON_GetErrorPtr());
        goto err;
    }
    
    if (redis_get_trace((void*)&fg, &outdata) != 0) {
        goto err;
    }

    if (fg.uuid) sdsfree(fg.uuid);
    cJSON_Delete(root);

    return outdata;

err:
    if (root) cJSON_Delete(root);
    if (fg.uuid) sdsfree(fg.uuid);
    if (outdata == NULL) {
        outdata = sdsnew(STRFAIL);
    }
    
    return outdata;
}