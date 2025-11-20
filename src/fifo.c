#include <stdlib.h>
#include <assert.h>
#include <stdatomic.h>

#include "zc_profile.h"
#include "misc.h"

#include "fifo.h"

struct fifo *fifo_create(unsigned int size)
{
	unsigned int total_size = size + sizeof(struct fifo);
	struct fifo *fifo = malloc(total_size);
	if (!fifo) {
		zc_error("failed to alloc fifo");
		return NULL;
	}

	fifo->in = 0;
	fifo->out = 0;
	fifo->size = size;
	return fifo;
}

void fifo_destroy(struct fifo *fifo)
{
	free(fifo);
}

/* todo: add mem fence */
char *fifo_in_ref(struct fifo *fifo, unsigned int size)
{
    unsigned int free_size = fifo->size - (fifo->in - atomic_load_explicit(&fifo->out, memory_order_acquire));
	if (size > free_size) {
        zc_error("fifo not enough space");
        return NULL;
	}

    return &fifo->data[fifo->in % fifo->size];
}

void fifo_in_commit(struct fifo *fifo, unsigned int size)
{
    assert(fifo_freed(fifo) >= size);
    atomic_store_explicit(&fifo->in, fifo->in + size, memory_order_release);
}

unsigned int fifo_out_ref(struct fifo *fifo, char **buf)
{
    unsigned int used_size = atomic_load_explicit(&fifo->in, memory_order_acquire) - fifo->out;
    if (used_size == 0)
        return 0;

    *buf = &fifo->data[fifo->out % fifo->size];
    return used_size;
}

void fifo_out_commit(struct fifo *fifo, unsigned int size)
{
    assert(size <= fifo_pushed(fifo));
    atomic_store_explicit(&fifo->out, fifo->out + size, memory_order_release);
}

struct fifo_ref *fifo_ref_create(unsigned int size)
{
	struct fifo *fifo = fifo_create(size);
	if (!fifo) {
		zc_error("failed to create fifo");
		return NULL;
	}

	struct fifo_ref *fifo_ref = malloc(sizeof(*fifo_ref));
	if (!fifo_ref) {
		zc_error("failed to create fifo_ref");
		goto free_fifo;
	}
	ref_init(&fifo_ref->ref);

	return fifo_ref;

free_fifo:
	fifo_destroy(fifo);
	return NULL;
}

static void fifo_ref_destroy(struct fifo_ref *fifo_ref)
{
	fifo_destroy(fifo_ref->fifo);
	free(fifo_ref);
}

void fifo_ref_release(struct ref *ref)
{
	struct fifo_ref *fifo_ref = container_of(ref, struct fifo_ref, ref);

	fifo_ref_destroy(fifo_ref);
}
