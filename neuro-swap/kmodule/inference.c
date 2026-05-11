#include <math.h>
#include <stdio.h>
#include <string.h>

#include "inference.h"
#include "weights.h"

static float sigmoid(float x) { return 1.0f / (1.0f + expf(-x)); }

static void matvec(float *out, const float *A, const float *x, int m, int n) {
  for (int i = 0; i < m; i++) {
    out[i] = 0.0f;
    for (int j = 0; j < n; j++)
      out[i] += A[i * n + j] * x[j];
  }
}

static void vec_add(float *out, const float *v, int n) {
  for (int i = 0; i < n; i++)
    out[i] += v[i];
}

void lstm_step(LSTMState *s, const float *x, float *y) {
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
    gg[i] = tanhf(gg[i]);

  matvec(go, lstmw_Wo, x, HIDDEN_SIZE, INPUT_SIZE);
  matvec(tmp, lstmw_Uo, s->h, HIDDEN_SIZE, HIDDEN_SIZE);
  vec_add(go, tmp, HIDDEN_SIZE);
  vec_add(go, lstmw_bo, HIDDEN_SIZE);
  for (int i = 0; i < HIDDEN_SIZE; i++)
    go[i] = sigmoid(go[i]);

  for (int i = 0; i < HIDDEN_SIZE; i++) {
    s->c[i] = gf[i] * s->c[i] + gi[i] * gg[i];
    s->h[i] = go[i] * tanhf(s->c[i]);
  }

  if (y != NULL) {
    matvec(y, lstmw_Wy, s->h, OUTPUT_SIZE, HIDDEN_SIZE);
    vec_add(y, lstmw_by, OUTPUT_SIZE);
  }
}

static void lstm_state_reset(LSTMState *s) { memset(s, 0, sizeof(LSTMState)); }

static void lstm_state_copy(LSTMState *dst, const LSTMState *src) {
  memcpy(dst, src, sizeof(LSTMState));
}

LSTMState lstm_memorize(const uint8_t *seq, int len) {
  LSTMState state;
  lstm_state_reset(&state);

  for (int t = 0; t < len; t++) {
    float in[] = { (float)seq[t] / 255.0f };
    lstm_step(&state, in, NULL);
  }

  return state;
}

void lstm_recall(LSTMState state, uint8_t *seq, int len) {
  LSTMState saved = state;

  float zero[INPUT_SIZE] = {0.0f};
  for (int t = 0; t < len; t++) {
    float out[OUTPUT_SIZE];
    lstm_step(&saved, zero, out);
    seq[t] = out[0] * 255.0f;
  }
}
