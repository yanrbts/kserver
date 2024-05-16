/*
 * Copyright 2024-2024 yanruibinghxu
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <locale.h>

#include "kserver.h"


struct Server server;

static char error_text[256] = {0};

static void initserver();
static void startserver();
static void stopserver();
static int ksresponse(struct mg_connection *conn, 
                        const void *buf,
                        size_t len,
                        int status);
static char *ksdup(const char *str);
static void init_system_info(void);

static int log_message_cb(const struct mg_connection *conn, const char *message);
static void connection_close_cb(const struct mg_connection *conn);
static int request_handler(struct mg_connection *conn, void *cbdata);

/**************************API FUNCTION******************************/

struct ApiEntry ApiTable[] = {
    {"/userregister", "POST", NULL, js_user_register_data},
    {"/userlogin", "POST", NULL, js_user_login_data}
};

static struct ApiEntry *getApiFunc(const char *uri, const char *method) {
    int i;
    int num = sizeof(ApiTable) / sizeof(struct ApiEntry);

    for (i = 0; i < num; i++) {
        struct ApiEntry *api = ApiTable + i;
        if (strcmp(uri, api->uri) == 0
            && strcmp(method, api->method) == 0)
            return api;
    }
    return NULL;
}

/*************************************************************************/

static char *ksdup(const char *str) {
	size_t len;
	char *p;

	len = strlen(str) + 1;
	p = (char *)malloc(len);

	if (p == NULL) {
		log_error("Cannot allocate %u bytes", (unsigned)len);
        exit(EXIT_FAILURE);
	}

	memcpy(p, str, len);
	return p;
}

static void init_system_info(void) {
	int len = mg_get_system_info(NULL, 0);
	if (len > 0) {
		server.system_info = (char *)malloc((unsigned)len + 1);
		(void)mg_get_system_info(server.system_info, len + 1);
	} else {
		server.system_info = ksdup("Not available");
	}
}

static int log_message_cb(const struct mg_connection *conn, const char *message) {
    log_info("[*] http info (%s)", message);
    return 1;
}

static void connection_close_cb(const struct mg_connection *conn) {
    const struct mg_request_info *ri = NULL;
    ri = mg_get_request_info(conn);
    log_info("[*] %s connect close", ri->local_uri);
}

/* Server responds to client
 * conn : Created link object
 * buf : Information sent to the client
 * len : info length
 * status : status code*/
static int ksresponse(struct mg_connection *conn, 
                        const void *buf,
                        size_t len,
                        int status)
{
    char len_text[32];

    mg_response_header_start(conn, status);
    mg_response_header_add(conn,
	                       "Content-Type",
	                       "application/json; charset=utf-8",
	                       -1);

    sprintf(len_text, "%lu", len);
    mg_response_header_add(conn, "Content-Length", len_text, -1);
	mg_response_header_send(conn);
	mg_write(conn, buf, len);

    return status;
}

static int
request_handler(struct mg_connection *conn, void *cbdata) {
    /* Generate a response text and status code. */
	int status;
	char response[1024];
    struct ApiEntry *api = NULL;
    const struct mg_request_info *ri = NULL;
    uint32_t uri_len;
    uint64_t content_len;
    char buf[1024] = {0};

    /* Get the URI from the request info. */
    ri = mg_get_request_info(conn);
    uri_len = (uint32_t)strlen(ri->local_uri);

    if (uri_len <= 100) {
        status = 200; /* 200 = OK */

        api = getApiFunc(ri->local_uri, ri->request_method);
        mg_read(conn, buf, sizeof(buf));
        // api->func(conn, api->jfunc);
        if (api) {
            api->jfunc(buf, strlen(buf));
            sprintf(response, "%s", "{\"action\":\"COMPLETE\", \"msg\":\"The upload has been completed.\"}");
        } else {
            sprintf(response, "%s", "{\"action\":\"FAILED\", \"msg\":\"FAILE.\"}");
        }
    } else {
        status = 404; /* 404 = Not Found */
        /* We don't like this URL */
		sprintf(response, "No such URL\n");
    }
    content_len = (uint64_t)strlen(response);
    
	ksresponse(conn, response, content_len, status);

	return status;
}

static void sigShutdownHandler(int sig) {
    char *msg;

    switch (sig) {
    case SIGINT:
        msg = "Received SIGINT scheduling shutdown...";
        break;
    case SIGTERM:
        msg = "Received SIGTERM scheduling shutdown...";
        break;
    default:
        msg = "Received shutdown signal, scheduling shutdown...";
    }
    printf("[*] %s\n", msg);

    stopserver();
    exit(1);
}

void setupSignalHandlers(void) {
    struct sigaction act;
    
    /* When the SA_SIGINFO flag is set in sa_flags then sa_sigaction is used.
     * Otherwise, sa_handler is used. */
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigShutdownHandler;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    return;
}

static void initserver() {
    int ret;

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    setupSignalHandlers();

    ret = mg_init_library(MG_FEATURES_TLS);
    if (ret != MG_FEATURES_TLS) {
        log_error("Initializing SSL libraries failed.");
        goto err;
    }
    memset(&server.callbacks, 0, sizeof(struct mg_callbacks));
    server.callbacks.log_message = log_message_cb;
    server.callbacks.connection_close = connection_close_cb;

    memset(&server.error, 0, sizeof(struct mg_error_data));
    memset(&server.init, 0, sizeof(struct mg_init_data));
    init_system_info();

    server.error.text = error_text;
    server.error.text_buffer_size = sizeof(error_text);

    server.init.callbacks = &server.callbacks;
    server.init.user_data = NULL;
    server.ctx = NULL;
    return;
err:
    exit(0);
}

static void startserver() {
    server.ctx = mg_start2(&server.init, &server.error);

    if (server.ctx) {
        mg_set_request_handler(server.ctx, "/user", request_handler, NULL);
    } else {
        log_error("Initialization failed: (%u), %s", server.error.code, server.error.text);
        goto err;
    }

    while (1) {
        sleep(1);
    }
err:
    exit(0);
}

static void stopserver() {
    if (server.ctx) 
        mg_stop(server.ctx);
    mg_exit_library();
}

int main(int argc, char *argv[]) {
    struct timeval tv;

    /* The setlocale() function is used to set or query the program's current locale.
     * 
     * The function is used to set the current locale of the program and the 
     * collation of the specified locale. Specifically, the LC_COLLATE parameter
     * represents the collation of the region. By setting it to an empty string,
     * the default locale collation is used.*/
    setlocale(LC_COLLATE, "");

    /* The  tzset()  function initializes the tzname variable from the TZ environment variable.  
     * This function is automati‐cally called by the other time conversion functions 
     * that depend on the timezone.*/
    tzset();
    srand(time(NULL)^getpid());
    gettimeofday(&tv,NULL);

    initserver();
    startserver();
    stopserver();
    return 0;
}