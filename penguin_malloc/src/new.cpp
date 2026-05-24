#include <cstddef>
#include <cstdlib>
#include <new>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>

extern "C" {
#include "buddy.h"
#include "penguin_malloc.h"
}

#ifndef PENGUIN_MALLOC_DISABLE

namespace {

#ifndef PENGUIN_MALLOC_HEAP_SiZE
constexpr size_t kPenguinMallocSize = 128 * 1024 * 1024; // 128 MiB
#define PENGUIN_MALLOC_HEAP_SiZE kPenguinMallocSize
#endif

// ------------------------------------------------------------------
// Global state for the buddy allocator
// ------------------------------------------------------------------
buddy_alloc_t *g_alloc = nullptr; // the buddy allocator instance
std::byte *g_heap_base = nullptr; // mmap’ed region (for cleanup)
pthread_once_t g_init_once = PTHREAD_ONCE_INIT;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

// ------------------------------------------------------------------
// Initialisation function – called exactly once (thread‑safe)
// ------------------------------------------------------------------
static void init_allocator() {
  g_heap_base = reinterpret_cast<std::byte *>(
      mmap(nullptr, PENGUIN_MALLOC_HEAP_SiZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  if (g_heap_base == MAP_FAILED) {
    std::abort();
  }

  g_alloc = buddy_init(g_heap_base, g_heap_base + PENGUIN_MALLOC_HEAP_SiZE);
  penguin_malloc_init(g_alloc);
}

} // namespace

// ------------------------------------------------------------------
// Overridden operators new / delete
// ------------------------------------------------------------------

void *operator new(std::size_t size) {
  std::cout << "JOPA" << std::endl;
  pthread_once(&g_init_once, init_allocator);

  pthread_mutex_lock(&g_mutex);
  void *ptr = penguin_malloc(size);
  pthread_mutex_unlock(&g_mutex);

  if (!ptr) {
    throw std::bad_alloc();
  }
  return ptr;
}

void *operator new[](std::size_t size) {
  std::cout << "JOPA" << std::endl;
  pthread_once(&g_init_once, init_allocator);
  pthread_mutex_lock(&g_mutex);
  void *ptr = penguin_malloc(size);
  pthread_mutex_unlock(&g_mutex);
  if (!ptr) {
    throw std::bad_alloc();
  }
  return ptr;
}

void operator delete(void *ptr) noexcept {
  pthread_mutex_lock(&g_mutex);
  penguin_free(ptr);
  pthread_mutex_unlock(&g_mutex);
}

void operator delete[](void *ptr) noexcept {
  pthread_mutex_lock(&g_mutex);
  penguin_free(ptr);
  pthread_mutex_unlock(&g_mutex);
}

// ------------------------------------------------------------------
// Nothrow variants (optional but recommended)
// ------------------------------------------------------------------
void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  std::cout << "JOPA" << std::endl;
  pthread_once(&g_init_once, init_allocator);
  pthread_mutex_lock(&g_mutex);
  void *ptr = penguin_malloc(size);
  pthread_mutex_unlock(&g_mutex);
  return ptr;
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
  std::cout << "JOPA" << std::endl;
  pthread_once(&g_init_once, init_allocator);
  pthread_mutex_lock(&g_mutex);
  void *ptr = penguin_malloc(size);
  pthread_mutex_unlock(&g_mutex);
  return ptr;
}

void operator delete(void *ptr, const std::nothrow_t &) noexcept {
  pthread_mutex_lock(&g_mutex);
  penguin_free(ptr);
  pthread_mutex_unlock(&g_mutex);
}

void operator delete[](void *ptr, const std::nothrow_t &) noexcept {
  pthread_mutex_lock(&g_mutex);
  penguin_free(ptr);
  pthread_mutex_unlock(&g_mutex);
}

// ------------------------------------------------------------------
// Sized deallocation (C++14 and later)
// ------------------------------------------------------------------
void operator delete(void *ptr, std::size_t) noexcept {
  pthread_mutex_lock(&g_mutex);
  penguin_free(ptr);
  pthread_mutex_unlock(&g_mutex);
}

void operator delete[](void *ptr, std::size_t) noexcept {
  pthread_mutex_lock(&g_mutex);
  penguin_free(ptr);
  pthread_mutex_unlock(&g_mutex);
}

#endif  // #ifndef PENGUIN_MALLOC_DISABLE
