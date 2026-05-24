#include "slab.h"
#include "buddy.h"
#include "config.h"

static size_t slab_cache_pages(size_t objsize) {
  if (objsize * SLAB_MIN_CACHE_OBJECTS < SLAB_MIN_CACHE_PAGES) {
    return SLAB_MIN_CACHE_PAGES;
  }
  return SLAB_MIN_CACHE_OBJECTS * sizeof(objsize);
}

static void slab_cache_move_down(slab_alloc_t *alloc, slab_cache_header_t *cache) {
  if (cache->next_cache == NULL) {
    return;
  }

  cache->next_cache->next_cache = cache;
  cache->next_cache->prev_cache = cache->prev_cache;

  cache->prev_cache = cache->next_cache;
  cache->next_cache = cache->next_cache->next_cache;

  if (cache->prev_cache == NULL) {
    alloc->first_cache = cache->next_cache;
  }
}

static void slab_cache_move_up(slab_alloc_t *alloc, slab_cache_header_t *cache) {
  if (cache->prev_cache == NULL) {
    return;
  }

  cache->prev_cache->prev_cache = cache;
  cache->prev_cache->next_cache = cache->next_cache;

  cache->next_cache = cache->prev_cache;
  cache->prev_cache = cache->prev_cache->prev_cache;

  if (cache->next_cache == NULL) {
    alloc->last_cache = cache->prev_cache;
  }
}

static void slab_cache_init(slab_alloc_t *alloc, void *begin, void *end) {
  slab_cache_header_t *cache = begin;

  cache->size = (uint8_t *)end - (uint8_t *)begin - sizeof(slab_cache_header_t);

  cache->begin = begin;
  cache->unmanaged_begin = (uint8_t *)begin + sizeof(slab_cache_header_t);
  cache->prev_cache = NULL;
  cache->next_cache = NULL;
  cache->freelist = NULL;
  cache->allocated_objects = 0;
  cache->max_objects = cache->size / alloc->objsize;
  cache->root_cache = false;

  if (alloc->first_cache == NULL) {
    cache->root_cache = true;
    alloc->first_cache = cache;
    alloc->last_cache = cache;
  } else {
    alloc->last_cache->next_cache = cache;
    cache->prev_cache = alloc->last_cache;
    alloc->last_cache = cache;
  }

  if (alloc->first_free_cache == NULL) {
    alloc->first_free_cache = cache;
  }
}

static void *slab_cache_alloc(slab_alloc_t *alloc, slab_cache_header_t *cache) {
  if (cache->allocated_objects == cache->max_objects) {
    return NULL;
  }

  cache->allocated_objects += 1;

  if (cache->prev_cache != NULL &&
      cache->prev_cache->allocated_objects < cache->allocated_objects) {
    slab_cache_move_up(alloc, cache);
  }

  while (alloc->first_free_cache != NULL &&
         alloc->first_free_cache->allocated_objects ==
             alloc->first_free_cache->max_objects) {
    alloc->first_free_cache = alloc->first_free_cache->next_cache;
  }

  void *ptr = freelist_pop(&cache->freelist);

  if (ptr == NULL) {
    ptr = cache->unmanaged_begin;
    cache->unmanaged_begin = (uint8_t *)cache->unmanaged_begin + alloc->objsize;
  }

  slab_allocated_object_header_t *header = ptr;
  header->cache = cache;

  return (uint8_t *)ptr + sizeof(slab_allocated_object_header_t);
}

static void slab_cache_free(slab_alloc_t *alloc, slab_cache_header_t *cache,
                     void *ptr) {
  freelist_push(&cache->freelist, ptr);
  cache->allocated_objects -= 1;

  if (cache->next_cache != NULL &&
      cache->next_cache->allocated_objects > cache->allocated_objects) {
    slab_cache_move_down(alloc, cache);
  }
}

static void slab_cache_destroy(slab_alloc_t *alloc, slab_cache_header_t *cache) {
  if (cache->prev_cache == NULL) {
    alloc->first_cache = cache->next_cache;
  }
  if (cache->next_cache == NULL) {
    alloc->last_cache = cache->prev_cache;
  }
  if (cache->prev_cache != NULL && cache->next_cache != NULL) {
    cache->prev_cache->next_cache = cache->next_cache;
    cache->next_cache->prev_cache = cache->prev_cache;
  }

  uint8_t *mem = (uint8_t *)cache;
  if (cache->root_cache) {
    mem -= sizeof(slab_alloc_t);
  }
  buddy_free(alloc->buddy, mem);
}

slab_alloc_t *slab_init(buddy_alloc_t *buddy, size_t size) {
  size_t objsize = size + sizeof(slab_allocated_object_header_t);
  size_t cachesize = slab_cache_pages(objsize);
  void *mem = buddy_alloc(buddy, cachesize);

  if (mem == NULL) {
    return NULL;
  }

  slab_alloc_t *alloc = mem;

  alloc->buddy = buddy;
  alloc->first_cache = NULL;
  alloc->first_free_cache = NULL;
  alloc->last_cache = NULL;
  alloc->objsize = objsize;
  alloc->cachesize = cachesize;

  void *cache_begin = (uint8_t *)mem + sizeof(slab_alloc_t);
  void *cache_end = (uint8_t *)mem + cachesize * PAGE_SIZE;

  slab_cache_init(alloc, cache_begin, cache_end);

  return alloc;
}

void *slab_alloc(slab_alloc_t *alloc) {
  if (alloc->first_free_cache == NULL) {
    void *cache = buddy_alloc(alloc->buddy, alloc->cachesize);

    if (cache == NULL) {
      return NULL;
    }

    slab_cache_init(alloc, cache,
                    (uint8_t *)cache + alloc->cachesize * PAGE_SIZE);
  }
  return slab_cache_alloc(alloc, alloc->first_free_cache);
}

void slab_free(slab_alloc_t *alloc, void *ptr) {
  void *mem = (uint8_t *)ptr - sizeof(slab_allocated_object_header_t);
  slab_allocated_object_header_t *header = mem;
  slab_cache_header_t *cache = header->cache;
  slab_cache_free(alloc, cache, mem);

  if (cache->allocated_objects == 0 && !cache->root_cache) {
    slab_cache_destroy(alloc, cache);
    if (cache == alloc->first_free_cache) {
      alloc->first_free_cache = NULL;
    }
  }
}

void slab_destroy(slab_alloc_t *alloc) {
  slab_alloc_t alloc_copy = *alloc;

  while (alloc_copy.first_cache != NULL) {
    slab_cache_destroy(&alloc_copy, alloc_copy.first_cache);
  }
}
