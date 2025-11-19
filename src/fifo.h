#ifndef __FIFO_H
#define __FIFO_H

#include <pthread.h>

#include "ref.h"

/**
 * todo add mem fence, 1 in 1 out, lock free
 */
struct fifo {
    unsigned int in;
    unsigned int out;
    unsigned int size;
    char data[];
};

struct fifo *fifo_create(unsigned int size);
void fifo_destroy(struct fifo *fifo);

char *fifo_in_ref(struct fifo *fifo, unsigned int size);
void fifo_in_commit(struct fifo *fifo, unsigned int size);

unsigned int fifo_out_ref(struct fifo *fifo, char **buf);
void fifo_out_commit(struct fifo *fifo, unsigned int size);

static inline unsigned int fifo_pushed(struct fifo *fifo)
{
	return fifo->in - fifo->out;
}

static inline unsigned int fifo_freed(struct fifo *fifo)
{
	return fifo->size - (fifo->in - fifo->out);
}


struct fifo_ref {
	struct fifo *fifo;
	struct ref ref;
};

struct fifo_ref *fifo_ref_create(unsigned int size);

void fifo_ref_release(struct ref *ref);

#endif
