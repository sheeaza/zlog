#include <stdlib.h>
#include <assert.h>

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
	if (size > fifo_freed(fifo)) {
        zc_error("fifo not enough space");
        return NULL;
	}

    return &fifo->data[fifo->in];
}

void fifo_in_commit(struct fifo *fifo, unsigned int size)
{
    assert(fifo_freed(fifo) > size);
    fifo->in += size;
}

unsigned int fifo_out_ref(struct fifo *fifo, char **buf)
{
    if (fifo_used(fifo) == 0)
        return 0;

    *buf = &fifo->data[fifo->out];
    return fifo_used(fifo);
}

void fifo_out_commit(struct fifo *fifo, unsigned int size)
{
    assert(size < fifo_used(fifo));
    fifo->out += size;
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
