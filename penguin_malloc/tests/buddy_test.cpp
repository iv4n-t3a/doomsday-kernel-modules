#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <random>
#include <unordered_set>

#include <gtest/gtest.h>

#include "mem_test_util.h"

extern "C" {
#include "config.h"
#include "buddy.h"
}

class BuddyTestWithVaryingPoolSize : public ::testing::TestWithParam<size_t> {
protected:
  void SetUp() override {
    mem_ = std::unique_ptr<std::byte[]>(new std::byte[GetParam() * PAGE_SIZE]);
    mem_test_util::mark_free(mem_begin(), mem_size());
  }

  void *mem_begin() { return mem_.get(); }
  void *mem_end() { return mem_.get() + mem_size(); }
  size_t mem_size() { return GetParam() * PAGE_SIZE; }

private:
  std::unique_ptr<std::byte[]> mem_;
};

TEST_P(BuddyTestWithVaryingPoolSize, AllocFree) {
  std::vector<size_t> allocations = {1, 2,   1, 10, 23, 11, 7,
                                     8, 109, 4, 42, 1,  1,  1};
  std::unordered_set<void *> pointers;

  buddy_alloc_t *alloc = buddy_init(mem_begin(), mem_end());

  if (alloc == NULL) {
    return;
  }

  for (size_t pages : allocations) {
    void *ptr = buddy_alloc(alloc, pages);
    if (ptr != NULL) {
      mem_test_util::check_free(ptr, pages * PAGE_SIZE);
      mem_test_util::mark_used(ptr, pages * PAGE_SIZE);
      EXPECT_FALSE(pointers.contains(ptr));
      pointers.insert(ptr);
    }
  }

  for (void *ptr : pointers) {
    buddy_free(alloc, ptr);
  }
}

INSTANTIATE_TEST_SUITE_P(VaryingPoolSizes, BuddyTestWithVaryingPoolSize,
                         ::testing::Values(1, 2, 3, 4, 10, 100, 256, 999,
                                           1234));

TEST(BuddyTestWithFixedPoolSize, AllocFree) {
  size_t kPoolSize = 3000 * PAGE_SIZE;
  std::unique_ptr<std::byte[]> pool(new std::byte[kPoolSize]);
  EXPECT_NE(pool.get(), nullptr);

  buddy_alloc_t *alloc = buddy_init(pool.get(), pool.get() + kPoolSize);
  EXPECT_NE(alloc, nullptr);

  // Add some fragmentation
  EXPECT_NE(buddy_alloc(alloc, 1), nullptr);
  EXPECT_NE(buddy_alloc(alloc, 8), nullptr);
  EXPECT_NE(buddy_alloc(alloc, 4), nullptr);
  EXPECT_NE(buddy_alloc(alloc, 32), nullptr);

  std::random_device rd;
  std::mt19937 gen(rd());

  for (size_t i = 0; i < 100000; ++i) {
    void *ptr1 = buddy_alloc(alloc, 1 + gen() % 32);
    void *ptr2 = buddy_alloc(alloc, 1 + gen() % 32);
    void *ptr3 = buddy_alloc(alloc, 1 + gen() % 32);

    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_NE(ptr3, nullptr);

    buddy_free(alloc, ptr1);
    buddy_free(alloc, ptr2);
    buddy_free(alloc, ptr3);
  }
}
