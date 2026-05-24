#include <algorithm>
#include <gtest/gtest.h>
#include <cstring>
#include <random>

extern "C" {
#include "config.h"
#include "buddy_util.h"
}

TEST(BuddyUtilTest, MaskIndex) {
  // [ _0112222 33333333 44444444 44444444 ]
  EXPECT_EQ(mask_index(0, 0), 0);
  EXPECT_EQ(mask_index(2, 2), 0);
  EXPECT_EQ(mask_index(3, 0), 1);
  EXPECT_EQ(mask_index(4, 0), 2);
  EXPECT_EQ(mask_index(4, 9), 3);
}

TEST(BuddyUtilTest, BitIndex) {
  EXPECT_EQ(bit_index(0, 0), 1);
  EXPECT_EQ(bit_index(1, 1), 3);
  EXPECT_EQ(bit_index(3, 0), 0);
  EXPECT_EQ(bit_index(2, 3), 7);
  EXPECT_EQ(bit_index(80, 257), 1);
}

TEST(BuddyUtilTest, BuddyGetFromFirstByte) {
  uint8_t bitmap[] = {0b1000'1010};

  EXPECT_EQ(buddy_get(bitmap, 0, 0), 1);
  EXPECT_EQ(buddy_get(bitmap, 1, 0), 0);
  EXPECT_EQ(buddy_get(bitmap, 1, 1), 1);
  EXPECT_EQ(buddy_get(bitmap, 2, 0), 0);
  EXPECT_EQ(buddy_get(bitmap, 2, 1), 0);
  EXPECT_EQ(buddy_get(bitmap, 2, 2), 0);
  EXPECT_EQ(buddy_get(bitmap, 2, 3), 1);
}

TEST(BuddyUtilTest, BuddyGet) {
  uint8_t bitmap[] = {0b0000'0000, 0b1000'1001};

  EXPECT_EQ(buddy_get(bitmap, 3, 0), 1);
  EXPECT_EQ(buddy_get(bitmap, 3, 1), 0);
  EXPECT_EQ(buddy_get(bitmap, 3, 2), 0);
  EXPECT_EQ(buddy_get(bitmap, 3, 3), 1);
  EXPECT_EQ(buddy_get(bitmap, 3, 4), 0);
  EXPECT_EQ(buddy_get(bitmap, 3, 5), 0);
  EXPECT_EQ(buddy_get(bitmap, 3, 6), 0);
  EXPECT_EQ(buddy_get(bitmap, 3, 7), 1);
}

TEST(BuddyUtilTest, MarkAll) {
  std::random_device rd;
  std::mt19937 gen(rd());

  std::vector<int> order(8);
  std::iota(order.begin(), order.end(), 0);
  std::shuffle(order.begin(), order.end(), gen);

  uint8_t bitmap[] = {0b0000'0000, 0b0000'0000};

  for (size_t i = 0; i < 8; ++i) {
    buddy_mark(bitmap, 3, order[i]);
  }

  EXPECT_EQ(bitmap[0], 0b0000'0010);
  EXPECT_EQ(bitmap[1], 0b0000'0000);
}

TEST(BuddyUtilTest, BestFit) {
  EXPECT_EQ(best_fit_layer(2 * PAGE_SIZE, 2 * PAGE_SIZE), 0);
  EXPECT_EQ(best_fit_layer(1 * PAGE_SIZE, 2 * PAGE_SIZE), 1);
  EXPECT_EQ(best_fit_layer(1 * PAGE_SIZE, 4 * PAGE_SIZE), 2);
  EXPECT_EQ(best_fit_layer(4 * PAGE_SIZE, 16 * PAGE_SIZE), 2);
  EXPECT_EQ(best_fit_layer(2 * PAGE_SIZE, 16 * PAGE_SIZE), 3);
  EXPECT_EQ(best_fit_layer(1 * PAGE_SIZE, 16 * PAGE_SIZE), 4);
  EXPECT_EQ(best_fit_layer(1 * PAGE_SIZE, 11 * PAGE_SIZE), 4);
}

TEST(BuddyUtilTest, BlockSize) {
  EXPECT_EQ(block_size(1, 1024), 512);
  EXPECT_EQ(block_size(0, 1024), 1024);
  EXPECT_EQ(block_size(3, 1024), 128);
}

TEST(BuddyUtilTest, ToIndex) {
  EXPECT_EQ(to_index(0, 0, 0, 100), 0);
  EXPECT_EQ(to_index((void *)1, 0, (void *)1, 1024), 0);
  EXPECT_EQ(to_index((void *)1, 1, (void *)1, 1024), 0);
  EXPECT_EQ(to_index(0, 1, (void *)(512 * PAGE_SIZE), 1024), 1);
  EXPECT_EQ(to_index((void *)100, 2, (void *)(100 + 512 * PAGE_SIZE), 1024), 2);
}

TEST(BuddyUtilTest, ToPointer) {
  EXPECT_EQ(to_pointer(0, 0, 0, 100), nullptr);
  EXPECT_EQ(to_pointer((void *)1, 0, 0, 1024), (void *)1);
  EXPECT_EQ(to_pointer((void *)1, 1, 0, 1024), (void *)1);
  EXPECT_EQ(to_pointer(0, 1, 1, 1024), (void *)(512 * PAGE_SIZE));
  EXPECT_EQ(to_pointer((void *)100, 2, 2, 1024), (void *)(100 + 512 * PAGE_SIZE));
}
