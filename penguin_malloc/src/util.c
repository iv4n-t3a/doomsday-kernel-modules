#include "util.h"
#include "config.h"

void *align_addr_to_page_size_up(void *ptr) {
  return ptr + (PAGE_SIZE - (size_t)ptr % PAGE_SIZE);
}

void *align_addr_to_page_size_down(void *ptr) {
  return ptr - (size_t)ptr % PAGE_SIZE;
}

size_t align_size_to_page_size_up(size_t ptr) {
  return ptr + (PAGE_SIZE - (size_t)ptr % PAGE_SIZE);
}

size_t align_size_to_page_size_down(size_t ptr) {
  return ptr - (size_t)ptr % PAGE_SIZE;
}

size_t min_upper_power_of_2(size_t size) {
  size_t p = 0;

  while ((1ull << p) < size) {
    p += 1;
  }

  return 1ull << p;
}

size_t max_lower_power_of_2(size_t size) {
  size_t p = 0;

  while ((1ull << (p + 1)) <= size) {
    p += 1;
  }

  return 1ull << p;
}

size_t log2_upper(size_t size) {
  size_t p = 0;

  while ((1ull << p) < size) {
    p += 1;
  }

  return p;
}

size_t log2_lower(size_t size) {
  size_t p = 0;

  while ((1ull << (p + 1)) <= size) {
    p += 1;
  }

  return p;
}

size_t pages_count(void *begin, void *end) {
  begin = align_addr_to_page_size_up(begin);
  end = align_addr_to_page_size_down(end);
  return (end - begin) / PAGE_SIZE;
}

size_t dist(void *ptr1, void *ptr2) {
  if (ptr1 > ptr2) {
    return (uint8_t *)ptr1 - (uint8_t *)ptr2;
  } else {
    return (uint8_t *)ptr2 - (uint8_t *)ptr1;
  }
}

size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len]) {
    len++;
  }
  return len;
}

void itos(uint32_t num, char *str) {
  if (num == 0) {
    str[0] = '0';
    str[1] = '\0';
    return;
  }

  uint32_t denum = 1;

  while (num / denum >= 10) {
    denum *= 10;
  }

  size_t i = 0;
  while (denum != 0) {
    str[i] = num / denum % 10 + '0';
    denum /= 10;
    i += 1;
  }
  str[i] = '\0';
}

