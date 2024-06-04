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
#include <openssl/ssl.h>

#include "kserver.h"



char *ascii_logo ="\n" 
"    __                                      | Version: %s\n"                       
"   / /__________  ______   _____  _____     | Port: %s \n"
"  / //_/ ___/ _ \\/ ___/ | / / _ \\/ ___/     | PID: %ld\n"
" / ,< (__  )  __/ /   | |/ /  __/ /         | Author: Yanruibing\n"   
"/_/|_/____/\\___/_/    |___/\\___/_/          | Web: http://www.kxyk.com \n\n";

struct Server server;

static char error_text[256] = {0};

static void showWebOption(void);
static void initServer();
static void startServer();
static void stopServer();
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

/**************************CERT**************************************/

/* This array corresponds to the content of the file
 * /resources/cert/server.crt converted from BASE64 to HEX.
 * When using this code, you need to REPLACE IT BY YOUR OWN CERTIFICATE!
 */
const uint8_t SSL_CERT_ASN1[] = {
    0x30, 0x82, 0x04, 0x40, 0x30, 0x82, 0x03, 0x28, 0xa0, 0x03, 0x02, 0x01,
    0x02, 0x02, 0x14, 0x49, 0x65, 0x5b, 0x35, 0xce, 0x42, 0x20, 0x15, 0xa7,
    0xc4, 0x8a, 0x29, 0xe6, 0x44, 0x58, 0x6c, 0x11, 0xe6, 0x8c, 0xa7, 0x30,
    0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b,
    0x05, 0x00, 0x30, 0x71, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04,
    0x06, 0x13, 0x02, 0x41, 0x41, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55,
    0x04, 0x08, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73,
    0x74, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0c, 0x09,
    0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x31, 0x12, 0x30,
    0x10, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61,
    0x6c, 0x68, 0x6f, 0x73, 0x74, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55,
    0x04, 0x0b, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73,
    0x74, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x09,
    0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x30, 0x1e, 0x17,
    0x0d, 0x32, 0x31, 0x30, 0x34, 0x30, 0x34, 0x31, 0x38, 0x33, 0x39, 0x35,
    0x34, 0x5a, 0x17, 0x0d, 0x33, 0x31, 0x30, 0x34, 0x30, 0x32, 0x31, 0x38,
    0x33, 0x39, 0x35, 0x34, 0x5a, 0x30, 0x71, 0x31, 0x0b, 0x30, 0x09, 0x06,
    0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x41, 0x41, 0x31, 0x12, 0x30, 0x10,
    0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c,
    0x68, 0x6f, 0x73, 0x74, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04,
    0x07, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74,
    0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x09, 0x6c,
    0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x31, 0x12, 0x30, 0x10,
    0x06, 0x03, 0x55, 0x04, 0x0b, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c,
    0x68, 0x6f, 0x73, 0x74, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04,
    0x03, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74,
    0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
    0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
    0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xca, 0x25, 0x51,
    0xe7, 0xb4, 0x06, 0x2d, 0x11, 0xe1, 0x33, 0xfb, 0xde, 0xe1, 0x92, 0xbe,
    0x63, 0x29, 0xf9, 0x1a, 0xea, 0xcd, 0x2d, 0x71, 0xf8, 0xc9, 0xae, 0x2b,
    0x35, 0xae, 0x6f, 0xd1, 0x71, 0xae, 0xf5, 0x6f, 0xcc, 0xdd, 0xcf, 0x25,
    0xd1, 0x59, 0x78, 0x30, 0x82, 0xec, 0x65, 0xe3, 0xd7, 0xa2, 0x6f, 0x7c,
    0x84, 0x82, 0xec, 0x2f, 0xd4, 0x37, 0x7d, 0xbc, 0x6c, 0x36, 0x07, 0x82,
    0x24, 0xed, 0x21, 0xe5, 0x81, 0xc3, 0x12, 0x2b, 0x54, 0x96, 0x44, 0xf7,
    0xdd, 0x6a, 0x58, 0xe1, 0x2f, 0x3f, 0xcd, 0x67, 0x1e, 0x32, 0xc8, 0x99,
    0x9b, 0xda, 0x33, 0x54, 0xdb, 0x52, 0x5d, 0xb2, 0xc5, 0x80, 0x50, 0x3b,
    0x41, 0xe1, 0xd3, 0xa8, 0x0c, 0x0d, 0x23, 0x90, 0xe4, 0x62, 0xf4, 0x4f,
    0xd6, 0x1b, 0xc5, 0x5a, 0xe0, 0x18, 0xa9, 0x3d, 0x74, 0xbb, 0x30, 0x20,
    0xbd, 0x85, 0xbd, 0xc3, 0xbd, 0x69, 0x86, 0x62, 0x75, 0xa6, 0x9e, 0x1c,
    0x74, 0x45, 0xa2, 0x5e, 0x67, 0x4c, 0xe0, 0xa2, 0x87, 0x13, 0x28, 0x8c,
    0x76, 0xbf, 0x2e, 0xdf, 0xe7, 0xaf, 0x79, 0x12, 0xa9, 0x65, 0x42, 0x66,
    0x1c, 0x25, 0xee, 0xca, 0x45, 0x29, 0x30, 0xff, 0x9c, 0x26, 0x14, 0x06,
    0x63, 0x91, 0x08, 0xa4, 0x58, 0x8d, 0x22, 0x0b, 0xc8, 0x18, 0x8e, 0x42,
    0xcd, 0x16, 0xb5, 0xdf, 0xa8, 0x7b, 0x7a, 0xb4, 0x71, 0x5f, 0xc2, 0xaa,
    0x60, 0x99, 0xc5, 0x2f, 0xef, 0x13, 0xd1, 0x9b, 0x9c, 0x0a, 0x8c, 0x14,
    0x7a, 0x8a, 0xb9, 0xdf, 0xdf, 0x9d, 0xaa, 0x76, 0x52, 0xff, 0x0c, 0x93,
    0x68, 0x3f, 0x73, 0x0d, 0xb7, 0xc0, 0x21, 0x67, 0x48, 0xfd, 0xf2, 0xe2,
    0xc3, 0x9c, 0xf5, 0xd4, 0x89, 0x86, 0xa9, 0x7b, 0x6a, 0x1b, 0x81, 0xa0,
    0x89, 0x39, 0x71, 0xc9, 0x3a, 0x65, 0x54, 0xdb, 0x06, 0x22, 0x82, 0x7f,
    0xc7, 0x02, 0x03, 0x01, 0x00, 0x01, 0xa3, 0x81, 0xcf, 0x30, 0x81, 0xcc,
    0x30, 0x81, 0x98, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x81, 0x90, 0x30,
    0x81, 0x8d, 0xa1, 0x75, 0xa4, 0x73, 0x30, 0x71, 0x31, 0x0b, 0x30, 0x09,
    0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x41, 0x41, 0x31, 0x12, 0x30,
    0x10, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61,
    0x6c, 0x68, 0x6f, 0x73, 0x74, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55,
    0x04, 0x07, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73,
    0x74, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x09,
    0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x31, 0x12, 0x30,
    0x10, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61,
    0x6c, 0x68, 0x6f, 0x73, 0x74, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55,
    0x04, 0x03, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73,
    0x74, 0x82, 0x14, 0x60, 0x3a, 0x75, 0xa0, 0xfc, 0x63, 0x14, 0xe1, 0x70,
    0x07, 0x16, 0x23, 0x7b, 0x84, 0x63, 0x3d, 0xe1, 0x1d, 0x07, 0x12, 0x30,
    0x0c, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x02, 0x30,
    0x00, 0x30, 0x0b, 0x06, 0x03, 0x55, 0x1d, 0x0f, 0x04, 0x04, 0x03, 0x02,
    0x04, 0xf0, 0x30, 0x14, 0x06, 0x03, 0x55, 0x1d, 0x11, 0x04, 0x0d, 0x30,
    0x0b, 0x82, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74,
    0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
    0x0b, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x13, 0x3c, 0x41, 0x03,
    0xb2, 0x58, 0x00, 0x0a, 0x73, 0x1f, 0xb4, 0x15, 0xc7, 0x5f, 0xa5, 0x07,
    0x14, 0xa6, 0xee, 0x7c, 0x83, 0x40, 0x85, 0x70, 0x40, 0x6c, 0x08, 0x94,
    0x71, 0x77, 0x1c, 0xd8, 0x9a, 0xb3, 0x8d, 0x7c, 0xfa, 0x45, 0x96, 0x03,
    0x51, 0x92, 0x46, 0xf8, 0x84, 0x6d, 0x09, 0x40, 0x3a, 0x53, 0x07, 0xc3,
    0xd6, 0x1f, 0x72, 0xe6, 0x15, 0xeb, 0x49, 0x3a, 0x63, 0x2d, 0x11, 0x58,
    0x73, 0xe3, 0x25, 0xde, 0xb0, 0x85, 0x5b, 0x45, 0xc6, 0xb9, 0x46, 0x2d,
    0x86, 0x2d, 0xc9, 0x9b, 0x1f, 0x73, 0x6a, 0x18, 0xe7, 0x94, 0x49, 0x4c,
    0x77, 0xea, 0x0b, 0xb6, 0xd1, 0xfb, 0x77, 0x6a, 0x1b, 0xfc, 0xb0, 0xcf,
    0x5e, 0x29, 0xf1, 0xd5, 0xfc, 0x93, 0x8f, 0x48, 0xe3, 0xe0, 0xbd, 0x09,
    0xa5, 0x7d, 0xa1, 0x50, 0x2c, 0x9f, 0xdc, 0x44, 0xe4, 0x85, 0xbb, 0xca,
    0xab, 0xbd, 0x73, 0xa7, 0xd8, 0x3e, 0x31, 0x53, 0xfe, 0xd4, 0xc3, 0xd7,
    0x05, 0x3c, 0x93, 0x72, 0xcc, 0x16, 0x2b, 0xe6, 0x33, 0xf0, 0xbd, 0xea,
    0x9a, 0x5f, 0x24, 0x39, 0x7e, 0xfe, 0x82, 0x34, 0xa6, 0xb2, 0x23, 0xe4,
    0x67, 0x8f, 0xf9, 0x60, 0x6b, 0xd7, 0x2e, 0xd9, 0xfb, 0x9a, 0x12, 0x3c,
    0xb5, 0x3b, 0xf9, 0x9d, 0x0a, 0xbf, 0xff, 0x2d, 0x6b, 0x33, 0x82, 0xe6,
    0x54, 0x75, 0x2b, 0xf2, 0x74, 0x3b, 0xc7, 0x96, 0xa5, 0x06, 0x70, 0xb6,
    0x0a, 0x43, 0x7a, 0x7b, 0xa1, 0xc3, 0x8e, 0x2a, 0x58, 0xcc, 0xff, 0x49,
    0xeb, 0x51, 0xb6, 0x7c, 0x7e, 0xa2, 0x68, 0x9f, 0x82, 0xe8, 0x2e, 0x4e,
    0xf9, 0x14, 0xc7, 0xbe, 0x60, 0x42, 0x65, 0x9d, 0x5a, 0xce, 0xca, 0x09,
    0xf7, 0xee, 0x48, 0x41, 0x93, 0x69, 0x7c, 0x3e, 0x47, 0xf0, 0x5e, 0x60,
    0x5d, 0x1d, 0xa3, 0x67, 0x59, 0x3c, 0xf9, 0x3c, 0x49, 0x63, 0x74, 0x84};


/* This array corresponds to the content of the file
 * /resources/cert/server.key converted from BASE64 to HEX.
 * When using this code, you need to REPLACE IT BY YOUR OWN PRIVATE KEY!
 */
const uint8_t SSL_KEY_ASN1[] = {
    0x30, 0x82, 0x04, 0xa2, 0x02, 0x01, 0x00, 0x02, 0x82, 0x01, 0x01, 0x00,
    0xca, 0x25, 0x51, 0xe7, 0xb4, 0x06, 0x2d, 0x11, 0xe1, 0x33, 0xfb, 0xde,
    0xe1, 0x92, 0xbe, 0x63, 0x29, 0xf9, 0x1a, 0xea, 0xcd, 0x2d, 0x71, 0xf8,
    0xc9, 0xae, 0x2b, 0x35, 0xae, 0x6f, 0xd1, 0x71, 0xae, 0xf5, 0x6f, 0xcc,
    0xdd, 0xcf, 0x25, 0xd1, 0x59, 0x78, 0x30, 0x82, 0xec, 0x65, 0xe3, 0xd7,
    0xa2, 0x6f, 0x7c, 0x84, 0x82, 0xec, 0x2f, 0xd4, 0x37, 0x7d, 0xbc, 0x6c,
    0x36, 0x07, 0x82, 0x24, 0xed, 0x21, 0xe5, 0x81, 0xc3, 0x12, 0x2b, 0x54,
    0x96, 0x44, 0xf7, 0xdd, 0x6a, 0x58, 0xe1, 0x2f, 0x3f, 0xcd, 0x67, 0x1e,
    0x32, 0xc8, 0x99, 0x9b, 0xda, 0x33, 0x54, 0xdb, 0x52, 0x5d, 0xb2, 0xc5,
    0x80, 0x50, 0x3b, 0x41, 0xe1, 0xd3, 0xa8, 0x0c, 0x0d, 0x23, 0x90, 0xe4,
    0x62, 0xf4, 0x4f, 0xd6, 0x1b, 0xc5, 0x5a, 0xe0, 0x18, 0xa9, 0x3d, 0x74,
    0xbb, 0x30, 0x20, 0xbd, 0x85, 0xbd, 0xc3, 0xbd, 0x69, 0x86, 0x62, 0x75,
    0xa6, 0x9e, 0x1c, 0x74, 0x45, 0xa2, 0x5e, 0x67, 0x4c, 0xe0, 0xa2, 0x87,
    0x13, 0x28, 0x8c, 0x76, 0xbf, 0x2e, 0xdf, 0xe7, 0xaf, 0x79, 0x12, 0xa9,
    0x65, 0x42, 0x66, 0x1c, 0x25, 0xee, 0xca, 0x45, 0x29, 0x30, 0xff, 0x9c,
    0x26, 0x14, 0x06, 0x63, 0x91, 0x08, 0xa4, 0x58, 0x8d, 0x22, 0x0b, 0xc8,
    0x18, 0x8e, 0x42, 0xcd, 0x16, 0xb5, 0xdf, 0xa8, 0x7b, 0x7a, 0xb4, 0x71,
    0x5f, 0xc2, 0xaa, 0x60, 0x99, 0xc5, 0x2f, 0xef, 0x13, 0xd1, 0x9b, 0x9c,
    0x0a, 0x8c, 0x14, 0x7a, 0x8a, 0xb9, 0xdf, 0xdf, 0x9d, 0xaa, 0x76, 0x52,
    0xff, 0x0c, 0x93, 0x68, 0x3f, 0x73, 0x0d, 0xb7, 0xc0, 0x21, 0x67, 0x48,
    0xfd, 0xf2, 0xe2, 0xc3, 0x9c, 0xf5, 0xd4, 0x89, 0x86, 0xa9, 0x7b, 0x6a,
    0x1b, 0x81, 0xa0, 0x89, 0x39, 0x71, 0xc9, 0x3a, 0x65, 0x54, 0xdb, 0x06,
    0x22, 0x82, 0x7f, 0xc7, 0x02, 0x03, 0x01, 0x00, 0x01, 0x02, 0x82, 0x01,
    0x00, 0x73, 0xbb, 0x54, 0x1e, 0x34, 0xca, 0x48, 0x69, 0x71, 0x26, 0xc2,
    0xf0, 0x02, 0xf3, 0x71, 0xbe, 0xf2, 0x5b, 0xe5, 0x16, 0x42, 0xeb, 0xde,
    0xd1, 0x92, 0x1d, 0xfe, 0x2d, 0x18, 0xb6, 0x7a, 0x11, 0xfd, 0x1a, 0x15,
    0xad, 0x13, 0xdc, 0xb2, 0x09, 0x1e, 0x91, 0x1a, 0x2d, 0x0a, 0xcc, 0xf6,
    0xda, 0x10, 0xec, 0x85, 0x3c, 0x94, 0x7c, 0x46, 0x91, 0xd8, 0x47, 0x4b,
    0x66, 0x24, 0xb4, 0xbd, 0xc5, 0x08, 0x62, 0x9c, 0xb4, 0x63, 0x0b, 0x76,
    0xf5, 0x51, 0xa7, 0x20, 0xc5, 0x8a, 0x4a, 0x62, 0x7a, 0x1b, 0xac, 0x2c,
    0x7a, 0x74, 0x96, 0xb6, 0xa3, 0x2d, 0x14, 0xb0, 0x63, 0x74, 0xcf, 0xa2,
    0x37, 0x42, 0xd4, 0x2c, 0x68, 0xf6, 0xb2, 0xa8, 0x06, 0x66, 0x4b, 0x53,
    0x7b, 0xfe, 0x4f, 0x63, 0x99, 0xf0, 0x82, 0x58, 0x19, 0xee, 0xe4, 0x8e,
    0x03, 0xd3, 0xdb, 0xa5, 0x12, 0xfc, 0x8b, 0xfd, 0x90, 0xe2, 0x55, 0xd8,
    0xb4, 0x59, 0xc8, 0x75, 0xab, 0x14, 0x02, 0x65, 0xee, 0x00, 0xf0, 0xdc,
    0xa4, 0x5f, 0x1e, 0xbb, 0x10, 0x90, 0x57, 0xea, 0x45, 0x33, 0x75, 0xfd,
    0x0c, 0x97, 0xe6, 0xd6, 0xe6, 0xaa, 0x08, 0x22, 0x26, 0x4a, 0x34, 0x0d,
    0xfe, 0xb5, 0xde, 0xdb, 0xa8, 0xc0, 0x67, 0x83, 0x5f, 0x27, 0x62, 0x55,
    0x5a, 0xa0, 0x83, 0x25, 0xe6, 0x19, 0xd9, 0x78, 0x33, 0x7b, 0x2b, 0xa0,
    0x53, 0x1d, 0xed, 0x62, 0xd1, 0x10, 0x95, 0x9f, 0xb0, 0xa2, 0xe7, 0xe8,
    0x58, 0x39, 0x31, 0x76, 0x65, 0x53, 0x28, 0x23, 0x98, 0xa9, 0xab, 0xc2,
    0x9c, 0x57, 0x0e, 0x9c, 0x17, 0x10, 0x14, 0x35, 0x5b, 0x5d, 0xce, 0x94,
    0x1b, 0xdf, 0x8b, 0x42, 0xc0, 0xc6, 0x0e, 0x5c, 0x48, 0x67, 0x4f, 0xaf,
    0x27, 0x3d, 0xc4, 0xda, 0xfa, 0xb4, 0xbd, 0x8e, 0x55, 0xdd, 0xa8, 0x18,
    0x34, 0x01, 0xb6, 0xd8, 0x09, 0x02, 0x81, 0x81, 0x00, 0xe9, 0x53, 0x5f,
    0xf9, 0x0e, 0xdf, 0x1e, 0x18, 0x90, 0xcb, 0xb8, 0x66, 0xea, 0x70, 0x6a,
    0x72, 0xc3, 0x6e, 0x87, 0x7a, 0x79, 0x2f, 0xc4, 0x50, 0x32, 0x32, 0xcf,
    0x97, 0xa7, 0x3b, 0x2b, 0x90, 0xd1, 0x05, 0x39, 0x5f, 0x51, 0x47, 0x79,
    0xc3, 0x1d, 0xd8, 0xaa, 0xca, 0xda, 0x1a, 0x38, 0x6b, 0xfa, 0x02, 0x29,
    0xbd, 0x43, 0x45, 0xed, 0xe8, 0xae, 0xf5, 0xc0, 0xaa, 0xde, 0xe8, 0xac,
    0x21, 0xf5, 0x62, 0x99, 0xea, 0xeb, 0xb2, 0x54, 0xfd, 0xf3, 0x6a, 0x9d,
    0x13, 0xbe, 0x09, 0x51, 0xef, 0x0f, 0x12, 0xb2, 0x14, 0x20, 0xe9, 0x6d,
    0xfe, 0x6c, 0x63, 0x02, 0x7c, 0xd7, 0x0e, 0xa9, 0x2f, 0x2b, 0x52, 0x68,
    0x83, 0x50, 0xdd, 0xc2, 0xf1, 0x86, 0x7c, 0x33, 0xe8, 0x62, 0x6e, 0x8e,
    0x48, 0x50, 0x5c, 0x84, 0x7e, 0x22, 0x36, 0x60, 0x74, 0x16, 0x27, 0xd5,
    0x77, 0xb6, 0x94, 0x7e, 0x75, 0x02, 0x81, 0x81, 0x00, 0xdd, 0xca, 0x42,
    0x1f, 0x3d, 0x3f, 0xc7, 0x4e, 0xce, 0x7d, 0x37, 0x09, 0xef, 0xf8, 0xee,
    0x67, 0x97, 0xbb, 0xf8, 0x34, 0x49, 0x44, 0xa9, 0x9a, 0x07, 0x7f, 0x48,
    0xaa, 0xb9, 0x77, 0xb6, 0x22, 0xfd, 0x88, 0x97, 0x77, 0x20, 0x6e, 0x0c,
    0x67, 0x19, 0x2e, 0xc9, 0x58, 0x3c, 0xfd, 0xdb, 0x3b, 0xfb, 0x0b, 0xfb,
    0x86, 0xa2, 0x74, 0x31, 0x60, 0xaa, 0x27, 0x41, 0x3d, 0xdf, 0x9a, 0xaa,
    0xb3, 0xd8, 0x9a, 0x0a, 0x2d, 0xf9, 0xd7, 0xee, 0x67, 0xdc, 0x49, 0x40,
    0x74, 0x30, 0x32, 0xb7, 0x94, 0xfd, 0x84, 0x13, 0xb8, 0x24, 0x89, 0xdf,
    0xee, 0x7d, 0xe3, 0x1b, 0xe5, 0x76, 0xc4, 0x1b, 0x81, 0x32, 0xa6, 0x0f,
    0x07, 0x26, 0x87, 0x3b, 0xff, 0xaf, 0xa9, 0x25, 0x71, 0xd0, 0x70, 0x2e,
    0xa8, 0xbc, 0x7e, 0xe2, 0xe2, 0x6f, 0x71, 0x5e, 0xe2, 0xad, 0xc1, 0x22,
    0x0c, 0x3f, 0xc4, 0x35, 0xcb, 0x02, 0x81, 0x80, 0x46, 0x40, 0x08, 0x21,
    0x60, 0xcc, 0xe4, 0xae, 0xd8, 0xc9, 0xbd, 0x97, 0x9e, 0xf6, 0x81, 0xd6,
    0x53, 0xe9, 0x2f, 0x79, 0x3c, 0x8b, 0x99, 0x3b, 0xdc, 0x21, 0x58, 0x47,
    0x7c, 0xde, 0x5f, 0xdb, 0x96, 0x53, 0x50, 0x56, 0xd6, 0x8e, 0x02, 0xa7,
    0x30, 0x91, 0x4f, 0xbb, 0x0b, 0xb7, 0xe1, 0x4d, 0x01, 0x55, 0x2d, 0x64,
    0x02, 0xa1, 0x47, 0x64, 0x4b, 0x69, 0x4a, 0xbd, 0x27, 0xa8, 0x3e, 0x4b,
    0x6b, 0x2a, 0x68, 0xd5, 0x46, 0x69, 0xc7, 0x15, 0x3e, 0xf8, 0xd6, 0x9a,
    0x5f, 0x19, 0x47, 0x46, 0x06, 0xef, 0xc6, 0x16, 0x31, 0x62, 0x96, 0xef,
    0x87, 0x8a, 0xb7, 0xf1, 0x06, 0x7f, 0x2f, 0x89, 0x38, 0x2d, 0xf3, 0xb1,
    0xb5, 0xe3, 0x4f, 0x12, 0x91, 0x3f, 0x4c, 0x11, 0xa7, 0xb1, 0x49, 0xbd,
    0x94, 0x14, 0x86, 0xff, 0xc3, 0x25, 0x44, 0x1d, 0x2f, 0x9e, 0x86, 0xb3,
    0x28, 0x91, 0xc5, 0x11, 0x02, 0x81, 0x80, 0x57, 0x5c, 0xef, 0x54, 0xcc,
    0xd4, 0x8d, 0x96, 0x9e, 0x41, 0xb6, 0x67, 0x64, 0xae, 0x62, 0x82, 0x4d,
    0xc3, 0x8e, 0x0e, 0x52, 0x7a, 0x08, 0x70, 0x92, 0xd9, 0x71, 0x6f, 0x46,
    0x65, 0x40, 0x4a, 0x62, 0x21, 0xe6, 0xbf, 0xd6, 0xf7, 0x62, 0x4d, 0x4e,
    0x1f, 0x1e, 0xd2, 0x72, 0x1b, 0xf0, 0xba, 0x9c, 0xb5, 0xe8, 0x9a, 0xec,
    0xec, 0xe5, 0xf2, 0x54, 0xb3, 0xe7, 0xc0, 0x0e, 0x8f, 0x27, 0x04, 0x76,
    0xa2, 0x9e, 0xb5, 0xe3, 0x7f, 0x49, 0xfa, 0x81, 0x4c, 0x1d, 0x66, 0x67,
    0x01, 0xe3, 0x4c, 0x7d, 0xdc, 0x03, 0xc4, 0x7a, 0x28, 0x11, 0x1c, 0x29,
    0x5c, 0x47, 0x45, 0xc8, 0xd5, 0x90, 0x9c, 0x00, 0xae, 0x66, 0xa7, 0x03,
    0x67, 0x2b, 0x9c, 0x18, 0xbe, 0x80, 0xf0, 0x67, 0x11, 0x79, 0x5f, 0x9f,
    0xf8, 0x3f, 0x38, 0xc0, 0x7b, 0x20, 0xcc, 0x1b, 0x73, 0x43, 0x0d, 0x1e,
    0x25, 0x14, 0xa7, 0x02, 0x81, 0x80, 0x09, 0xe0, 0xa5, 0xb9, 0x56, 0x96,
    0x64, 0xf7, 0xd3, 0xc5, 0xbc, 0x19, 0x2b, 0x20, 0xcb, 0x08, 0x9e, 0x0b,
    0x3e, 0x5e, 0xcc, 0x8e, 0xf2, 0x72, 0x83, 0x9b, 0x9f, 0x52, 0xf4, 0x0a,
    0xda, 0xe0, 0x0b, 0x91, 0x14, 0x51, 0x8e, 0x19, 0x1b, 0x77, 0x7d, 0x2a,
    0xec, 0x9a, 0xdc, 0xd1, 0x83, 0xff, 0x25, 0x75, 0xb6, 0xb7, 0xe4, 0x51,
    0xb0, 0xa1, 0x22, 0x7b, 0x1f, 0xb6, 0xcd, 0x7d, 0xe0, 0x55, 0x2f, 0x3d,
    0xe4, 0x1e, 0xe9, 0x4e, 0x77, 0x2e, 0xe5, 0x8c, 0xcb, 0x82, 0x5e, 0xee,
    0xe7, 0x4e, 0x08, 0x09, 0x67, 0xb1, 0xcc, 0x67, 0x66, 0x68, 0xd1, 0x2c,
    0x65, 0x08, 0xc7, 0x8a, 0x23, 0xc4, 0x5b, 0x7c, 0x9c, 0x54, 0x44, 0x2e,
    0xe1, 0xad, 0x8f, 0x99, 0x2a, 0xd8, 0x8f, 0x31, 0x05, 0x7c, 0x9f, 0xbc,
    0x79, 0x67, 0x40, 0x5e, 0xda, 0x2c, 0x38, 0x7d, 0x3f, 0x0b, 0x6c, 0x83,
    0xcc, 0x75};


/* This is an example for a custom SSL initialization function.
 * It will setup the certificate and private key defined in the arrays above.
 * Thus, they do not need to be located in files in the file system.
 * To slightly improve security, you could use some encryption for the arrays
 * above, and only decrypt it in memory before passing to the SSL_CTX_use_*
 * functions. Then the data will not exist in the executable stored at the
 * disk, but only in memory at runtime - making reverse engineering more
 * complex. Here we use unencrypted versions, so you could test the conversion
 * process from server.key/server.crt to these arrays on your own, and see if
 * you come to the same result.
 */
static int
init_ssl(void *ssl_ctx, void *user_data)
{
	SSL_CTX *ctx = (SSL_CTX *)ssl_ctx;

	SSL_CTX_use_certificate_ASN1(ctx, sizeof(SSL_CERT_ASN1), SSL_CERT_ASN1);
	SSL_CTX_use_PrivateKey_ASN1(EVP_PKEY_RSA,
	                            ctx,
	                            SSL_KEY_ASN1,
	                            sizeof(SSL_KEY_ASN1));
	if (SSL_CTX_check_private_key(ctx) == 0) {
		log_error("SSL data inconsistency detected\n");
		return -1;
	}

	return 0; /* let CivetWeb set up the rest of OpenSSL */
}

/**************************API FUNCTION******************************/

struct ApiEntry ApiTable[] = {
    {"/userregister", "POST", kx_user_register},
    // {"/userget", "POST", kx_user_get},
    {"/fileset", "POST", kx_file_set},
    {"/fileget", "POST", kx_file_get},
    {"/filegetall", "POST", kx_file_getall},
    {"/filesettrace", "POST", kx_trace_set},
    {"/filegettrace", "POST", kx_trace_get}
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
    if (ri && ri->local_uri)
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
    sds response = NULL;
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

            /* The return data must be released here, 
             * otherwise a memory leak will occur */
            response = api->jfunc(buf, strlen(buf));
        } else {
            status = HTTP_NOFOUND;
            response = sdsnew(STRFAIL);
        }
    } else {
        status = HTTP_NOFOUND; /* 404 = Not Found */
        /* We don't like this URL */
        response = sdsnew(STRFAIL);
    }
    content_len = sdslen(response);
    
    /* Returns:
     * 0: the handler could not handle the request, so fall through.
     * 1 - 999: the handler processed the request. The return code is
     * stored as a HTTP status code for the access log. */
	if (ksresponse(conn, response, content_len, status) != -1) {
        sdsfree(response);
        return status;
    }
    sdsfree(response);
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

    stopServer();
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

static void initServerConfig(void) {
    server.redisip = zstrdup(CONFIG_REDIS_IP);
    server.redisport = CONFIG_REDIS_PORT;
    server.pagenum = REDIS_PAGENUM;
    server.httpport = zstrdup(HTTP_PORT);
    server.request_timeout = zstrdup(HTTP_REQUEST_MS);
    server.daemonize = 0;
    server.pidfile = NULL;
    server.logfile = zstrdup(CONFIG_DEFAULT_LOGFILE);
    server.auth_domain = zstrdup(CONFIG_CIVET_AUTH_DOMAIN);
    server.auth_domain_check = zstrdup(CONFIG_CIVET_DOMAIN_CHECK);
    server.ssl_no = CONFIG_CIVET_SSL_NO;
    server.ssl_certificate = zstrdup(CONFIG_CIVET_CERT);
    server.ssl_ca_file = zstrdup(CONFIG_CIVET_CA);
    server.ssl_protocol_version = zstrdup(CONFIG_CIVET_SSLPROTOVOL);
    server.ssl_cipher_list = zstrdup(CONFIG_CIVET_SSLCIPHER);
    server.logfp = NULL;

    server.num_threads = zstrdup(CONFIG_CIVET_THREADS_NUM);
    server.prespawn_threads = zstrdup(CONFIG_CIVET_THREADS_PRESPAWN);
    server.listen_backlog = zstrdup(CONFIG_CIVET_LISTEN_BACKLOG);
    server.connection_queue = zstrdup(CONFIG_CIVET_CONN_QUEUE);
}

static void initServer() {
    int ret;
    /*const char **options;
    
    const char *ssl_options[] = {
        "document_root", HTTP_ROOT,
        "listening_ports", server.httpport,
        "request_timeout_ms", server.request_timeout,
        "authentication_domain", server.auth_domain,
        "enable_auth_domain_check", server.auth_domain_check,
        "ssl_certificate", server.ssl_certificate,
        // "ssl_certificate_chain", "/home/yrb/src/kserver/cert/rootCA.pem",
        "ssl_ca_file", server.ssl_ca_file,
		"ssl_protocol_version", server.ssl_protocol_version,
        "ssl_cipher_list", server.ssl_cipher_list,
        "num_threads", server.num_threads,
        "prespawn_threads", server.prespawn_threads,
        "listen_backlog", server.listen_backlog,
        "connection_queue", server.connection_queue,
        "error_log_file", "error.log",
        NULL,NULL
    };*/

    const char *sslno_options[] = {
        "document_root", HTTP_ROOT,
        "listening_ports", server.httpport,
        "request_timeout_ms", server.request_timeout,
        "num_threads", server.num_threads,
        "prespawn_threads", server.prespawn_threads,
        "listen_backlog", server.listen_backlog,
        "connection_queue", server.connection_queue,
        NULL
    };
    
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    setupSignalHandlers();

    ret = mg_init_library(MG_FEATURES_TLS);
    if (ret != MG_FEATURES_TLS) {
        log_error("Initializing SSL libraries failed. (%u %s)", server.error.code, server.error.text);
        goto err;
    }

    if (!mg_check_feature(2)) {
		log_error("built with SSL support, but civetweb library build without.");
        goto err;
	}

    memset(&server.callbacks, 0, sizeof(struct mg_callbacks));
    server.callbacks.log_message = log_message_cb;
    server.callbacks.connection_close = connection_close_cb;
    // server.callbacks.init_ssl = init_ssl;

    memset(&server.error, 0, sizeof(struct mg_error_data));
    memset(&server.init, 0, sizeof(struct mg_init_data));
    memset(server.ports, 0, sizeof(server.ports));
    init_system_info();

    server.error.text = error_text;
    server.error.text_buffer_size = sizeof(error_text);

    server.init.callbacks = &server.callbacks;
    server.init.user_data = NULL;
    server.init.configuration_options = sslno_options;
    server.ctx = NULL;
    
    if (server.logfile[0] != '\0') {
        server.logfp = fopen(server.logfile, "a+");
        if (server.logfp) {
            log_add_fp(server.logfp, LOG_TRACE);
            log_set_quiet(1);
        } else {
            /* The log file failed to open, but it was output directly 
             * to standard output without exiting the program.*/
            log_error("Failed to open or create the %s log file.", server.logfile);
        }
    }

    return;
err:
    exit(0);
}

static void startServer() {
    int n;
    int port_cnt;

    server.ctx = mg_start2(&server.init, &server.error);
    if (server.ctx && server.error.code == MG_ERROR_DATA_CODE_OK) {
        mg_set_request_handler(server.ctx, "/", request_handler, NULL);
        mg_set_request_handler(server.ctx, "/api", apidoc_handle_request, NULL);
    } else {
        log_error("Initialization failed, (%u) %s", server.error.code, server.error.text);
        goto err;
    }

    showWebOption();

    port_cnt = mg_get_server_ports(server.ctx, 32, server.ports);
    for (n = 0; n < port_cnt && n < 32; n++) {
        const char *proto = server.ports[n].is_ssl ? "https" : "http";
        /* IPv4 */
        if ((server.ports[n].protocol & 1) == 1) {
            log_info("http protocol(%s), IPv4, port(%i)", proto, server.ports[n].port);
        }

        /* IPv6 */
        if ((server.ports[n].protocol & 2) == 2) {
            log_info("http protocol(%s), IPv6, port(%i)", proto, server.ports[n].port);
        }
    }

    while (1) {
        sleep(1);
    }
err:
    exit(0);
}

static void stopServer() {
    if (server.ctx) 
        mg_stop(server.ctx);
    if (server.configfile)
        sdsfree(server.configfile);
    if (server.redisip)
        zfree(server.redisip);
    if (server.httpport)
        zfree(server.httpport);
    if (server.request_timeout)
        zfree(server.request_timeout);
    if (server.pidfile)
        zfree(server.pidfile);
    if (server.logfile)
        zfree(server.logfile);

    if (server.auth_domain)
        zfree(server.auth_domain);
    if (server.auth_domain_check)
        zfree(server.auth_domain_check);
    if (server.ssl_certificate)
        zfree(server.ssl_certificate);
    if (server.ssl_ca_file)
        zfree(server.ssl_ca_file);
    if (server.ssl_protocol_version)
        zfree(server.ssl_protocol_version);
    if (server.ssl_cipher_list)
        zfree(server.ssl_cipher_list);
    if (server.logfp)
        fclose(server.logfp);
    
    if (server.num_threads)
        zfree(server.num_threads);
    if (server.prespawn_threads)
        zfree(server.prespawn_threads);
    if (server.listen_backlog)
        zfree(server.listen_backlog);
    if (server.connection_queue)
        zfree(server.connection_queue);

    free(server.system_info);
    mg_exit_library();
}
/*********************************EXPORT FUNCTION*******************************/

/* Return the UNIX time in microseconds */
long long ustime(void) {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust;
}

/********************************************************************************/
static void daemonize(void) {
    int fd;

    if (fork() != 0) exit(0); /* parent exits */
    setsid(); /* create a new session */

    /* Every output goes to /dev/null. If kserver is daemonized but
     * the 'logfile' is set to 'stdout' in the configuration file
     * it will not log at all. */
    if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) close(fd);
    }
}

static void createPidFile(void) {
    /* If pidfile requested, but no pidfile defined, use
     * default pidfile path */
    if (!server.pidfile) 
        server.pidfile = zstrdup(CONFIG_DEFAULT_PID_FILE);

    /* Try to write the pid file in a best-effort way. */
    FILE *fp = fopen(server.pidfile,"w");
    if (fp) {
        fprintf(fp,"%d\n", (int)getpid());
        fclose(fp);
    }
}

static void showWebOption(void) {
    // int i;
    const char *value;
    // const struct mg_option *options;

    // options = mg_get_valid_options();
    // for (i = 0; options[i].name != NULL; i++) {
    //     value = mg_get_option(server.ctx, options[i].name);
	// 		fprintf(stderr,
	// 		        "# %s %s\n",
	// 		        options[i].name,
	// 		        value ? value : "<value>");
    // }

    value = mg_get_option(server.ctx, "listening_ports");
    fprintf(stderr, "[ %-24s %40s ]\n", "listening_ports", value ? value : "<value>");
    value = mg_get_option(server.ctx, "document_root");
    fprintf(stderr, "[ %-24s %40s ]\n", "document_root", value ? value : "<value>");
    value = mg_get_option(server.ctx, "authentication_domain");
    fprintf(stderr, "[ %-24s %40s ]\n", "authentication_domain", value ? value : "<value>");
    value = mg_get_option(server.ctx, "num_threads");
    fprintf(stderr, "[ %-24s %40s ]\n", "num_threads", value ? value : "<value>");
    value = mg_get_option(server.ctx, "prespawn_threads");
    fprintf(stderr, "[ %-24s %40s ]\n", "prespawn_threads", value ? value : "<value>");
    value = mg_get_option(server.ctx, "listen_backlog");
    fprintf(stderr, "[ %-24s %40s ]\n", "listen_backlog", value ? value : "<value>");
    value = mg_get_option(server.ctx, "connection_queue");
    fprintf(stderr, "[ %-24s %40s ]\n", "connection_queue", value ? value : "<value>");
    value = mg_get_option(server.ctx, "ssl_protocol_version");
    fprintf(stderr, "[ %-24s %40s ]\n", "ssl_protocol_version", value ? value : "<value>");
    value = mg_get_option(server.ctx, "request_timeout_ms");
    fprintf(stderr, "[ %-24s %40s ]\n", "request_timeout_ms", value ? value : "<value>");
    value = mg_get_option(server.ctx, "keep_alive_timeout_ms");
    fprintf(stderr, "[ %-24s %40s ]\n", "keep_alive_timeout_ms", value ? value : "<value>");
    value = mg_get_option(server.ctx, "max_request_size");
    fprintf(stderr, "[ %-24s %40s ]\n", "max_request_size", value ? value : "<value>");
    value = mg_get_option(server.ctx, "ssl_certificate");
    fprintf(stderr, "[ %-24s %40s ]\n", "ssl_certificate", value ? value : "<value>");
    value = mg_get_option(server.ctx, "ssl_ca_file");
    fprintf(stderr, "[ %-24s %40s ]\n", "ssl_ca_file", value ? value : "<value>");
    
}

int main(int argc, char *argv[]) {
    struct timeval tv;
    char *configfile;

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

    /* The initialization here is mainly to determine later whether 
     * to use the value set in the configuration file or use the 
     * default value.*/
    initServerConfig();

    if (argc >= 2) {
        configfile = argv[1];
        server.configfile = getAbsolutePath(configfile);
        loadServerConfig(configfile);
    }

    if (server.daemonize)
        daemonize();
    
    printf(ascii_logo, KSERVER_VERSION, server.httpport, (long)getpid());

    if (argc == 1) {
        log_warn("no config file specified, using the default config. \nIn order to specify a config file use %s /path/to/%s.conf", argv[0], argv[0]+2);
    } else {
        log_info("Configuration loaded");
    }

    initServer();

    if (server.daemonize || server.pidfile)
        createPidFile();
    
    startServer();
    stopServer();
    return 0;
}