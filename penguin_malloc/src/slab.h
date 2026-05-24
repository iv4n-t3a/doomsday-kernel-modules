#ifndef SLAB_H
#define SLAB_H

#include "buddy.h"
#include "freelist.h"

typedef struct slab_cache_header {
  size_t size;
  void *begin;
  void *unmanaged_begin;

  struct slab_cache_header *prev_cache;
  struct slab_cache_header *next_cache;

  freelist_t freelist;
  size_t allocated_objects;
  size_t max_objects;

  bool root_cache;
} slab_cache_header_t;

typedef struct {
  slab_cache_header_t *cache;
} slab_allocated_object_header_t;

typedef struct {
  buddy_alloc_t *buddy;

  slab_cache_header_t *first_cache;
  slab_cache_header_t *first_free_cache;
  slab_cache_header_t *last_cache;

  size_t objsize;
  size_t cachesize;
} slab_alloc_t;

slab_alloc_t *slab_init(buddy_alloc_t *buddy, size_t size);

void *slab_alloc(slab_alloc_t *alloc);

void slab_free(slab_alloc_t *alloc, void *mem);

void slab_destroy(slab_alloc_t *alloc);

#endif // #ifndef SLAB_H
