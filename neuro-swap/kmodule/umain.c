#if __KERNEL__
#error "You're trying to user-space test for kernel"
#endif

#include "inference.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TEST_SEQ_LEN 5

static const uint8_t tests[][TEST_SEQ_LEN] = {
    {100, 80, 60, 40, 20}, {100, 20, 80, 40, 20}, {100, 40, 100, 40, 100},
    {1, 2, 3, 4, 5},       {5, 4, 3, 2, 1},
};

static const int n_tests = sizeof(tests) / sizeof(tests[0]);

static void run_userspace_tests() {
  for (int i = 0; i < n_tests; i++) {
    const uint8_t *input = tests[i];
    uint8_t out_seq[TEST_SEQ_LEN];
    LSTMState state;
    lstm_state_reset(&state);
    lstm_memorize(&state, input, TEST_SEQ_LEN);
    lstm_recall(&state, out_seq, TEST_SEQ_LEN);

    printf("Test %d (direct):\n", i);
    printf("  Input sequence:  ");
    for (int t = 0; t < TEST_SEQ_LEN; t++)
      printf("%d ", input[t]);
    printf("\n  Predicted seq:   ");
    for (int t = 0; t < TEST_SEQ_LEN; t++)
      printf("%d ", out_seq[t]);
    printf("\n\n");
  }
}

static void run_device_tests() {
  int fd = open("/dev/nswap", O_RDWR);

  if (fd < 0) {
    printf("Failed to open /dev/nswap. Neuro-swap module is likely not loaded");
    return;
  }

  for (int i = 0; i < n_tests; i++) {
    const uint8_t *input = tests[i];
    uint8_t out_seq[TEST_SEQ_LEN];
    ssize_t ret;

    ret = write(fd, input, TEST_SEQ_LEN);

    if (ret != TEST_SEQ_LEN) {
      fprintf(stderr, "  Device write failed for test %d: %s\n", i,
              strerror(errno));
      return;
    }

    ret = read(fd, out_seq, TEST_SEQ_LEN);
    if (ret != TEST_SEQ_LEN) {
      fprintf(stderr, "  Device read failed for test %d: %s\n", i,
              strerror(errno));
      return;
    }

    printf("Test %d (via /dev/nswap):\n", i);
    printf("  Input sequence:  ");
    for (int t = 0; t < TEST_SEQ_LEN; t++)
      printf("%d ", input[t]);
    printf("\n  Recalled seq:    ");
    for (int t = 0; t < TEST_SEQ_LEN; t++)
      printf("%d ", out_seq[t]);
    printf("\n\n");
  }

  close(fd);
}

int main(int argc, char **argv) {
  // This message is not written by AI btw
  printf("\n=== Tests through lstm calls from user-space ===\n\n");

  run_userspace_tests();

  printf("\n=== Tests through /dev/nswap ===\n\n");

  run_device_tests();

  // This one actualy is
  printf("\nWe trained an LSTM to remember the sequence of numbers.\n"
         "Computer science was a mistake.\n\n");
}
