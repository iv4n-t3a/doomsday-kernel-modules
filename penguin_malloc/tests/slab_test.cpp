#include <random>
#include <cstring>

#include <gtest/gtest.h>

#include "mem_test_util.h"

extern "C" {
#include "config.h"
#include "buddy.h"
#include "slab.h"
}

class SlabTest : public ::testing::TestWithParam<size_t> {
protected:
  static const size_t kPoolSize = 100 * PAGE_SIZE;

  void SetUp() override {
    mem_test_util::mark_free(mem_.data(), kPoolSize);
    buddy_ = buddy_init(mem_.data(), mem_.data() + kPoolSize);
    slab_ = slab_init(buddy_, GetParam());
    EXPECT_NE(slab_, nullptr);
  }

  void TearDown() override { slab_destroy(slab_); }

  slab_alloc_t *alloc() {
    EXPECT_NE(slab_, nullptr);
    return slab_;
  }

private:
  std::array<std::byte, kPoolSize> mem_;
  buddy_alloc_t *buddy_;
  slab_alloc_t *slab_;
};

TEST_P(SlabTest, Alloc) {
  for (size_t i = 0; i < kPoolSize / GetParam() / 100; ++i) {
    void *ptr = slab_alloc(alloc());
    EXPECT_NE(ptr, nullptr);
    mem_test_util::check_free(ptr, GetParam());
    mem_test_util::mark_used(ptr, GetParam());
  }
}

TEST_P(SlabTest, Free) {
  for (size_t i = 0; i < kPoolSize / GetParam() * 2; ++i) {
    void *ptr = slab_alloc(alloc());
    EXPECT_NE(ptr, nullptr);
    mem_test_util::check_free(ptr, GetParam());
    slab_free(alloc(), ptr);
  }
}

TEST_P(SlabTest, Randomized) {
  std::vector<void *> pointers;

  std::random_device rd;
  std::mt19937 gen(rd());

  for (size_t i = 0; i < kPoolSize / GetParam() / 10; ++i) {
    if (gen() % 2) {
      void *ptr = slab_alloc(alloc());
      EXPECT_NE(ptr, nullptr);
      mem_test_util::check_free(ptr, GetParam());
      mem_test_util::mark_used(ptr, GetParam());
      pointers.push_back(ptr);
    } else if (!pointers.empty()) {
      auto ptr_iter = pointers.begin() + gen() % pointers.size();
      void *ptr = *ptr_iter;
      mem_test_util::mark_free(ptr, GetParam());
      slab_free(alloc(), ptr);
      pointers.erase(ptr_iter);
    }
  }
}

TEST_P(SlabTest, RandomizedSeries) {
  std::vector<void *> pointers;

  std::random_device rd;
  std::mt19937 gen(rd());

  int cmd = gen() % 2;

  for (size_t i = 0; i < kPoolSize / GetParam() / 10; ++i) {
    if (i % 100 == 0) {
      cmd = gen() % 2;
    }
    
    if (cmd) {
      void *ptr = slab_alloc(alloc());
      EXPECT_NE(ptr, nullptr);
      mem_test_util::check_free(ptr, GetParam());
      mem_test_util::mark_used(ptr, GetParam());
      pointers.push_back(ptr);
    } else if (!pointers.empty()) {
      auto ptr_iter = pointers.begin() + gen() % pointers.size();
      void *ptr = *ptr_iter;
      mem_test_util::mark_free(ptr, GetParam());
      slab_free(alloc(), ptr);
      pointers.erase(ptr_iter);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(VaryingObjectSizes, SlabTest,
                         ::testing::Values(1, 2, 3, 4, 10, 16, 100,
                                           PAGE_SIZE *SLAB_MIN_CACHE_PAGES / 2,
                                           PAGE_SIZE *SLAB_MIN_CACHE_PAGES,
                                           PAGE_SIZE *SLAB_MIN_CACHE_PAGES *
                                               2));
