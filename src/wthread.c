#include <stdlib.h>
#include <pthread.h>

#include "zc_profile.h"
#include "wthread.h"

static void *wthread_func(void *arg)
{
	zc_debug("xx");
	return NULL;
}

struct wthread *wthread_create(struct wthread_create_arg *arg)
{
	pthread_attr_t attr;
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
    list_head_init(&wthread->per_thread_data);

	pthread_t tid;
	ret = pthread_create(&tid, &attr, wthread_func, wthread);
	if (ret) {
		zc_error("pthread_create failed");
		goto free_wthread;
	}

	wthread->tid = tid;
	goto free_attr;

free_wthread:
	free(wthread);
	wthread = NULL;
free_attr:
	ret = pthread_attr_destroy(&attr);
	if (ret) {
		zc_error("pthread_attr_destroy failed, ignore");
	}

	return wthread;
}

void wthread_destroy(struct wthread *wthread)
{
	int ret = pthread_join(wthread->tid, NULL);
	if (ret) {
		zc_error("pthread_join failed %d, ignore", ret);
	}

    /* todo del list */

	free(wthread);
}
