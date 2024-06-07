/*
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <event.h>
#include <hiredis/adapters/libevent.h>
#include "worker.h"
#include "log.h"

struct worker *worker_new(struct Server *s) {
    int ret;
	struct worker *w = calloc(1, sizeof(struct worker));
	w->s = s;

	/* setup communication link */
	ret = pipe(w->link);
	(void)ret;

	/* Redis connection pool */
	w->pool = pool_new(w, 5);

	return w;
}

void worker_can_read(int fd, short event, void *p) {
    // struct http_client *c = p;
}

/**
 * Monitor client FD for possible reads.
 */
void worker_monitor_input(struct http_client *c) {
	// event_set(&c->ev, c->fd, EV_READ, worker_can_read, c);
	// event_base_set(c->w->base, &c->ev);
	// event_add(&c->ev, NULL);
}

/**
 * Called when a client is sent to this worker.
 */
static void worker_on_new_client(int pipefd, short event, void *ptr) {
    struct http_client *c;
    unsigned long addr;

	(void)event;
	(void)ptr;

	/* Get client from messaging pipe */
	int ret = read(pipefd, &addr, sizeof(addr));
    if(ret == sizeof(addr)) {
		c = (struct http_client*)addr;
		log_info("pipe recv new http client");
		/* monitor client for input */
		// worker_monitor_input(c);
	}
}

static void worker_pool_connect(struct worker *w) {
	int i;
	/* create connections */
	for(i = 0; i < w->pool->count; ++i) {
		pool_connect(w->pool, 0, 1);
	}
}

static void* worker_main(void *p) {
    struct worker *w = p;
    struct event ev;

    w->base = event_base_new();

    /* monitor pipe link */
	event_set(&ev, w->link[0], EV_READ | EV_PERSIST, worker_on_new_client, w);
	event_base_set(w->base, &ev);
	event_add(&ev, NULL);

    /* connect to Redis */
	worker_pool_connect(w);

    /* loop */
	event_base_dispatch(w->base);
    return NULL;
}

void worker_start(struct worker *w) {
    pthread_create(&w->thread, NULL, worker_main, w);
}

/**
 * Queue new client to process
 */
void worker_add_client(struct worker *w, struct http_client *c) {
	/* write into pipe link */
	unsigned long addr = (unsigned long)c;
	int ret = write(w->link[1], &addr, sizeof(addr));
	(void)ret;
}