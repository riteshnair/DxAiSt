#ifndef DXAIT_DX_MODEL_API_H
#define DXAIT_DX_MODEL_API_H

// [DXAIT-COMPONENT: dxmodel]
// [DXAIT-SUBSYSTEM: model file ABI]
// [DXAIT-ABI: C99]

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXMODEL_BUILD)
#    define DXAIT_MODEL_API __declspec(dllexport)
#  elif defined(DXAIT_DXMODEL_USE)
#    define DXAIT_MODEL_API __declspec(dllimport)
#  else
#    define DXAIT_MODEL_API
#  endif
#  define DXAIT_MODEL_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_MODEL_API __attribute__((visibility("default")))
#  else
#    define DXAIT_MODEL_API
#  endif
#  define DXAIT_MODEL_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dx_model_t dx_model_t;

typedef enum dx_model_format_t {
    DX_MODEL_FORMAT_UNKNOWN = 0,
    DX_MODEL_FORMAT_GGUF = 1,
    DX_MODEL_FORMAT_SAFETENSORS = 2,
    DX_MODEL_FORMAT_ONNX = 3,
    DX_MODEL_FORMAT_GENERIC = 4
} dx_model_format_t;

DXAIT_MODEL_API int32_t DXAIT_MODEL_CALL dx_model_open(
    const char* path,
    dx_model_t** out_model);
DXAIT_MODEL_API void DXAIT_MODEL_CALL dx_model_destroy(dx_model_t* model);
DXAIT_MODEL_API uint32_t DXAIT_MODEL_CALL dx_model_format(const dx_model_t* model);
DXAIT_MODEL_API uint64_t DXAIT_MODEL_CALL dx_model_size(const dx_model_t* model);
DXAIT_MODEL_API int32_t DXAIT_MODEL_CALL dx_model_read(
    dx_model_t* model,
    uint64_t offset,
    void* buffer,
    uint64_t capacity,
    uint64_t* out_bytes_read);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_MODEL_API_H */
