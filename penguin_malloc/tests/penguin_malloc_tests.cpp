#include "mem_test_util.h"

#include <array>
#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "config.h"
#include "penguin_malloc.h"
#include "pfalloc.h"
}

class PenguinMallocTest : public ::testing::TestWithParam<size_t> {
protected:
  static const size_t kPagesInPool = 10000;
  static const size_t kPoolSize = kPagesInPool * PAGE_SIZE;

  void SetUp() override {
    mem_test_util::mark_free(mem_.data(), kPoolSize);
    pfalloc_init(mem_.data(), kPoolSize);
    penguin_malloc_init(get_pfalloc_buddy());
  }

private:
  std::array<std::byte, kPoolSize> mem_;
};

void *penguin_malloc_and_check(size_t size) {
  void *mem = penguin_malloc(size);
  EXPECT_NE(mem, nullptr);
  mem_test_util::check_free(mem, size);
  mem_test_util::mark_used(mem, size);
  return mem;
}

TEST_P(PenguinMallocTest, Alloc) {
  void *mem = penguin_malloc_and_check(GetParam());
  penguin_free(mem);
}

TEST_P(PenguinMallocTest, Free) {
  size_t kSize = GetParam();

  for (size_t i = 0; i < 10000; ++i) {
    void *mem = penguin_malloc_and_check(kSize);
    penguin_free(mem);
    mem_test_util::mark_free(mem, kSize);
  }
}

INSTANTIATE_TEST_SUITE_P(VaryingObjectSizes, PenguinMallocTest,
                         ::testing::Values(42, PAGE_SIZE * 2));
