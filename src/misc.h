#ifndef __MISC_H
#define __MISC_H

#include <stddef.h>

#define container_of(ptr, type, member) 				\
	((type *)((char *)(ptr) - offsetof(type, member)))

#endif
