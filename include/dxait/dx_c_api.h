#ifndef DXAIT_DX_C_API_H
#define DXAIT_DX_C_API_H

// [DXAIT-COMPONENT: dxcapi]
// [DXAIT-SUBSYSTEM: compatibility C API]
// [DXAIT-ABI: C99]
// [DXAIT-CONTRACT: versioned, opaque handles, explicit ownership]

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXCAPI_BUILD)
#    define DXAIT_C_API __declspec(dllexport)
#  elif defined(DXAIT_DXCAPI_USE)
#    define DXAIT_C_API __declspec(dllimport)
#  else
#    define DXAIT_C_API
#  endif
#  define DXAIT_C_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_C_API __attribute__((visibility("default")))
#  else
#    define DXAIT_C_API
#  endif
#  define DXAIT_C_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define DXAIT_C_API_VERSION 1u

typedef enum dx_c_status_t {
    DX_C_STATUS_OK = 0,
    DX_C_STATUS_INVALID_ARGUMENT = -1,
    DX_C_STATUS_NOT_FOUND = -2,
    DX_C_STATUS_ALLOCATION_FAILED = -3,
    DX_C_STATUS_DEPENDENCY_FAILED = -4,
    DX_C_STATUS_INVALID_STATE = -5,
    DX_C_STATUS_UNSUPPORTED = -6,
    DX_C_STATUS_DEVICE_ERROR = -7,
    DX_C_STATUS_INTERNAL_ERROR = -8
} dx_c_status_t;

typedef enum dx_c_buffer_location_t {
    DX_C_BUFFER_DEFAULT = 1,
    DX_C_BUFFER_UPLOAD = 2,
    DX_C_BUFFER_READBACK = 3
} dx_c_buffer_location_t;

typedef struct dx_device_s dx_device;
typedef struct dx_queue_s dx_queue;
typedef struct dx_buffer_s dx_buffer;

typedef struct dx_c_buffer_desc_t {
    uint32_t struct_size;
    uint32_t api_version;
    uint64_t bytes;
    uint64_t alignment;
    uint32_t location;
    uint32_t flags;
} dx_c_buffer_desc_t;

typedef struct dx_c_device_info_t {
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
} dx_c_device_info_t;

// Returns the ABI version implemented by the loaded compatibility DLL.
DXAIT_C_API uint32_t DXAIT_C_CALL dx_c_api_version(void);

// The caller owns the returned device and releases it with dx_destroy_device.
// The device owns its DxAiSt execution handle and memory pool.
DXAIT_C_API int32_t DXAIT_C_CALL dx_create_device(uint32_t index, dx_device** out_device);
DXAIT_C_API void DXAIT_C_CALL dx_destroy_device(dx_device* device);
DXAIT_C_API int32_t DXAIT_C_CALL dx_device_get_info(
    const dx_device* device,
    dx_c_device_info_t* out_info);

// The caller owns the returned queue and releases it with dx_destroy_queue.
// A queue must not outlive its parent device.
DXAIT_C_API int32_t DXAIT_C_CALL dx_device_queue(
    dx_device* device,
    dx_queue** out_queue);
DXAIT_C_API void DXAIT_C_CALL dx_destroy_queue(dx_queue* queue);

// Source-compatible legacy constructor: location 1=Default, 2=Upload,
// 3=Readback. The descriptor form below carries alignment and flags.
DXAIT_C_API int32_t DXAIT_C_CALL dx_create_buffer(
    dx_device* device,
    uint64_t bytes,
    int location,
    dx_buffer** out_buffer);
DXAIT_C_API int32_t DXAIT_C_CALL dx_create_buffer_ex(
    dx_device* device,
    const dx_c_buffer_desc_t* desc,
    dx_buffer** out_buffer);
DXAIT_C_API void DXAIT_C_CALL dx_destroy_buffer(dx_buffer* buffer);
DXAIT_C_API void* DXAIT_C_CALL dx_buffer_map(dx_buffer* buffer);
DXAIT_C_API void DXAIT_C_CALL dx_buffer_unmap(dx_buffer* buffer);
DXAIT_C_API uint64_t DXAIT_C_CALL dx_buffer_size(const dx_buffer* buffer);
DXAIT_C_API void* DXAIT_C_CALL dx_buffer_native_resource(const dx_buffer* buffer);

// Transfers use direct mapped copies for host-visible allocations. On Windows,
// default buffers use synchronized temporary upload/readback resources; on the
// portable reference host, allocations are host-visible. Unsupported failures
// are reported explicitly through dx_c_status_t.
DXAIT_C_API int32_t DXAIT_C_CALL dx_upload(
    dx_device* device,
    dx_buffer* destination,
    uint64_t offset,
    const void* source,
    uint64_t bytes);
DXAIT_C_API int32_t DXAIT_C_CALL dx_download(
    dx_device* device,
    const dx_buffer* source,
    uint64_t offset,
    void* destination,
    uint64_t bytes);

// These operations call the validated DxAiSt F32 operator ABI. The queue is
// accepted for API stability; this compatibility layer does not yet submit
// asynchronous command-list work.
DXAIT_C_API int32_t DXAIT_C_CALL dx_copy_f32(
    dx_queue* queue,
    const float* input,
    float* output,
    uint64_t count);
DXAIT_C_API int32_t DXAIT_C_CALL dx_fill_f32(
    dx_queue* queue,
    float* output,
    uint64_t count,
    float value);
DXAIT_C_API int32_t DXAIT_C_CALL dx_gemm_f32(
    dx_queue* queue,
    const float* a,
    const float* b,
    float* c,
    uint64_t m,
    uint64_t n,
    uint64_t k,
    float alpha,
    float beta);

// Legacy linear-algebra entry points retained for source and ABI compatibility.
// Operation codes: elementwise 0=add, 1=sub, 2=mul, 3=div; activation
// 0=relu, 1=gelu, 2=silu, 3=tanh, 4=sigmoid, 5=leaky-relu; reduction
// 0=sum, 1=max, 2=min, 3=mean. These use validated buffer transfers and
// capability-safe reference execution until native kernels are wired.
DXAIT_C_API int32_t DXAIT_C_CALL dx_la_elementwise(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* a,
    dx_buffer* b, uint32_t count, int op, float alpha, float beta);
DXAIT_C_API int32_t DXAIT_C_CALL dx_la_activation(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* input,
    uint32_t count, int act, float alpha);
DXAIT_C_API int32_t DXAIT_C_CALL dx_la_rmsnorm(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* input,
    dx_buffer* gamma, uint32_t rows, uint32_t dim, float eps);
DXAIT_C_API int32_t DXAIT_C_CALL dx_la_softmax(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* input,
    uint32_t rows, uint32_t dim);
DXAIT_C_API int32_t DXAIT_C_CALL dx_la_reduce(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* input,
    uint32_t rows, uint32_t dim, int op);
DXAIT_C_API int32_t DXAIT_C_CALL dx_la_gemm_f16_dot2(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* a,
    dx_buffer* b, uint32_t M, uint32_t N, uint32_t K);
DXAIT_C_API int32_t DXAIT_C_CALL dx_la_gemm_f16_wmma(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* a,
    dx_buffer* b, uint32_t M, uint32_t N, uint32_t K);

// No asynchronous work is submitted by the current compatibility operations.
DXAIT_C_API int32_t DXAIT_C_CALL dx_wait(dx_device* device);
DXAIT_C_API int32_t DXAIT_C_CALL dx_device_desc(
    const dx_device* device,
    char* buffer,
    uint32_t capacity);
DXAIT_C_API const char* DXAIT_C_CALL dx_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_C_API_H */
