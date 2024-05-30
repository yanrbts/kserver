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

int yesnotoi(char *s) {
    if (!strcasecmp(s,"yes")) return 1;
    else if (!strcasecmp(s,"no")) return 0;
    else return -1;
}

static void loadServerConfigFromString(char *config) {
    char *err = NULL;
    int linenum = 0, totlines, i;
    sds *lines;

    lines = sdssplitlen(config,strlen(config),"\n",1,&totlines);

    for (i = 0; i < totlines; i++) {
        sds *argv;
        int argc;

        linenum = i+1;
        lines[i] = sdstrim(lines[i], " \t\r\n");

        /* Skip comments and blank lines */
        if (lines[i][0] == '#' || lines[i][0] == '\0') continue;

        /* Split into arguments */
        argv = sdssplitargs(lines[i], &argc);
        if (argv == NULL) {
            err = "Unbalanced quotes in configuration line";
            goto loaderr;
        }

        /* Skip this line if the resulting command vector is empty. */
        if (argc == 0) {
            sdsfreesplitres(argv, argc);
            continue;
        }
        sdstolower(argv[0]);

        /* Execute config directives */
        if (!strcasecmp(argv[0], "redis-ip") && argc == 2) {
            zfree(server.redisip);
            server.redisip = argv[1][0] ? zstrdup(argv[1]) : NULL;
        } else if (!strcasecmp(argv[0], "redis-port") && argc == 2) {
            server.redisport = atoi(argv[1]);
            if (server.redisport < 0 ||
                server.redisport > 65535)
            {
                err = "Invalid port"; goto loaderr;
            }
        } else if (!strcasecmp(argv[0], "redis-page") && argc == 2) {
            server.pagenum = atoi(argv[1]);
        } else if (!strcasecmp(argv[0], "port") && argc == 2) {
            zfree(server.httpport);
            server.httpport = argv[1][0] ? zstrdup(argv[1]) : NULL;
        } else if (!strcasecmp(argv[0], "request_timeout_ms") && argc == 2) {
            zfree(server.request_timeout);
            server.request_timeout = argv[1][0] ? zstrdup(argv[1]) : NULL;
        } else if (!strcasecmp(argv[0], "daemonize") && argc == 2) {
            if ((server.daemonize = yesnotoi(argv[1])) == -1) {
                err = "argument must be 'yes' or 'no'"; goto loaderr;
            }
        } else if (!strcasecmp(argv[0], "pidfile") && argc == 2) {
            zfree(server.pidfile);
            server.pidfile = zstrdup(argv[1]);
        } else if (!strcasecmp(argv[0], "logfile") && argc == 2) {
            FILE *logfp;

            zfree(server.logfile);
            server.logfile = zstrdup(argv[1]);
            if (server.logfile[0] != '\0') {
                /* Test if we are able to open the file. The server will not
                 * be able to abort just for this problem later... */
                logfp = fopen(server.logfile,"a");
                if (logfp == NULL) {
                    err = sdscatprintf(sdsempty(),
                        "Can't open the log file: %s", strerror(errno));
                    goto loaderr;
                }
                fclose(logfp);
            }
        } else if (!strcasecmp(argv[0], "auth_domain") && argc == 2) {
            zfree(server.auth_domain);
            server.auth_domain = zstrdup(argv[1]);
        } else if (!strcasecmp(argv[0], "auth_domain_check") && argc == 2) {
            zfree(server.auth_domain_check);
            server.auth_domain_check = zstrdup(argv[1]);
        } else if (!strcasecmp(argv[0], "ssl_certificate") && argc == 2) {
            zfree(server.ssl_certificate);
            server.ssl_certificate = zstrdup(argv[1]);
        } else if (!strcasecmp(argv[0], "ssl_ca_file") && argc == 2) {
            zfree(server.ssl_ca_file);
            server.ssl_ca_file = zstrdup(argv[1]);
        } else if (!strcasecmp(argv[0], "ssl_protocol_version") && argc == 2) {
            zfree(server.ssl_protocol_version);
            server.ssl_protocol_version = zstrdup(argv[1]);
        } else if (!strcasecmp(argv[0], "ssl_cipher_list") && argc == 2) {
            zfree(server.ssl_cipher_list);
            server.ssl_cipher_list = zstrdup(argv[1]);
        } else {
            err = "Bad directive or wrong number of arguments"; 
            goto loaderr;
        }
        sdsfreesplitres(argv, argc);
    }

    sdsfreesplitres(lines, totlines);
    return;

loaderr:
    fprintf(stderr, "\n*** FATAL CONFIG FILE ERROR ***\n");
    fprintf(stderr, "Reading the configuration file, at line %d\n", linenum);
    fprintf(stderr, ">>> '%s'\n", lines[i]);
    fprintf(stderr, "%s\n", err);
    exit(1);
}
/* Load the server configuration from the specified filename.
 * The function appends the additional configuration directives stored
 * in the 'options' string to the config file before loading.
 *
 * Both filename and options can be NULL, in such a case are considered
 * empty. This way loadServerConfig can be used to just load a file or
 * just load a string. */
void loadServerConfig(char *filename) {
    sds config = sdsempty();
    char buf[CONFIG_MAX_LINE+1];

    /* Load the file content */
    if (filename) {
        FILE *fp;

        if ((fp = fopen(filename, "r")) == NULL) {
            log_error("Fatal error, can't open config file '%s'", filename);
            exit(1);
        }

        while (fgets(buf, CONFIG_MAX_LINE+1, fp) != NULL)
            config = sdscat(config, buf);
    }
    loadServerConfigFromString(config);
    sdsfree(config);
}