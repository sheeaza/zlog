#ifndef __wthread_h
#define __wthread_h

#include <pthread.h>

#include "list.h"

struct wthread_create_arg {
	int xxx;
};

struct wthread {
	pthread_t tid;
    struct list_head per_thread_data;
};

struct wthread *wthread_create(struct wthread_create_arg *arg);
void wthread_destroy(struct wthread *wthread);

#endif
