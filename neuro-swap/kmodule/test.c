#include "inference.h"

#include <stdio.h>

#define TEST_SEQ_LEN 5

int main(int argc, char **argv) {
  static const uint8_t tests[][TEST_SEQ_LEN] = {
      {100, 80, 60, 40, 20},
      {100, 20, 80, 40, 20},
      {100, 40, 100, 40, 100},
      {1, 2, 3, 4, 5},
      {5, 4, 3, 2, 1},
  };

  int n_tests = sizeof(tests) / sizeof(tests[0]);

  for (int i = 0; i < n_tests; i++) {
    uint8_t out_seq[TEST_SEQ_LEN];
    LSTMState state = lstm_memorize(tests[i], TEST_SEQ_LEN);
    lstm_recall(state, out_seq, TEST_SEQ_LEN);

    printf("Test %d:\n", i + 1);
    printf("  Input sequence:  ");
    for (int t = 0; t < TEST_SEQ_LEN; t++) {
      printf("%d ", tests[i][t]);
    }
    printf("\n  Predicted seq:   ");
    for (int t = 0; t < TEST_SEQ_LEN; t++) {
      printf("%d ", out_seq[t]);
    }
    printf("\n\n");
  }

  printf("\nWe trained an LSTM to remember the sequence of numbers.\n"
         "Computer science was a mistake.\n\n");
}
