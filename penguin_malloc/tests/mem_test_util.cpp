#include "mem_test_util.h"
#include <cstring>

#include <gtest/gtest.h>

namespace mem_test_util {

void mark_free(void *mem, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    *(char *)mem = kFreeMagic;
  }
}

void mark_used(void *mem, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    *(char *)mem = kUsedMagic;
  }
}

void check_free(void *mem, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    // kFreeMagic may be overwritten by allocator metadata
    if (*(char *)mem == kUsedMagic) {
      FAIL() << "Memory " << mem << " supposed to be free!" << std::endl;
      return;
    }
  }
}

void check_used(void *mem, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    if (*(char *)mem != kUsedMagic) {
      FAIL() << "Memory " << mem << " supposed to be used!" << std::endl;
      return;
    }
  }
}

} // namespace mem_test_util
