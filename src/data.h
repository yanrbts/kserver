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
#ifndef __DATA__
#define __DATA__

#include <stddef.h>
#include "sds.h"
#include "db.h"

typedef int (*redis_data_save)(Ksyncredis *redis, void *data);

typedef struct Kuser {
    sds action;   /* function action (REGISTER or LOGIN) */
    sds machine;  /* machine code (uuid)*/
    sds username; /* username */
    sds pwd;      /* password */
} Kuser;

typedef struct Kfile {
    sds machine;  /* machine code (uuid)*/
    sds uuid;     /* file uuid */
    sds data;     /* json data */
} Kfile;

sds kx_user_register(char *buf, size_t len);
sds kx_user_get(char *buf, size_t len);
/** @brief Upload encrypted file information
 * 
 * @param buf Request data
 * @param len Request data length
 * @return Return success information, if failure returns failure information
 */
sds kx_file_set(char *buf, size_t len);

extern const char *STRFAIL;
extern const char *STROK;

#endif