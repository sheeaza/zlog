#include <assert.h>

#include "ref.h"

void ref_get(struct ref *ref, pthread_mutex_t *lock)
{
	if (lock) {
		assert(!pthread_mutex_lock(lock));
	}

	ref->cnt++;
	if (lock) {
		assert(!pthread_mutex_unlock(lock));
	}
}

void ref_put(struct ref *ref, pthread_mutex_t *lock, void (*release)(struct ref *))
{
	if (lock) {
		assert(!pthread_mutex_lock(lock));
	}

	ref->cnt--;
	if (ref->cnt == 0) {
		if (release) {
			release(ref);
		}
	}

	if (lock) {
		assert(!pthread_mutex_unlock(lock));
	}
}
