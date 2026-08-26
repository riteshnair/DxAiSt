#ifndef DXAIT_DX_MEMORY_API_H
#define DXAIT_DX_MEMORY_API_H

// [DXAIT-COMPONENT: dxmemory]
// [DXAIT-SUBSYSTEM: allocation ABI]
// [DXAIT-ABI: C99]

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXMEMORY_BUILD)
#    define DXAIT_MEMORY_API __declspec(dllexport)
#  elif defined(DXAIT_DXMEMORY_USE)
#    define DXAIT_MEMORY_API __declspec(dllimport)
#  else
#    define DXAIT_MEMORY_API
#  endif
#  define DXAIT_MEMORY_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_MEMORY_API __attribute__((visibility("default")))
#  else
#    define DXAIT_MEMORY_API
#  endif
#  define DXAIT_MEMORY_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dx_memory_pool_t dx_memory_pool_t;
typedef struct dx_memory_allocation_t dx_memory_allocation_t;

typedef enum dx_memory_location_t {
    DX_MEMORY_HOST = 0,
    DX_MEMORY_UPLOAD = 1,
    DX_MEMORY_READBACK = 2,
    DX_MEMORY_DEVICE = 3,
    DX_MEMORY_GPU_UPLOAD = 4,
    DX_MEMORY_MANAGED = 5
} dx_memory_location_t;

typedef struct dx_memory_desc_t {
    uint32_t struct_size;
    uint32_t api_version;
    uint64_t bytes;
    uint64_t alignment;
    uint32_t location;
    uint32_t flags;
} dx_memory_desc_t;

typedef struct dx_memory_stats_t {
    uint32_t struct_size;
    uint32_t api_version;
    uint64_t allocation_count;
    uint64_t live_bytes;
    uint64_t peak_bytes;
    uint64_t allocation_failures;
} dx_memory_stats_t;

DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_pool_create(
    dx_memory_pool_t** out_pool);

// [DXAIT-CONTRACT] `native_device` is an ID3D12Device* on Windows. The
// pointer is borrowed by the pool and never released through this API.
DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_pool_create_for_native_device(
    void* native_device,
    dx_memory_pool_t** out_pool);

DXAIT_MEMORY_API void DXAIT_MEMORY_CALL dx_memory_pool_destroy(
    dx_memory_pool_t* pool);

DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_alloc(
    dx_memory_pool_t* pool,
    const dx_memory_desc_t* desc,
    dx_memory_allocation_t** out_allocation);

DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_free(
    dx_memory_pool_t* pool,
    dx_memory_allocation_t* allocation);

DXAIT_MEMORY_API void* DXAIT_MEMORY_CALL dx_memory_data(
    dx_memory_allocation_t* allocation);

DXAIT_MEMORY_API uint64_t DXAIT_MEMORY_CALL dx_memory_size(
    const dx_memory_allocation_t* allocation);

DXAIT_MEMORY_API void* DXAIT_MEMORY_CALL dx_memory_native_resource(
    const dx_memory_allocation_t* allocation);

// Copies between a host pointer and a device allocation. On Windows these use
// temporary upload/readback resources and a synchronized copy queue. On the
// portable reference host, mapped allocations use direct memcpy and device
// allocations are reported unsupported.
DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_upload(
    dx_memory_pool_t* pool,
    dx_memory_allocation_t* destination,
    uint64_t offset,
    const void* source,
    uint64_t bytes);
DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_download(
    dx_memory_pool_t* pool,
    const dx_memory_allocation_t* source,
    uint64_t offset,
    void* destination,
    uint64_t bytes);

DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_get_stats(
    const dx_memory_pool_t* pool,
    dx_memory_stats_t* out_stats);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_MEMORY_API_H */
