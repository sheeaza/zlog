#ifndef __FIFO_H
#define __FIFO_H

#include <pthread.h>

#include "ref.h"

/**
 * todo add mem fence, 1 in 1 out, lock free
 */
struct fifo {
	unsigned int	in;
	unsigned int	out;
	unsigned int size;
	char data[];
};

struct fifo *fifo_create(unsigned int size);
void fifo_destroy(struct fifo *fifo);

unsigned int fifo_in(struct fifo *fifo, const char *buf, unsigned int size);
unsigned int fifo_out(struct fifo *fifo, char *buf, unsigned int size);
unsigned int fifo_used(struct fifo *fifo);
unsigned int fifo_freed(struct fifo *fifo);

struct fifo_ref {
	struct fifo *fifo;
	struct ref ref;
};

struct fifo_ref *fifo_ref_create(unsigned int size);

void fifo_ref_release(struct ref *ref);

#endif
