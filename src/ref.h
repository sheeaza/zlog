#ifndef __REF_H
#define __REF_H

#include <pthread.h>

struct ref {
	int cnt;
};

static inline void ref_init(struct ref *ref)
{
	ref->cnt = 1;
}

void ref_get(struct ref *ref, pthread_mutex_t *lock);
void ref_put(struct ref *ref, pthread_mutex_t *lock, void (*release)(struct ref *));

#endif
