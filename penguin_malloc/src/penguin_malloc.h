#ifndef PENGUIN_MALLOC_H
#define PENGUIN_MALLOC_H

#include "buddy.h"
#include <stddef.h>

void penguin_malloc_init(buddy_alloc_t *alloc);

void *penguin_malloc(size_t size);

void penguin_free(void *ptr);

#endif // #ifndef PENGUIN_MALLOC_H
