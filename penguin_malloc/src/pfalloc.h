#ifndef PFALLOC_H
#define PFALLOC_H

#include "buddy.h"

void pfalloc_init(void *pool_begin, size_t pool_size);

buddy_alloc_t *get_pfalloc_buddy();

void *pfalloc(size_t pages);

void pffree(void *ptr);

#endif  // #ifndef PFALLOC_H
