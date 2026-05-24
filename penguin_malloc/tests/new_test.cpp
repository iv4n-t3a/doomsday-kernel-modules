#include <gtest/gtest.h>
#include <new>
#include <tuple>
#include <vector>

#include "mem_test_util.h"

#ifdef PENGUIN_MALLOC_DISABLE
#error "Penguin malloc is disabled for tests"
#endif

class NewTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}
};


TEST_F(NewTest, MallocAndFreeSingle) {
  char* p = new char[64];
  ASSERT_NE(p, nullptr);
  mem_test_util::mark_used(p, 64);
  mem_test_util::check_used(p, 64);
  delete[] p;

  // mem_test_util::mark_free(p, 64);
  // mem_test_util::check_free(p, 64);
}

TEST_F(NewTest, MallocZeroSize) {
  // new char[0] is allowed and returns a non‑null unique pointer
  char* p = new char[0];
  delete[] p;
}

TEST_F(NewTest, MultipleIndependentAllocations) {
  constexpr size_t NUM_ALLOCS = 100;
  char* ptrs[NUM_ALLOCS] = {nullptr};

  for (size_t i = 0; i < NUM_ALLOCS; ++i) {
    ptrs[i] = new char[128];
    ASSERT_NE(ptrs[i], nullptr);
    mem_test_util::mark_used(ptrs[i], 128);
  }

  for (size_t i = 0; i < NUM_ALLOCS; ++i) {
    mem_test_util::check_used(ptrs[i], 128);
    delete[] ptrs[i];
  }
}

TEST_F(NewTest, LargeAllocation) {
  const size_t LARGE_SIZE = 32 * 1024 * 1024; // 32 MiB
  char* p = new char[LARGE_SIZE];
  ASSERT_NE(p, nullptr);

  std::memset(p, 0xAA, LARGE_SIZE);
  delete[] p;
}

TEST_F(NewTest, ReallocateStress) {
  for (int i = 0; i < 1000; ++i) {
    char* p = new char[256];
    ASSERT_NE(p, nullptr);
    mem_test_util::mark_used(p, 256);
    mem_test_util::check_used(p, 256);
    delete[] p;
  }
}

TEST_F(NewTest, OperatorNewScalar) {
  int *p = new int(42);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 42);
  delete p;
}

TEST_F(NewTest, OperatorNewArray) {
  constexpr size_t N = 100;
  int *arr = new int[N];
  ASSERT_NE(arr, nullptr);
  for (size_t i = 0; i < N; ++i) {
    arr[i] = static_cast<int>(i);
  }
  for (size_t i = 0; i < N; ++i) {
    EXPECT_EQ(arr[i], i);
  }
  delete[] arr;
}

TEST_F(NewTest, OperatorNewNothrow) {
  int *p = new (std::nothrow) int(123);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 123);
  delete p;
}

TEST_F(NewTest, OperatorNewArrayNothrow) {
  constexpr size_t N = 50;
  double *arr = new (std::nothrow) double[N];
  ASSERT_NE(arr, nullptr);
  for (size_t i = 0; i < N; ++i) {
    arr[i] = i * 1.5;
  }
  delete[] arr;
}

TEST_F(NewTest, OperatorDeleteWithNullptr) {
  int *p = nullptr;
  delete p;
  delete[] p;
}

TEST_F(NewTest, DISABLED_AllocationFailureThrowsBadAlloc) {
  const size_t HUGE_SIZE =
      1024 * 1024 * 1024; // 1 GiB (adjust to exceed your heap)
  EXPECT_THROW({ std::ignore = ::operator new(HUGE_SIZE); }, std::bad_alloc);
}

TEST_F(NewTest, DISABLED_AllocationFailureNothrowReturnsNull) {
  const size_t HUGE_SIZE = 1024 * 1024 * 1024;
  void *p = ::operator new(HUGE_SIZE, std::nothrow);
  EXPECT_EQ(p, nullptr);
}

TEST_F(NewTest, AllocationAlignment) {
  struct alignas(16) Aligned16 {
    char c;
  };
  Aligned16 *p = new Aligned16();
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % 16, 0U);
  delete p;

  struct alignas(32) Aligned32 {
    double d;
  };
  Aligned32 *arr = new Aligned32[10];
  ASSERT_NE(arr, nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(arr) % 32, 0U);
  delete[] arr;
}

TEST_F(NewTest, StdVectorUsesOverriddenNew) {
  std::vector<int> v;
  for (int i = 0; i < 1000; ++i) {
    v.push_back(i);
  }
  EXPECT_EQ(v.size(), 1000);
  for (size_t i = 0; i < v.size(); ++i) {
    EXPECT_EQ(v[i], i);
  }
}
