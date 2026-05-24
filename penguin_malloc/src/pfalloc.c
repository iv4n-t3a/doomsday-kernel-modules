#include "pfalloc.h"

#include "config.h"
#include "buddy.h"

static buddy_alloc_t *global_buddy;

void pfalloc_init(void *pool_begin, size_t pool_size) {
  global_buddy = buddy_init(pool_begin, (uint8_t *)pool_begin + pool_size);
}

buddy_alloc_t *get_pfalloc_buddy() {
  return global_buddy;
}

void *pfalloc(size_t pages) {
  if (global_buddy == NULL) {
    return NULL;
  }
  return buddy_alloc(global_buddy, pages);
}

void pffree(void *ptr) {
  if (global_buddy != NULL && ptr != NULL) {
    return buddy_free(global_buddy, ptr);
  }
}

