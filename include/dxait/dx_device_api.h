#ifndef DXAIT_DX_DEVICE_API_H
#define DXAIT_DX_DEVICE_API_H

// [DXAIT-COMPONENT: dxdevice]
// [DXAIT-SUBSYSTEM: adapter inventory ABI]
// [DXAIT-ABI: C99]

#include <stdint.h>
#include "dx_core_api.h"

#if defined(_WIN32)
#  if defined(DXAIT_DXDEVICE_BUILD)
#    define DXAIT_DEVICE_API __declspec(dllexport)
#  elif defined(DXAIT_DXDEVICE_USE)
#    define DXAIT_DEVICE_API __declspec(dllimport)
#  else
#    define DXAIT_DEVICE_API
#  endif
#  define DXAIT_DEVICE_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_DEVICE_API __attribute__((visibility("default")))
#  else
#    define DXAIT_DEVICE_API
#  endif
#  define DXAIT_DEVICE_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dx_device_manager_t dx_device_manager_t;
typedef struct dx_execution_device_t dx_execution_device_t;

typedef enum dx_device_backend_t {
    DX_DEVICE_BACKEND_UNKNOWN = 0,
    DX_DEVICE_BACKEND_DX12 = 1,
    DX_DEVICE_BACKEND_CPU = 2,
    DX_DEVICE_BACKEND_HIP = 3
} dx_device_backend_t;

typedef enum dx_device_capability_t {
    DX_DEVICE_CAP_COMPUTE = 1u << 0u,
    DX_DEVICE_CAP_FP16 = 1u << 1u,
    DX_DEVICE_CAP_BF16 = 1u << 2u,
    DX_DEVICE_CAP_WAVE_OPS = 1u << 3u,
    DX_DEVICE_CAP_SM67 = 1u << 4u,
    DX_DEVICE_CAP_REBAR = 1u << 5u,
    DX_DEVICE_CAP_DIRECTSTORAGE = 1u << 6u,
    DX_DEVICE_CAP_CPU_REFERENCE = 1u << 31u
} dx_device_capability_t;

typedef struct dx_device_info_t {
    uint32_t struct_size;
    uint32_t api_version;
    uint32_t backend;
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t capabilities;
    uint32_t is_software;
    uint64_t dedicated_video_memory;
    uint64_t dedicated_system_memory;
    uint64_t shared_system_memory;
    char name[128];
} dx_device_info_t;

DXAIT_DEVICE_API int32_t DXAIT_DEVICE_CALL dx_device_manager_create(
    dx_device_manager_t** out_manager);

DXAIT_DEVICE_API void DXAIT_DEVICE_CALL dx_device_manager_destroy(
    dx_device_manager_t* manager);

DXAIT_DEVICE_API uint32_t DXAIT_DEVICE_CALL dx_device_manager_count(
    const dx_device_manager_t* manager);

DXAIT_DEVICE_API int32_t DXAIT_DEVICE_CALL dx_device_manager_get_info(
    const dx_device_manager_t* manager,
    uint32_t index,
    dx_device_info_t* out_info);

// [DXAIT-CONTRACT] Creates a backend execution handle for an enumerated device.
// On Windows this opens a native D3D12 device for DX12 adapters. On the
// portable host the CPU entry creates a valid reference execution handle.
DXAIT_DEVICE_API int32_t DXAIT_DEVICE_CALL dx_device_manager_open(
    const dx_device_manager_t* manager,
    uint32_t index,
    dx_execution_device_t** out_device);

DXAIT_DEVICE_API void DXAIT_DEVICE_CALL dx_execution_device_destroy(
    dx_execution_device_t* device);

DXAIT_DEVICE_API uint32_t DXAIT_DEVICE_CALL dx_execution_device_backend(
    const dx_execution_device_t* device);

DXAIT_DEVICE_API void* DXAIT_DEVICE_CALL dx_execution_device_native_handle(
    const dx_execution_device_t* device);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_DEVICE_API_H */
