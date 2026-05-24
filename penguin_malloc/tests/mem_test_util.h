#ifndef TESTS_KERNEL_MEM_MEM_TEST_UTIL_H
#define TESTS_KERNEL_MEM_MEM_TEST_UTIL_H

#include <cstring>
#include <cstddef>

namespace mem_test_util {

const char kFreeMagic = 'F';
const char kUsedMagic = 'U';

void mark_free(void *mem, size_t size);

void mark_used(void *mem, size_t size);

void check_free(void *mem, size_t size);

void check_used(void *mem, size_t size);

} // namespace mem_test_util

#endif // #ifndef TESTS_KERNEL_MEM_MEM_TEST_UTIL_H
