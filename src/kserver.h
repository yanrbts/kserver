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
#ifndef __KSERVER__
#define __KSERVER__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <civetweb.h>
#include <hiredis.h>
#include <read.h>
#include <net.h>

#include "zmalloc.h"
#include "sds.h"
#include "cJSON.h"
#include "data.h"
#include "db.h"
#include "util.h"
#include "log.h"

#define KSERVER_VERSION         "1.0.0"
#define REDIS_PAGENUM           100
#define MAXLEN                  1024
#define HTTP_OK                 200
#define HTTP_NOFOUND            404
#define HTTP_ROOT               "./api"
#define HTTP_PORT               "8099"
#define HTTP_REQUEST_MS         "10000"
// #define HTTPS_PORT              "80r,443s"
#define CONFIG_MAX_LINE         1024
#define CONFIG_DEFAULT_PID_FILE "/var/run/kserver.pid"
#define CONFIG_DEFAULT_LOGFILE  ""
#define CONFIG_REDIS_IP         "127.0.0.1"
#define CONFIG_REDIS_PORT       6379

#define CONFIG_CIVET_AUTH_DOMAIN    "localhost"
#define CONFIG_CIVET_DOMAIN_CHECK   "yes"
#define CONFIG_CIVET_CERT           "/home/yrb/kserver/cert/server.pem"
#define CONFIG_CIVET_CA             "/home/yrb/kserver/cert/rootCA.pem"
#define CONFIG_CIVET_DOMAIN_CHECK   "yes"
#define CONFIG_CIVET_SSL_NO         1
#define CONFIG_CIVET_SSLPROTOVOL    "4"
#define CONFIG_CIVET_SSLCIPHER      "TLS_AES_128_GCM_SHA256:AES256-SHA:HIGH:!aNULL:!MD5:!3DES"

#define CONFIG_CIVET_THREADS_NUM        "50"
#define CONFIG_CIVET_THREADS_PRESPAWN   "5"
#define CONFIG_CIVET_LISTEN_BACKLOG     "200"
#define CONFIG_CIVET_CONN_QUEUE         "20"

struct Server {
    struct mg_init_data init;
    struct mg_callbacks callbacks;
	struct mg_context *ctx;
    struct mg_server_port ports[32];
    struct mg_error_data error;
    uint64_t clients;                   /* Current number of connections */
    char *system_info;                  /* information on the system. Useful for support requests.*/
    /* configure */
    char *redisip;                      /* redis server ip address */
    uint32_t redisport;                 /* redis server port */
    redisContext *redisctx;
    char *configfile;                   /* Absolute config file path, or NULL */
    uint32_t pagenum;                   /* Redis paging query is the maximum number 
                                         * of query data items per page.*/
    char *httpport;                     /* web service configuration port */
    char *request_timeout;              /* Request timeout in milliseconds */
    char *auth_domain;                  /* config parameter of the domain being configured.*/
    char *auth_domain_check;            /* */
    char *ssl_certificate;              /* configuration parameter to the
                                         * file name (including path) of the resulting *.pem file.*/
    int  ssl_no;                        /* Whether to use SSL connection */
    char *ssl_ca_file;                  /* ca file path */
    char *ssl_protocol_version;         /* 4:TLS1.2, 2:TLS1.x Allow SSLv3 and TLS */
    char *ssl_cipher_list;              /* some strong cipher(s) */
    char *num_threads;                  /* Maximum number of worker threads allowed. 
                                         * CivetWeb handles each incoming connection in a separate thread*/
    char *prespawn_threads;             /* Number of worker threads that should be pre-spawned by mg_start() */
    char *listen_backlog;               /* Maximum number of connections waiting to be accepted 
                                         * by the server operating system.*/
    char *connection_queue;             /* Maximum number of accepted connections waiting to be dispatched 
                                         * by a worker thread.*/
    int daemonize;                      /* True if running as a daemon */
    char *pidfile;                      /* PID file path */
    char *logfile;                      /* log file */
    FILE *logfp;                        /* log file handle */
};

typedef sds (*json_parse_handler)(char *buf, size_t len);
struct ApiEntry {
    char *uri;                  /* HTTP URI */
    char *method;               /* POST / GET */
    json_parse_handler jfunc;   /* json parsing function */
};


/*-----------------------------------------------------------------------------
 * Extern declarations
 *----------------------------------------------------------------------------*/
long long ustime(void);
/* Configuration */
void loadServerConfig(char *filename);

extern struct Server server;

#endif