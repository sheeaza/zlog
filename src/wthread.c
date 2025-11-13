#include <stdlib.h>
#include <pthread.h>

#include "wthread.h"
#include "zc_profile.h"

static void *wthread_func(void *arg)
{
	return NULL;
}

struct wthread *wthread_create(struct wthread_create_arg *arg)
{
	pthread_attr_t      attr;
	int ret;

	ret = pthread_attr_init(&attr);
	if (ret) {
		zc_error("pthread_attr_init failed");
		return NULL;
	}

	struct wthread *wthread = calloc(1, sizeof(*wthread));
	if (!wthread) {
		zc_error("pthread_attr_init failed");
		goto free_attr;
	}
	pthread_t tid;
	ret = pthread_create(&tid, &attr, wthread_func, wthread);
	if (!ret) {
		zc_error("pthread_create failed");
	}

	return wthread;

free_attr:
	ret = pthread_attr_destroy(&attr);
	if (!ret) {
		zc_error("pthread_attr_destroy failed, ignore");
	}

	return NULL;
}

void wthread_destroy(struct wthread *wthread)
{
}
