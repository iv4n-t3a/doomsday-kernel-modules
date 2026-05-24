#include "buddy.h"

#include "string.h"
#include "buddy_util.h"
#include "freelist.h"
#include "util.h"

void buddy_update_params(buddy_alloc_t *alloc, void *begin, void *end) {
  alloc->blocks_count = pages_count(begin, end);
  alloc->root_block_size = max_lower_power_of_2(alloc->blocks_count);
  alloc->buddy_layers = log2_upper(alloc->blocks_count);
}

buddy_alloc_t *buddy_init(void *begin, void *end) {
  if (pages_count(begin, end) < 16) {
    return NULL;
  }

  buddy_alloc_t *alloc = begin;
  begin += sizeof(buddy_alloc_t);

  buddy_update_params(alloc, begin, end);

  alloc->freelist = begin;
  begin = (uint8_t *)begin + alloc->buddy_layers * sizeof(freelist_t);

  alloc->bitmap = begin;
  begin = (uint8_t *)begin + buddy_bitmap_size(alloc->blocks_count);

  // Update after begin move
  begin = align_addr_to_page_size_up(begin);
  end = align_addr_to_page_size_down(end);
  buddy_update_params(alloc, begin, end);

  memset(alloc->freelist, 0, alloc->buddy_layers * sizeof(freelist_t));
  memset(alloc->bitmap, 0xFF, buddy_bitmap_size(alloc->blocks_count));

  freelist_push(&alloc->freelist[0], begin);
  buddy_unset(alloc->bitmap, 0, 0);

  alloc->begin = begin;

  return alloc;
}

void *buddy_alloc(buddy_alloc_t *alloc, size_t pages) {
  size_t bf_layer = best_fit_layer(pages, alloc->root_block_size);
  size_t layer = bf_layer;

  while (layer > 0 && freelist_get(alloc->freelist[layer]) == NULL) {
    layer -= 1;
  }

  void *ptr = freelist_pop(&alloc->freelist[layer]);

  if (ptr == NULL) {
    return NULL;
  }

  size_t index = to_index(alloc->begin, layer, ptr, alloc->root_block_size);

  while (layer < bf_layer) {
    buddy_set(alloc->bitmap, layer, index);

    layer += 1;
    index = index * 2;

    buddy_unset(alloc->bitmap, layer, index);
    buddy_set(alloc->bitmap, layer, index + 1);

    void *freed =
        to_pointer(alloc->begin, layer, index + 1, alloc->root_block_size);
    freelist_push(&alloc->freelist[layer], freed);
  }

  return ptr;
}

void buddy_free(buddy_alloc_t *alloc, void *ptr) {
  int layer = alloc->buddy_layers - 1;
  int index = to_index(alloc->begin, layer, ptr, alloc->root_block_size);

  while (layer > 0 && buddy_get(alloc->bitmap, layer, index)) {
    layer -= 1;
    index /= 2;
  }

  buddy_unset(alloc->bitmap, layer, index);

  while (layer > 0 && !buddy_get(alloc->bitmap, layer, index ^ 1)) {
    buddy_set(alloc->bitmap, layer, index);
    buddy_set(alloc->bitmap, layer, index ^ 1);

    layer -= 1;
    index /= 2;

    buddy_unset(alloc->bitmap, layer, index);
  }

  ptr = to_pointer(alloc->begin, layer, index, alloc->root_block_size);
  freelist_push(&alloc->freelist[layer], ptr);
}
