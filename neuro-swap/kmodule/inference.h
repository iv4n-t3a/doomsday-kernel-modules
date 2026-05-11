#ifndef INFERENCE_H
#define INFERENCE_H

#include <stdint.h>

#include "weights.h"

typedef struct {
  float h[HIDDEN_SIZE];
  float c[HIDDEN_SIZE];
} LSTMState;

LSTMState lstm_memorize(const uint8_t *seq, int len);

void lstm_recall(LSTMState, uint8_t *seq, int len);

#endif
