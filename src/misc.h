#ifndef __MISC_H
#define __MISC_H

#include <stddef.h>
#include <pthread.h>

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

struct wthread;
struct zlog_process_data
{
    struct wthread *wthread;
    pthread_mutex_t share_mutex;
};

enum msg_type {
    MSG_TYPE_STRING = 0,
};

struct msg_pack {
    unsigned int type;
    unsigned int size;
    char data[];
};

static inline unsigned int msg_pack_head_size(void)
{
    return sizeof(struct msg_pack);
}

#endif
