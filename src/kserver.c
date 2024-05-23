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
#include <dirent.h>

#include "kserver.h"

#define MAXLEN          1024
#define HTTP_OK         200
#define HTTP_NOFOUND    404
#define HTTP_ROOT       "./api"
#define HTTP_PORT       "8099"

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
static void send_directory_listing(struct mg_connection *conn, const char *dir);

/**************************API FUNCTION******************************/

struct ApiEntry ApiTable[] = {
    {"/userregister", "POST", kx_user_register},
    {"/userget", "POST", kx_user_get},
    {"/fileset", "POST", kx_file_set},
    {"/fileget", "POST", kx_file_get},
    {"/filegetall", "POST", kx_file_getall}
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

/**********************************DOCAPI***************************************/

static void send_directory_listing(struct mg_connection *conn, const char *dir) {
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL) {
        mg_printf(conn, "HTTP/1.1 500 Internal Server Error\r\n"
                        "Content-Type: text/plain\r\n\r\n"
                        "Cannot open directory");
        return;
    }

    mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n\r\n");

    mg_printf(conn, "<html><body><h1>Kserver API listing</h1><ul>");

    while ((entry = readdir(dp))) {
        char path[2048];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        struct stat st;
        stat(path, &st);

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;   // Skip "." and ".."
        }

        if (S_ISDIR(st.st_mode)) {
            mg_printf(conn, "<li><a href=\"%s/\">%s/</a></li>", entry->d_name, entry->d_name);
        } else {
            mg_printf(conn, "<li><a href=\"%s\">%s</a></li>", path, entry->d_name);
        }
    }

    mg_printf(conn, "</ul></body></html>");

    closedir(dp);
}

/*******************************************************************************/
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
    log_info("http info (%s)", message);
    return 1;
}

static void connection_close_cb(const struct mg_connection *conn) {
    const struct mg_request_info *ri = NULL;
    ri = mg_get_request_info(conn);
    log_info("(%s) connect close", ri->local_uri);
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
    int ret;
    char len_text[32];

    if ((ret = mg_response_header_start(conn, status)) != 0) {
        log_error("mg_response_header_start error (%d)", ret);
        goto err;
    }
        
    
    ret = mg_response_header_add(conn,
	                       "Content-Type",
	                       "application/json; charset=utf-8",
	                       -1);
    if (ret != 0) {
        log_error("mg_response_header_add error (%d)", ret);
        goto err;
    }

    sprintf(len_text, "%lu", len);
    if ((ret = mg_response_header_add(conn, "Content-Length", len_text, -1)) != 0) {
        log_error("mg_response_header_add error (%d)", ret);
        goto err;
    }
	if ((ret = mg_response_header_send(conn)) != 0) {
        log_error("mg_response_header_send error (%d)", ret);
        goto err;
    }
	
    if ((ret = mg_write(conn, buf, len)) <= 0) {
        if (ret == 0)
            log_error("mg_write the connection has been closed error (%d)", ret);
        if (ret == -1)
            log_error("mg_write on error (%d)", ret);
        goto err;
    }

    return status;
err:
    return -1;
}

/* mg_request_handler

   Called when a new request comes in.  This callback is URI based
   and configured with mg_set_request_handler().

   Parameters:
      conn: current connection information.
      cbdata: the callback data configured with mg_set_request_handler().
   Returns:
      0: the handler could not handle the request, so fall through.
      1 - 999: the handler processed the request. The return code is
               stored as a HTTP status code for the access log. */
static int
request_handler(struct mg_connection *conn, void *cbdata) {
	int status;
    sds strret = NULL;
	char response[MAXLEN] = {0};
    char buf[MAXLEN] = {0};
    struct ApiEntry *api = NULL;
    const struct mg_request_info *ri = NULL;
    size_t uri_len;
    size_t content_len;
    
    /* Get the URI from the request info. */
    ri = mg_get_request_info(conn);
    uri_len = strlen(ri->local_uri);

    if (uri_len <= 100) {
        status = HTTP_OK; /* 200 = OK */

        api = getApiFunc(ri->local_uri, ri->request_method);
        if (api && ri->content_length > 0) {
            mg_read(conn, buf, sizeof(buf));
            strret = api->jfunc(buf, strlen(buf));
            sprintf(response, "%s", strret);

            /* The return data must be released here, 
             * otherwise a memory leak will occur */
            sdsfree(strret);
        } else {
            status = HTTP_NOFOUND;
            sprintf(response, "%s", STRFAIL);
        }
    } else {
        status = HTTP_NOFOUND; /* 404 = Not Found */
        /* We don't like this URL */
		sprintf(response, "%s", STRFAIL);
    }
    content_len = strlen(response);
    
    /* Returns:
     * 0: the handler could not handle the request, so fall through.
     * 1 - 999: the handler processed the request. The return code is
     * stored as a HTTP status code for the access log. */
	if (ksresponse(conn, response, content_len, status) != -1)
        return status;
    return 0;
}

static int apidoc_handle_request(struct mg_connection *conn, void *cbdata) {
    char path[1024];
    struct stat st;

    const struct mg_request_info *request_info = mg_get_request_info(conn);
    snprintf(path, sizeof(path), "%s", request_info->local_uri);
    log_info("(%s) api request.", path);

    if (stat(path+1, &st) == 0 && S_ISDIR(st.st_mode)) {
        send_directory_listing(conn, path+1);
        return 1; // Mark as processed
    } else {
        // Serve file or return 404
        mg_send_file(conn, path+1);
        return 1; // Mark as processed
    }
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

    const char *options[] = {
        "document_root", HTTP_ROOT,
        "listening_ports", HTTP_PORT,
        NULL
    };

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
    server.init.configuration_options = options;
    server.ctx = NULL;
    server.redisip = "127.0.0.1";
    server.redisport = 6379;

    return;
err:
    exit(0);
}

static void startserver() {
    server.ctx = mg_start2(&server.init, &server.error);

    if (server.ctx && server.error.code == MG_ERROR_DATA_CODE_OK) {
        mg_set_request_handler(server.ctx, "/", request_handler, NULL);
        mg_set_request_handler(server.ctx, "/api", apidoc_handle_request, NULL);
    } else {
        log_error("Initialization failed, (%u) %s", server.error.code, server.error.text);
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
    free(server.system_info);
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