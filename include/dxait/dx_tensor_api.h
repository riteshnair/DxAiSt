#ifndef DXAIT_DX_TENSOR_API_H
#define DXAIT_DX_TENSOR_API_H

// [DXAIT-COMPONENT: dxtensor]
// [DXAIT-SUBSYSTEM: tensor descriptor ABI]
// [DXAIT-ABI: C99]

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXTENSOR_BUILD)
#    define DXAIT_TENSOR_API __declspec(dllexport)
#  elif defined(DXAIT_DXTENSOR_USE)
#    define DXAIT_TENSOR_API __declspec(dllimport)
#  else
#    define DXAIT_TENSOR_API
#  endif
#  define DXAIT_TENSOR_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_TENSOR_API __attribute__((visibility("default")))
#  else
#    define DXAIT_TENSOR_API
#  endif
#  define DXAIT_TENSOR_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define DX_TENSOR_API_VERSION 1u
#define DX_TENSOR_MAX_RANK 16u

typedef enum dx_tensor_dtype_t {
    DX_TENSOR_DTYPE_INVALID = 0,
    DX_TENSOR_DTYPE_F32 = 1,
    DX_TENSOR_DTYPE_F16 = 2,
    DX_TENSOR_DTYPE_BF16 = 3,
    DX_TENSOR_DTYPE_I8 = 4,
    DX_TENSOR_DTYPE_U8 = 5,
    DX_TENSOR_DTYPE_I32 = 6,
    DX_TENSOR_DTYPE_I4_PACKED = 7
} dx_tensor_dtype_t;

typedef enum dx_tensor_layout_t {
    DX_TENSOR_LAYOUT_STRIDED = 0,
    DX_TENSOR_LAYOUT_CONTIGUOUS_ROW_MAJOR = 1
} dx_tensor_layout_t;

typedef struct dx_tensor_desc_t {
    uint32_t struct_size;
    uint32_t api_version;
    uint32_t rank;
    uint32_t dtype;
    uint32_t layout;
    uint32_t flags;
    uint64_t shape[DX_TENSOR_MAX_RANK];
    int64_t strides_bytes[DX_TENSOR_MAX_RANK];
    uint64_t byte_offset;
} dx_tensor_desc_t;

DXAIT_TENSOR_API void DXAIT_TENSOR_CALL dx_tensor_desc_init(dx_tensor_desc_t* desc);

DXAIT_TENSOR_API int32_t DXAIT_TENSOR_CALL dx_tensor_desc_init_contiguous(
    dx_tensor_desc_t* desc,
    uint32_t rank,
    const uint64_t* shape,
    uint32_t dtype);

DXAIT_TENSOR_API int32_t DXAIT_TENSOR_CALL dx_tensor_desc_validate(
    const dx_tensor_desc_t* desc);

DXAIT_TENSOR_API int32_t DXAIT_TENSOR_CALL dx_tensor_required_bytes(
    const dx_tensor_desc_t* desc,
    uint64_t* out_bytes);

DXAIT_TENSOR_API int32_t DXAIT_TENSOR_CALL dx_tensor_is_contiguous(
    const dx_tensor_desc_t* desc);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_TENSOR_API_H */
