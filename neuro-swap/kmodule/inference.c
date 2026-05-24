#include "inference.h"
#include "math.h"
#include "weights.h"

#if __KERNEL__
#include <asm/fpu/api.h>
#include <linux/math.h>
#include <linux/math64.h>
#include <linux/string.h>
#else
#include <stdio.h>
#include <string.h>
#endif

TARGET_SSE static void lstm_step(LSTMState *s, const float *x, float *y) {
  float tmp[HIDDEN_SIZE];
  float gi[HIDDEN_SIZE], gf[HIDDEN_SIZE], gg[HIDDEN_SIZE], go[HIDDEN_SIZE];

  matvec(gi, lstmw_Wi, x, HIDDEN_SIZE, INPUT_SIZE);
  matvec(tmp, lstmw_Ui, s->h, HIDDEN_SIZE, HIDDEN_SIZE);
  vec_add(gi, tmp, HIDDEN_SIZE);
  vec_add(gi, lstmw_bi, HIDDEN_SIZE);
  for (int i = 0; i < HIDDEN_SIZE; i++)
    gi[i] = sigmoid(gi[i]);

  matvec(gf, lstmw_Wf, x, HIDDEN_SIZE, INPUT_SIZE);
  matvec(tmp, lstmw_Uf, s->h, HIDDEN_SIZE, HIDDEN_SIZE);
  vec_add(gf, tmp, HIDDEN_SIZE);
  vec_add(gf, lstmw_bf, HIDDEN_SIZE);
  for (int i = 0; i < HIDDEN_SIZE; i++)
    gf[i] = sigmoid(gf[i]);

  matvec(gg, lstmw_Wg, x, HIDDEN_SIZE, INPUT_SIZE);
  matvec(tmp, lstmw_Ug, s->h, HIDDEN_SIZE, HIDDEN_SIZE);
  vec_add(gg, tmp, HIDDEN_SIZE);
  vec_add(gg, lstmw_bg, HIDDEN_SIZE);
  for (int i = 0; i < HIDDEN_SIZE; i++)
    gg[i] = ktanhf(gg[i]);

  matvec(go, lstmw_Wo, x, HIDDEN_SIZE, INPUT_SIZE);
  matvec(tmp, lstmw_Uo, s->h, HIDDEN_SIZE, HIDDEN_SIZE);
  vec_add(go, tmp, HIDDEN_SIZE);
  vec_add(go, lstmw_bo, HIDDEN_SIZE);
  for (int i = 0; i < HIDDEN_SIZE; i++)
    go[i] = sigmoid(go[i]);

  for (int i = 0; i < HIDDEN_SIZE; i++) {
    s->c[i] = gf[i] * s->c[i] + gi[i] * gg[i];
    float kernel_tanhfv = ktanhf(s->c[i]);
    s->h[i] = go[i] * kernel_tanhfv;
  }

  if (y != NULL) {
    matvec(y, lstmw_Wy, s->h, OUTPUT_SIZE, HIDDEN_SIZE);
    vec_add(y, lstmw_by, OUTPUT_SIZE);
  }
}

TARGET_SSE static void lstm_memorize_sse(LSTMState *state, const uint8_t *seq,
                                         int len) {
  for (int t = 0; t < len; t++) {
    float in[] = {(float)seq[t] / 255.0f};
    lstm_step(state, in, NULL);
  }
}

TARGET_SSE static void lstm_recall_sse(LSTMState *state, uint8_t *seq,
                                       int len) {
  float zero[INPUT_SIZE] = {0.0f};
  for (int t = 0; t < len; t++) {
    float out[OUTPUT_SIZE];
    lstm_step(state, zero, out);
    seq[t] = out[0] * 255.0f;
  }
}

void lstm_state_reset(LSTMState *s) { memset(s, 0, sizeof(LSTMState)); }

void lstm_state_copy(LSTMState *dst, const LSTMState *src) {
  memcpy(dst, src, sizeof(LSTMState));
}

void lstm_memorize(LSTMState *state, const uint8_t *seq, int len) {
  KERNEL_FPU_BEGIN;

  lstm_memorize_sse(state, seq, len);

  KERNEL_FPU_END;
}

void lstm_recall(LSTMState *state, uint8_t *seq, int len) {
  KERNEL_FPU_BEGIN;

  lstm_recall_sse(state, seq, len);

  KERNEL_FPU_END;
}
