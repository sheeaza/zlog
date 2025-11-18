#ifndef __wthread_h
#define __wthread_h

#include <pthread.h>
#include <stdbool.h>

#include "list.h"
#include "zc_xplatform.h"

struct zlog_conf_s;
struct wthread_create_arg {
    struct zlog_conf_s *conf;
};

struct zlog_buf_s;
struct wthread {
	pthread_t tid;
    struct list_head per_thread_data;
	struct zlog_buf_s *msg_buf;
	char time_str[MAXLEN_CFG_LINE + 1];
    bool exit;
};

struct wthread *wthread_create(struct wthread_create_arg *arg);
void wthread_destroy(struct wthread *wthread);

#endif
