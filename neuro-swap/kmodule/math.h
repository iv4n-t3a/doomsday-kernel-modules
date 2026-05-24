#ifndef MATH_H
#define MATH_H

#if __KERNEL__

#define KERNEL_FPU_BEGIN kernel_fpu_begin()
#define KERNEL_FPU_END kernel_fpu_end()
#define TARGET_SSE __attribute__((target("sse2")))

#else

#define KERNEL_FPU_BEGIN
#define KERNEL_FPU_END
#define TARGET_SSE

#endif

TARGET_SSE float kexpf(float x);

TARGET_SSE float ktanhf(float x);

TARGET_SSE float sigmoid(float x);

TARGET_SSE void matvec(float *out, const float *A, const float *x, int m,
                       int n);

TARGET_SSE void vec_add(float *out, const float *v, int n);

#endif
