#ifndef __LIST_H
#define __LIST_H

#include <stddef.h>

struct list_head {
    struct list_head *next, *prev;
};

static inline void list_head_init(struct list_head *head)
{
    head->next = NULL;
    head->prev = NULL;
}

static inline void list_head_add(struct list_head *head, struct list_head *new)
{
    new->next = head->next;
    new->prev = head;
    head->next = new;
}

static inline void list_del(struct list_head *entry)
{
    entry->prev->next = entry->next;
    entry->prev = NULL;
    entry->next = NULL;
}

#endif
