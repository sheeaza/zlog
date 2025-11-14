#include <stdlib.h>

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
