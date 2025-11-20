#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include "zc_profile.h"
#include "thread.h"
#include "misc.h"
#include "fifo.h"
#include "category.h"
#include "rule.h"
#include "conf.h"

#include "consumer.h"

static int enque_event(struct log_consumer *logc, struct event_pack *pack)
{
    int ret = 0;

    pthread_mutex_lock(&logc->event.queue_in_lock);
    if (logc->exit) {
        zc_debug("log consumer exited, return");
        goto exit;
    }

    char *buf = fifo_in_ref(logc->event.queue, event_pack_size());
    if (!buf) {
        zc_error("not enough space");
        ret = 1;
        goto exit;
    }

    printf("freed %d\n", fifo_freed(logc->event.queue));
    struct event_pack *_pack = (struct event_pack *)buf;
    _pack->type = pack->type;
    _pack->data = pack->data;
    fifo_in_commit(logc->event.queue, event_pack_size());

exit:
    pthread_mutex_unlock(&logc->event.queue_in_lock);
    return ret;
}

static void enque_event_exit(struct log_consumer *logc)
{
    assert(!pthread_mutex_lock(&logc->event.queue_in_lock));
    logc->exit = true; /* ensure this is the last */
    for (;;) {
        char *buf = fifo_in_ref(logc->event.queue, event_pack_size());
        if (!buf) {
            zc_error("not enough space, retry, queue free %d", fifo_freed(logc->event.queue));
            /* todo add sleep here */
            continue;
        }
        struct event_pack *pack = (struct event_pack *)buf;
        pack->type = EVENT_TYPE_EXIT;
        fifo_in_commit(logc->event.queue, event_pack_size());
        break;
    }
    assert(!pthread_mutex_unlock(&logc->event.queue_in_lock));
}

static void enque_signal(struct log_consumer *logc)
{
    assert(!pthread_mutex_lock(&logc->event.siglock));
    logc->event.sig_send++;
    assert(!pthread_cond_signal(&logc->event.cond));
    assert(!pthread_mutex_unlock(&logc->event.siglock));
}

static void handle_log(struct log_consumer *logc, struct zlog_thread_s *thread)
{
   struct fifo *fifo = thread->producer.fifo;

   char *buf;
   unsigned int len = fifo_out_ref(fifo, &buf);
   assert(len > 0);

   struct msg_pack *pack = (struct msg_pack *)buf;

   switch (pack->type) {
   case MSG_TYPE_PER_PRINT_DATA: {
       zlog_category_t *category = pack->category;
       struct zlog_output_data data = {
           .pack = pack,
           .thread = thread,
           .time_str.str = logc->time_str,
           .time_str.len = sizeof(logc->time_str),
           .tmp_buf = logc->msg_buf,
       };
       int ret = zlog_category_output(category, NULL, &data);
       if (ret) {
           zc_error("failed to output %d", ret);
       }
       unsigned int pack_size = msg_pack_head_size() + msg_per_print_data_head_size() +
                                ((struct msg_per_print_str *)pack->data)->formatted_string_size;
       fifo_out_commit(fifo, pack_size);
       break;
   }
   default:
       break;
   }
   /* todo: thread put */
}

static void *logc_func(void *arg)
{
    struct log_consumer *logc = arg;
    bool exit = false;
    for (; !exit;) {
        pthread_mutex_lock(&logc->event.siglock);
        /* impossible */
        if (logc->event.sig_recv > logc->event.sig_send) {
            assert(0);
        }
        /* empty */
        if (logc->event.sig_recv == logc->event.sig_send) {
            pthread_cond_wait(&logc->event.cond, &logc->event.siglock);
        }
        logc->event.sig_recv++;
        pthread_mutex_unlock(&logc->event.siglock);
        /* has data */

        char *buf;
        unsigned int size = fifo_out_ref(logc->event.queue, &buf);
        assert(size >= event_pack_size());
        struct event_pack *p_pack = (struct event_pack *)buf;
        struct event_pack pack = *p_pack;
        fifo_out_commit(logc->event.queue, event_pack_size());

        switch (pack.type) {
        case EVENT_TYPE_LOG:
            handle_log(logc, pack.data);
            break;
        case EVENT_TYPE_EXIT:
            assert(!fifo_pushed(logc->event.queue));
            exit = true;
            break;
        default:
            zc_error("invalid pack type %d", pack.type);
            break;
        }
    }
    zc_debug("exit");
	return NULL;
}

struct log_consumer *log_consumer_create(struct logc_create_arg *arg)
{
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	if (ret) {
		zc_error("pthread_attr_init failed");
		return NULL;
	}

	struct log_consumer *logc = calloc(1, sizeof(*logc));
	if (!logc) {
		zc_error("pthread_attr_init failed");
		goto free_attr;
	}

	logc->msg_buf = zlog_buf_new(arg->conf->buf_size_min, arg->conf->buf_size_max, "..." FILE_NEWLINE);
	if (!logc->msg_buf) {
		zc_error("zlog_buf_new fail");
		goto free_logc;
	}

    ret = pthread_mutex_init(&logc->event.queue_in_lock, NULL);
    if (ret) {
		zc_error("pthread_mutex_init failed, %d", ret);
		goto free_msgbuf;
    }

    logc->event.queue = fifo_create(arg->conf->log_consumer.consumer_msg_queue_len * event_pack_size());
    if (!logc->event.queue) {
		zc_error("fifo_create msg queue failed");
        goto free_lock;
    }

    ret = pthread_mutex_init(&logc->event.siglock, NULL);
    if (ret) {
		zc_error("pthread_mutex_init failed, %d", ret);
		goto free_equeue;
    }

    ret = pthread_cond_init(&logc->event.cond, NULL);
    if (ret) {
		zc_error("pthread_cond_init failed, %d", ret);
		goto free_sig_lock;
    }

    /* thread create must put at end */
	pthread_t tid;
	ret = pthread_create(&tid, &attr, logc_func, logc);
	if (ret) {
		zc_error("pthread_create failed");
		goto free_cond;
	}

	logc->tid = tid;
	goto free_attr;

free_cond:
    ret = pthread_cond_destroy(&logc->event.cond);
	if (ret) {
		zc_error("pthread_cond_destroy failed, ignore");
	}

free_sig_lock:
    ret = pthread_mutex_destroy(&logc->event.siglock);
	if (ret) {
		zc_error("pthread_mutex_destroy failed, ignore");
	}

free_equeue:
    fifo_destroy(logc->event.queue);

free_lock:
    ret = pthread_mutex_destroy(&logc->event.queue_in_lock);
	if (ret) {
		zc_error("pthread_mutex_destroy failed, ignore");
	}

free_msgbuf:
    zlog_buf_del(logc->msg_buf);
free_logc:
	free(logc);
	logc = NULL;
free_attr:
	ret = pthread_attr_destroy(&attr);
	if (ret) {
		zc_error("pthread_attr_destroy failed, ignore");
	}

	return logc;
}

void log_consumer_destroy(struct log_consumer *logc)
{
    enque_event_exit(logc);
    enque_signal(logc);

	int ret = pthread_join(logc->tid, NULL);
	if (ret) {
		zc_error("pthread_join failed %d, ignore", ret);
	}

    /* todo, check if need free all event in queue, exit enough ? */

    fifo_destroy(logc->event.queue);
    ret = pthread_cond_destroy(&logc->event.cond);
	if (ret) {
		zc_error("pthread_cond_destroy failed, ignore");
	}
    ret = pthread_mutex_destroy(&logc->event.queue_in_lock);
	if (ret) {
		zc_error("pthread_mutex_destroy failed, ignore");
	}

    zlog_buf_del(logc->msg_buf);
	free(logc);
}

int log_consumer_enque_wakeup(struct log_consumer *logc, struct event_pack *pack)
{
    int ret = 0;

    ret = enque_event(logc, pack);
    if (ret)
        return ret;
    enque_signal(logc);

    return ret;
}
