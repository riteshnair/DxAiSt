#ifndef DXAIT_DX_RUNTIME_API_H
#define DXAIT_DX_RUNTIME_API_H

// [DXAIT-COMPONENT: dxruntime]
// [DXAIT-SUBSYSTEM: runtime ABI]
// [DXAIT-ABI: C99]

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXRUNTIME_BUILD)
#    define DXAIT_RUNTIME_API __declspec(dllexport)
#  elif defined(DXAIT_DXRUNTIME_USE)
#    define DXAIT_RUNTIME_API __declspec(dllimport)
#  else
#    define DXAIT_RUNTIME_API
#  endif
#  define DXAIT_RUNTIME_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_RUNTIME_API __attribute__((visibility("default")))
#  else
#    define DXAIT_RUNTIME_API
#  endif
#  define DXAIT_RUNTIME_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dx_runtime_t dx_runtime_t;

typedef enum dx_backend_preference_t {
    DX_BACKEND_AUTO = 0,
    DX_BACKEND_DX12 = 1,
    DX_BACKEND_HIP = 2,
    DX_BACKEND_CPU = 3
} dx_backend_preference_t;

typedef struct dx_runtime_config_t {
    uint32_t struct_size;
    uint32_t api_version;
    uint32_t backend_preference;
    uint32_t flags;
    const char* component_name;
} dx_runtime_config_t;

typedef struct dx_runtime_info_t {
    uint32_t struct_size;
    uint32_t api_version;
    uint32_t selected_backend;
    uint32_t logger_modes;
    uint32_t device_count;
    char backend_name[32];
} dx_runtime_info_t;

// [DXAIT-CONTRACT] `config` may be null; defaults select AUTO and dxruntime.
DXAIT_RUNTIME_API int32_t DXAIT_RUNTIME_CALL dx_runtime_create(
    const dx_runtime_config_t* config,
    dx_runtime_t** out_runtime);

DXAIT_RUNTIME_API void DXAIT_RUNTIME_CALL dx_runtime_destroy(dx_runtime_t* runtime);

DXAIT_RUNTIME_API int32_t DXAIT_RUNTIME_CALL dx_runtime_get_info(
    const dx_runtime_t* runtime,
    dx_runtime_info_t* out_info);

DXAIT_RUNTIME_API int32_t DXAIT_RUNTIME_CALL dx_runtime_component_enabled(
    const dx_runtime_t* runtime,
    uint32_t mode);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_RUNTIME_API_H */
