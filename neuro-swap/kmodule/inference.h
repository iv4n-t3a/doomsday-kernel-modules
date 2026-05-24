#ifndef INFERENCE_H
#define INFERENCE_H

#include "config.h"
#include "weights.h"

#if __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

typedef struct {
  float h[HIDDEN_SIZE];
  float c[HIDDEN_SIZE];
} LSTMState;

void lstm_state_reset(LSTMState *s);

void lstm_state_copy(LSTMState *dst, const LSTMState *src);

void lstm_memorize(LSTMState *, const uint8_t *seq, int len);

void lstm_recall(LSTMState *, uint8_t *seq, int len);

#endif
