#ifndef DXAIT_DX_SHADER_API_H
#define DXAIT_DX_SHADER_API_H

// [DXAIT-COMPONENT: dxshader]
// [DXAIT-SUBSYSTEM: DXC shader ABI]
// [DXAIT-ABI: C99]

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXSHADER_BUILD)
#    define DXAIT_SHADER_API __declspec(dllexport)
#  elif defined(DXAIT_DXSHADER_USE)
#    define DXAIT_SHADER_API __declspec(dllimport)
#  else
#    define DXAIT_SHADER_API
#  endif
#  define DXAIT_SHADER_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_SHADER_API __attribute__((visibility("default")))
#  else
#    define DXAIT_SHADER_API
#  endif
#  define DXAIT_SHADER_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dx_shader_blob_t dx_shader_blob_t;

DXAIT_SHADER_API int32_t DXAIT_SHADER_CALL dx_shader_compile_file(
    const char* source_path,
    const char* entry_point,
    const char* target_profile,
    dx_shader_blob_t** out_blob);
DXAIT_SHADER_API void DXAIT_SHADER_CALL dx_shader_blob_destroy(
    dx_shader_blob_t* blob);
DXAIT_SHADER_API const void* DXAIT_SHADER_CALL dx_shader_blob_data(
    const dx_shader_blob_t* blob);
DXAIT_SHADER_API uint64_t DXAIT_SHADER_CALL dx_shader_blob_size(
    const dx_shader_blob_t* blob);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_SHADER_API_H */
