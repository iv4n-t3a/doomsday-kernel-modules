#include <gtest/gtest.h>
#include <cstring>
#include <vector>

#include "mem_test_util.h"

extern "C" {
#include "freelist.h"
}

constexpr size_t kNodeSize = 20;

typedef struct {
  char mem[kNodeSize];
} node_t;

TEST(FreelistTest, SanityCheck) {
  size_t size = 10;
  std::vector<node_t> nodes(size);
  freelist_t list = nullptr;

  for (size_t i = 0; i < size; ++i) {
    mem_test_util::mark_free(nodes[i].mem, kNodeSize);

    freelist_push(&list, (void *)&nodes[i]);
    EXPECT_TRUE(list);
  }

  node_t *prev = nullptr;
  for (size_t i = 0; i < size; ++i) {
    EXPECT_TRUE(list);
    node_t *node = (node_t *)freelist_pop(&list);

    EXPECT_NE(node, prev);
    prev = node;

    mem_test_util::check_free(node->mem, kNodeSize);
    mem_test_util::mark_used(node->mem, kNodeSize);
  }

  EXPECT_FALSE(list);
}

TEST(FreelistTest, EmptyList) {
  freelist_t list = nullptr;

  freelist_push(&list, nullptr);
  freelist_push(&list, nullptr);

  EXPECT_EQ(freelist_pop(&list), nullptr);
  EXPECT_EQ(freelist_pop(&list), nullptr);

  freelist_push(&list, nullptr);
  freelist_push(&list, nullptr);

  EXPECT_EQ(freelist_pop(&list), nullptr);
  EXPECT_EQ(freelist_pop(&list), nullptr);
  EXPECT_EQ(freelist_pop(&list), nullptr);
}

TEST(FreelistTest, PassNullptr) {
  freelist_push(nullptr, nullptr);
  EXPECT_EQ(freelist_pop(nullptr), nullptr);
}

TEST(FreelistTest, PushWithInvalidNext) {
  freelist_t list = nullptr;

  freelist_node_t node;
  node.next = (freelist_node_t *)12345;

  freelist_push(&list, &node);
  EXPECT_EQ(node.next, nullptr);

  EXPECT_EQ(freelist_get(list), &node);
  EXPECT_EQ(freelist_pop(&list), &node);
  EXPECT_EQ(freelist_get(list), nullptr);
  EXPECT_EQ(freelist_pop(&list), nullptr);
  EXPECT_EQ(freelist_get(list), nullptr);
  EXPECT_EQ(freelist_pop(&list), nullptr);
}
