#include "penguin_malloc.h"
#include "buddy.h"
#include "slab.h"
#include "config.h"

static buddy_alloc_t *global_buddy;
static slab_alloc_t *global_slabs[PENGUIN_MALLOC_SLABS];

#define BUDDY_INDEX PENGUIN_MALLOC_SLABS

static size_t slab_size_by_index(size_t index) {
  return PENGUIN_MALLOC_MIN_SLAB_SIZE << (index + 1);
}

static size_t to_pages_count(size_t size) {
  return (size + PAGE_SIZE - 1) / PAGE_SIZE;
}

static size_t best_fit_slab_index(size_t size) {
  for (size_t i = 0; i < BUDDY_INDEX; ++i) {
    if (size < slab_size_by_index(i)) {
      return i;
    }
  }

  return BUDDY_INDEX;
}

typedef struct {
  size_t slab_index;
} penguin_malloc_header_t;

static penguin_malloc_header_t *get_header(void *mem) {
  return (penguin_malloc_header_t *)mem;
}

void penguin_malloc_init(buddy_alloc_t *alloc) {
  global_buddy = alloc;

  for (size_t i = 0; i < PENGUIN_MALLOC_SLABS; ++i) {
    global_slabs[i] = slab_init(global_buddy, slab_size_by_index(i));
  }
}

void *penguin_malloc(size_t size) {
  size += sizeof(penguin_malloc_header_t);
  size_t index = best_fit_slab_index(size);

  void *mem = NULL;

  if (index != BUDDY_INDEX) {
    mem = slab_alloc(global_slabs[index]);
  }

  if (mem == NULL) { // index == BUDDY_INDEX or slab returned NULL somehow
    index = BUDDY_INDEX;
    mem = buddy_alloc(global_buddy, to_pages_count(size));
  }

  if (mem == NULL) {
    return NULL;
  }

  penguin_malloc_header_t *header = (penguin_malloc_header_t *)mem;
  header->slab_index = index;

  return (uint8_t *)mem + sizeof(penguin_malloc_header_t);
}

void penguin_free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  ptr = (uint8_t *)ptr - sizeof(penguin_malloc_header_t);
  penguin_malloc_header_t *header = (penguin_malloc_header_t *)ptr;

  if (header->slab_index == BUDDY_INDEX) {
    buddy_free(global_buddy, ptr);
  } else {
    slab_free(global_slabs[header->slab_index], ptr);
  }
}
