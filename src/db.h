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
    REDIS_USER_REGISTER,        /* User registration */
    REDIS_USER_GET_INFO,        /* Get individual user information */
    REDIS_USER_GET_ALL_INFO,
    REDIS_SET_FILE,             /* upload Encrypt file information*/
    REDIS_SET_MACHINE_FILE,     /* Record all files belonging to the same machine */
    REDIS_GET_FILE,             /* Get information about a single encrypted file */
    REDIS_GET_ALL_FILES,        /* Get all encrypted file information */
    REDIS_SET_TRACE,            /* Upload traceability information */
    REDIS_GET_TRACE             /* Get traceability information */
} Kdbtype;

typedef int (*synccallback)(redisReply *c, sds *out);
struct action {
    Kdbtype type;
    char *cmdline;
    synccallback syncexec;
};

/** @brief Save registered user data
 * 
 * @param data struct User object
 * @param outdate Output data in json format
 * @return Returns 0 on success, -1 otherwise
 */
int redis_user_register(void *data, sds *outdata);

/** @brief Get user information
 * 
 * @param data struct User object
 * @param outdate Output data in json format
 * @return Returns 0 on success, -1 otherwise
 */
int redis_get_user(void *data, sds *outdata);

/** @brief Record the information of a single encrypted file 
 *         so that it can be easily obtained when querying the owner.
 * 
 * @param data struct file object
 * @param outdate Output data in json format
 * @return Returns 0 on success, -1 otherwise
 */
int redis_upload_file(void *data, sds *outdata);

/** @brief Record encrypted files on the same machine. 
 *         In order to recover data, it is generally used 
 *         together with the redis_upload_file interface.
 * 
 * @param data struct file object
 * @param outdate Output data in json format
 * @return Returns 0 on success, -1 otherwise
 */
int redis_upload_machine_file(void *data, sds *outdata);

/** @brief Obtain the information of a single encrypted file 
 *         and call it when the file applies for authorization.
 * 
 * @param data file uuid
 * @param outdate Output data in json format
 * @return Returns 0 on success, -1 otherwise
 */
int redis_get_file(void *data, sds *outdata);

/** @brief Get information about all encrypted files on a machine
 * 
 * @param data Kfileall object
 * @param outdate Output data in json format
 * @return Returns 0 on success, -1 otherwise
 */
int redis_get_fileall(void *data, sds *outdata);

#endif