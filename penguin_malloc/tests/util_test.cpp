#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include "config.h"
#include "util.h"
}

TEST(MemUtilTest, AlginAddrUpTest) {
  size_t expected = PAGE_SIZE * 12345;
  size_t unaligned = (size_t)expected - 123 % PAGE_SIZE;
  void *aligned = align_addr_to_page_size_up((void *)unaligned);
  EXPECT_EQ((void *)expected, aligned);
}

TEST(MemUtilTest, AlginAddrDownTest) {
  size_t expected = PAGE_SIZE * 12345;
  size_t unaligned = (size_t)expected + 123 % PAGE_SIZE;
  void *aligned = align_addr_to_page_size_down((void *)unaligned);
  EXPECT_EQ((void *)expected, aligned);
}

TEST(MemUtilTest, AlginSizeUpTest) {
  size_t expected = PAGE_SIZE * 12345;
  size_t unaligned = (size_t)expected - 123 % PAGE_SIZE;
  size_t aligned = align_size_to_page_size_up(unaligned);
  EXPECT_EQ(expected, aligned);
}

TEST(MemUtilTest, AlginSizeDownTest) {
  size_t expected = PAGE_SIZE * 12345;
  size_t unaligned = (size_t)expected + 123 % PAGE_SIZE;
  size_t aligned = align_size_to_page_size_down(unaligned);
  EXPECT_EQ(expected, aligned);
}

TEST(MemUtilTest, MaxLowerPowerOf2) {
  EXPECT_EQ(max_lower_power_of_2(10), 8);
  EXPECT_EQ(max_lower_power_of_2(8), 8);
  EXPECT_EQ(max_lower_power_of_2(228), 128);
}

TEST(MemUtilTest, MinUpperPowerOf2) {
  EXPECT_EQ(min_upper_power_of_2(10), 16);
  EXPECT_EQ(min_upper_power_of_2(8), 8);
  EXPECT_EQ(min_upper_power_of_2(228), 256);
}

TEST(MemUtilTest, Log2Upper) {
  EXPECT_EQ(log2_lower(10), 3);
  EXPECT_EQ(log2_lower(8), 3);
  EXPECT_EQ(log2_lower(228), 7);
}

TEST(MemUtilTest, Log2Lower) {
  EXPECT_EQ(log2_upper(10), 4);
  EXPECT_EQ(log2_upper(8), 3);
  EXPECT_EQ(log2_upper(228), 8);
}
