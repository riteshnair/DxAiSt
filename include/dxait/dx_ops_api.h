#ifndef DXAIT_DX_OPS_API_H
#define DXAIT_DX_OPS_API_H

// [DXAIT-COMPONENT: dxops]
// [DXAIT-SUBSYSTEM: reference operator ABI]
// [DXAIT-ABI: C99]

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXOPS_BUILD)
#    define DXAIT_OPS_API __declspec(dllexport)
#  elif defined(DXAIT_DXOPS_USE)
#    define DXAIT_OPS_API __declspec(dllimport)
#  else
#    define DXAIT_OPS_API
#  endif
#  define DXAIT_OPS_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_OPS_API __attribute__((visibility("default")))
#  else
#    define DXAIT_OPS_API
#  endif
#  define DXAIT_OPS_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

DXAIT_OPS_API int32_t DXAIT_OPS_CALL dx_op_copy_f32(
    const float* input,
    float* output,
    uint64_t count);

DXAIT_OPS_API int32_t DXAIT_OPS_CALL dx_op_fill_f32(
    float* output,
    uint64_t count,
    float value);

// Row-major: C[M,N] = alpha * A[M,K] * B[K,N] + beta * C[M,N].
DXAIT_OPS_API int32_t DXAIT_OPS_CALL dx_op_gemm_f32(
    const float* a,
    const float* b,
    float* c,
    uint64_t m,
    uint64_t n,
    uint64_t k,
    float alpha,
    float beta);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_OPS_API_H */
