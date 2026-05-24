#ifndef BUDDY_UTIL_H
#define BUDDY_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

size_t buddy_bitmap_size(size_t blocks_count);

// Updates bit for given buddy layers and all upper.
void buddy_mark(uint8_t *bitmap, size_t layer, size_t index);

void buddy_set(uint8_t *bitmap, size_t layer, size_t index);
void buddy_unset(uint8_t *bitmap, size_t layer, size_t index);
bool buddy_get(uint8_t *bitmap, size_t layer, size_t index);

size_t block_size(size_t layer, size_t root_block_size);
size_t best_fit_layer(size_t pages, size_t root_block_size);

void *to_pointer(void *begin, size_t layer, size_t index,
                 size_t root_block_size);
size_t to_index(void *begin, size_t layer, void *pointer,
                size_t root_block_size);

// Returns index of 8-bit mask containing value for desired index in buddy
size_t mask_index(size_t layer, size_t index);

// Returns index of required bit in mask_index result
size_t bit_index(size_t layer, size_t index);

#endif // #ifndef BUDDY_UTIL_H
