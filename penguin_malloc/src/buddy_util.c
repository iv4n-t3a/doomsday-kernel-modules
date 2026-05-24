#include "buddy_util.h"
#include "config.h"
#include "util.h"

size_t buddy_bitmap_size(size_t blocks_count) { return (blocks_count + 7) / 8; }

void buddy_mark(uint8_t *bitmap, size_t layer, size_t index) {
  while (layer != 0 && buddy_get(bitmap, layer, index ^ 1)) {
    buddy_unset(bitmap, layer, index ^ 1);
    layer -= 1;
    index /= 2;
  }
  buddy_set(bitmap, layer, index);
}

void buddy_set(uint8_t *bitmap, size_t layer, size_t index) {
  bitmap[mask_index(layer, index)] |= 1ull << bit_index(layer, index);
}
void buddy_unset(uint8_t *bitmap, size_t layer, size_t index) {
  bitmap[mask_index(layer, index)] &= ~(1ull << bit_index(layer, index));
}
bool buddy_get(uint8_t *bitmap, size_t layer, size_t index) {
  return (bitmap[mask_index(layer, index)] &
          (1ull << bit_index(layer, index))) != 0;
}

size_t block_size(size_t layer, size_t root_block_size) {
  return (root_block_size >> layer);
}

size_t best_fit_layer(size_t pages, size_t root_block_size) {
  return log2_upper(root_block_size / pages);
}

void *to_pointer(void *begin, size_t layer, size_t index,
                 size_t root_block_size) {
  return (uint8_t *)begin +
         (block_size(layer, root_block_size) * index * PAGE_SIZE);
}

size_t to_index(void *begin, size_t layer, void *pointer,
                size_t root_block_size) {
  return (size_t)((uint8_t *)pointer - (uint8_t *)begin) /
         block_size(layer, root_block_size) / PAGE_SIZE;
}

size_t mask_index(size_t layer, size_t index) {
  if (layer < 3) {
    return 0;
  }

  size_t buddy_start =
      1ull << (layer - 3); // Skip first 3 buddies as they occupy first byte
  return buddy_start + index / 8;
}

size_t bit_index(size_t layer, size_t index) {
  if (layer < 3) { // 1-bit padding
    return index + (1 << layer);
  }

  return index % 8;
}
