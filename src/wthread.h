#ifndef __wthread_h
#define __wthread_h

#include <pthread.h>

struct wthread_create_arg {
};

struct wthread {
	pthread_t tid;
};

struct wthread *wthread_create(struct wthread_create_arg *arg);
void wthread_destroy(struct wthread *wthread);

#endif
