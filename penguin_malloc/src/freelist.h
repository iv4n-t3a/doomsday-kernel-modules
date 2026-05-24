#ifndef FREELIST_H
#define FREELIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct freelist_node {
  struct freelist_node *next;
} freelist_node_t;

typedef freelist_node_t *freelist_t;

void freelist_push(freelist_t *list, void *block);

void *freelist_pop(freelist_t *list);

void *freelist_get(freelist_t list);

bool freelist_empty(freelist_t list);

#endif // #ifndef FREELIST_H
