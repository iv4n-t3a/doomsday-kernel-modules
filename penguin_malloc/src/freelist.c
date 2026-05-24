#include "freelist.h"

freelist_node_t *freelist_get_next(freelist_node_t *node) {
  if (node == NULL) {
    return NULL;
  }
  return node->next;
}

void freelist_set_next(freelist_node_t *node, freelist_node_t *next) {
  if (node != NULL) {
    node->next = next;
  }
}

void freelist_push(freelist_t *list, void *block) {
  if (list == NULL) {
    return;
  }
  freelist_set_next((freelist_t)block, *list);
  *list = (freelist_t)block;
}

void *freelist_pop(freelist_t *list) {
  if (list == NULL) {
    return NULL;
  }

  freelist_t res = *list;
  if (res) {
    *list = freelist_get_next(*list);
  }
  return (void *)res;
}

void *freelist_get(freelist_t list) { return (void *)list; }

bool freelist_empty(freelist_t list) { return list == NULL; }
