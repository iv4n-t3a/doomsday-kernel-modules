#include "math.h"
#include "config.h"

#if __KERNEL__
#include <asm/fpu/api.h>
#include <linux/math.h>
#endif

TARGET_SSE float kexpf(float x) {
  double sum = 1.0;
  double term = 1.0;

  for (int i = 1; i <= EXP_TAILOR_POWER; i++) {
    term *= (double)x / i;
    sum += term;
  }

  return (float)sum;
}

TARGET_SSE float ktanhf(float x) {
  float ex = kexpf(x);
  float enx = kexpf(-x);
  return (ex - enx) / (ex + enx);
}

TARGET_SSE float sigmoid(float x) {
  float expfv = kexpf(-x);
  return 1.0f / (1.0f + expfv);
}

TARGET_SSE void matvec(float *out, const float *A, const float *x, int m,
                       int n) {
  for (int i = 0; i < m; i++) {
    out[i] = 0.0f;
    for (int j = 0; j < n; j++)
      out[i] += A[i * n + j] * x[j];
  }
}

TARGET_SSE void vec_add(float *out, const float *v, int n) {
  for (int i = 0; i < n; i++)
    out[i] += v[i];
}
