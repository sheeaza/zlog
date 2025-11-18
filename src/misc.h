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
    MSG_TYPE_STRING = 1,
    MSG_TYPE_PER_PRINT_DATA,
};

struct msg_pack {
    unsigned int type;
    struct zlog_category_s *category;
    const char *file;
    size_t filelen;
    const char *func;
    size_t funclen;
    long line;
    int level;
    struct timespec ts;

    char data[];
};

struct zlog_category_s;
struct msg_per_print_str {
    unsigned int formatted_string_size;
    char formatted_string[];
};

struct zlog_thread_s;
struct zlog_output_data {
    struct zlog_thread_s *thread;
    struct msg_pack *pack;
    struct zlog_buf_s *tmp_buf;
    struct {
        char *str;
        size_t len;
    } time_str;
};

static inline unsigned int msg_pack_head_size(void)
{
    return sizeof(struct msg_pack);
}

static inline unsigned int msg_per_print_data_head_size(void)
{
    return sizeof(struct msg_per_print_str);
}

#endif
