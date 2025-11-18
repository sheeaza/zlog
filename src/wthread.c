#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "zc_profile.h"
#include "list.h"
#include "thread.h"
#include "misc.h"
#include "fifo.h"
#include "category.h"
#include "rule.h"
#include "conf.h"

#include "wthread.h"

static void *wthread_func(void *arg)
{
    struct wthread *wthread = arg;
    while (!wthread->exit) {
        /* todo: lock ? */
        if (list_empty(&wthread->per_thread_data)) {
            usleep(10000);
            continue;
        }
        list_for_each(&wthread->per_thread_data, pos) {
            zlog_thread_t *thread_data = container_of(pos, zlog_thread_t, producer.node);
            struct fifo *fifo = thread_data->producer.fifo;

            char *buf;
            unsigned int len = fifo_out_ref(fifo, &buf);
            if (len == 0)
                continue;

            for (unsigned int len_used = 0; len_used < len;) {
                struct msg_pack *pack = (struct msg_pack *)(buf + len_used);
                unsigned int pack_size = 0;

                switch (pack->type) {
                case MSG_TYPE_PER_PRINT_DATA: {
                    zlog_category_t *category = pack->category;
                    struct zlog_output_data data = {
                        .pack = pack,
                        .thread = thread_data,
                        .time_str.str = wthread->time_str,
                        .time_str.len = sizeof(wthread->time_str),
                        .tmp_buf = wthread->msg_buf,
                    };
                    int ret = zlog_category_output2(category, &data);
                    if (ret) {
                        zc_error("failed to output %d", ret);
                    }
                    pack_size = msg_pack_head_size() + msg_per_print_data_head_size() + ((struct msg_per_print_str *)pack->data)->formatted_string_size;
                    break;
                }
                default:
                    len_used = len;
                    break;
                }
                len_used += pack_size;
            }
            fifo_out_commit(fifo, len);
        }
    }
	zc_debug("exit");
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

	wthread->msg_buf = zlog_buf_new(arg->conf->buf_size_min, arg->conf->buf_size_max, "..." FILE_NEWLINE);
	if (!wthread->msg_buf) {
		zc_error("zlog_buf_new fail");
		goto free_wthread;
	}

	pthread_t tid;
	ret = pthread_create(&tid, &attr, wthread_func, wthread);
	if (ret) {
		zc_error("pthread_create failed");
		goto free_msgbuf;
	}

	wthread->tid = tid;
	goto free_attr;

free_msgbuf:
    zlog_buf_del(wthread->msg_buf);
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
    wthread->exit = true;
	int ret = pthread_join(wthread->tid, NULL);
	if (ret) {
		zc_error("pthread_join failed %d, ignore", ret);
	}

    /* todo del list */

    zlog_buf_del(wthread->msg_buf);
	free(wthread);
}
