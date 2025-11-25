#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdatomic.h>

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
        zc_error("log consumer exited, return");
        goto exit;
    }

    char *buf = fifo_in_ref(logc->event.queue, event_pack_size());
    if (!buf) {
        /* zc_error("not enough space"); */
        ret = 1;
        goto exit;
    }

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
        struct msg_head *head = fifo_reserve(logc->event.queue, msg_cmd_size());
        if (!head) {
            zc_error("not enough space, retry, queue free %d", fifo_unused(logc->event.queue));
            /* todo add sleep here */
            continue;
        }
        struct msg_cmd *cmd = (struct msg_cmd *)head->data;
        cmd->type.val = MSG_TYPE_CMD;
        cmd->cmd = MSG_CMD_EXIT;
        fifo_commit(logc->event.queue, head);
        break;
    }
    assert(!pthread_mutex_unlock(&logc->event.queue_in_lock));
#if 0
    assert(!pthread_mutex_lock(&logc->event.queue_in_lock));
    logc->exit = true; /* ensure this is the last */
    for (;;) {
        char *buf = fifo_in_ref(logc->event.queue, event_pack_size());
        if (!buf) {
            zc_error("not enough space, retry, queue free %d", fifo_unused(logc->event.queue));
            /* todo add sleep here */
            continue;
        }
        struct event_pack *pack = (struct event_pack *)buf;
        pack->type = EVENT_TYPE_EXIT;
        fifo_in_commit(logc->event.queue, event_pack_size());
        break;
    }
    assert(!pthread_mutex_unlock(&logc->event.queue_in_lock));
#endif
}

static void enque_signal(struct log_consumer *logc)
{
    assert(!pthread_mutex_lock(&logc->event.siglock));
    logc->event.sig_send++;
    assert(!pthread_cond_signal(&logc->event.cond));
    assert(!pthread_mutex_unlock(&logc->event.siglock));
}

static void handle_log(struct log_consumer *logc, struct msg_head *head, bool *exit)
{
    struct msg_meta *meta = NULL;
    struct msg_usr_str *str = NULL;

    unsigned payload_size = head->total_size - msg_head_size();

    for (unsigned offset = 0; offset < payload_size;) {
        struct msg_type *msg_type = (struct msg_type *)&head->data[offset];
        switch (msg_type->val) {
            case MSG_TYPE_META:
                meta = (struct msg_meta *)msg_type;
                offset += msg_meta_size();
                break;
            case MSG_TYPE_USR_STR:
                str = (struct msg_usr_str *)msg_type;
                offset += str->total_size;
                break;
            case MSG_TYPE_CMD: {
                struct msg_cmd *cmd = (struct msg_cmd *)msg_type;
                if (cmd->cmd == MSG_CMD_EXIT) {
                    *exit = true;
                    return;
                }
                offset += msg_cmd_size();
                break;
            }
            default:
                zc_error("invalid msg type %d", msg_type->val);
                offset = payload_size;
                break;
        }
    }

    if (meta == NULL) {
        zc_error("no meta in packet");
        return;
    }

    struct zlog_output_data data = {
        .thread = meta->thread,
        .meta = meta,
        .usr_str = str,
        .time_str.str = logc->time_str,
        .time_str.len = sizeof(logc->time_str),
        .tmp_buf = logc->msg_buf,
    };
    int ret = zlog_category_output(meta->category, NULL, &data);
    if (ret) {
        zc_error("failed to output %d", ret);
    }
#if 0
   struct fifo *fifo = thread->producer.fifo;

   char *buf;
   unsigned int len = fifo_out_ref(fifo, &buf);
   assert(len > 0);

   struct msg_pack *pack = (struct msg_pack *)buf;

   switch (pack->type) {
   case MSG_TYPE_USR_STR: {
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
                                ((struct msg_usr_str *)pack->data)->formatted_string_size;
       fifo_out_commit(fifo, pack_size);
       break;
   }
   default:
       break;
   }
   /* todo: thread put */
#endif
}

static void *logc_func(void *arg)
{
    struct log_consumer *logc = arg;
    bool exit = false;
    unsigned prev_sig_send = 0;
    for (bool prev_sig_send_valid = false; !exit;) {
        pthread_mutex_lock(&logc->event.siglock);
        /* impossible */
        if (logc->event.sig_recv > logc->event.sig_send) {
            assert(0);
        }
        /* empty */
        if (logc->event.sig_recv == logc->event.sig_send ||
            (prev_sig_send_valid && prev_sig_send == logc->event.sig_send)) {
            pthread_cond_wait(&logc->event.cond, &logc->event.siglock);
        }
        prev_sig_send = logc->event.sig_send;
        pthread_mutex_unlock(&logc->event.siglock);
        /* has data */

        struct msg_head *head = fifo_peek(logc->event.queue);
        assert(head);
        unsigned flag = atomic_load_explicit(&head->flags, memory_order_acquire);
        if (flag == MSG_HEAD_FLAG_RESERVED) {
            /* not commit yet, continue wait */
            prev_sig_send_valid = true;
            continue;
        }

        logc->event.sig_recv++;
        if (flag == MSG_HEAD_FLAG_COMMITED) {
            handle_log(logc, head, &exit);
        } else if (flag == MSG_HEAD_FLAG_DISCARDED) {
        } else {
            assert(1);
        }
        prev_sig_send_valid = false;

        fifo_out(logc->event.queue, head);
#if 0
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
            assert(!fifo_used(logc->event.queue));
            exit = true;
            break;
        default:
            zc_error("invalid pack type %d", pack.type);
            break;
        }
#endif
    }
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

    logc->event.queue = fifo_create(arg->conf->log_consumer.consumer_msg_queue_len);
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

    printf("exit cnt sig send %d, sig re %d\n", logc->event.sig_send, logc->event.sig_recv);
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

struct msg_head *log_consumer_queue_reserve(struct log_consumer *logc, unsigned size)
{
    struct msg_head *head = NULL;

    pthread_mutex_lock(&logc->event.queue_in_lock);
    if (logc->exit) {
        zc_error("log consumer exited, return");
        goto exit;
    }

    head = fifo_reserve(logc->event.queue, size);

exit:
    pthread_mutex_unlock(&logc->event.queue_in_lock);

    return head;
}

void log_consumer_queue_commit_signal(struct log_consumer *logc, struct msg_head *head, bool discard)
{
    if (discard) {
        fifo_discard(logc->event.queue, head);
    } else {
        fifo_commit(logc->event.queue, head);
    }
    enque_signal(logc);
}
