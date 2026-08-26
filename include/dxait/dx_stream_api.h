#ifndef DXAIT_DX_STREAM_API_H
#define DXAIT_DX_STREAM_API_H

// [DXAIT-COMPONENT: dxstream]
// [DXAIT-SUBSYSTEM: stream/event ABI]
// [DXAIT-ABI: C99]

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXSTREAM_BUILD)
#    define DXAIT_STREAM_API __declspec(dllexport)
#  elif defined(DXAIT_DXSTREAM_USE)
#    define DXAIT_STREAM_API __declspec(dllimport)
#  else
#    define DXAIT_STREAM_API
#  endif
#  define DXAIT_STREAM_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_STREAM_API __attribute__((visibility("default")))
#  else
#    define DXAIT_STREAM_API
#  endif
#  define DXAIT_STREAM_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dx_stream_t dx_stream_t;
typedef struct dx_event_t dx_event_t;

typedef enum dx_stream_kind_t {
    DX_STREAM_COMPUTE = 0,
    DX_STREAM_COPY = 1,
    DX_STREAM_DIRECT = 2,
    DX_STREAM_HOST = 3
} dx_stream_kind_t;

DXAIT_STREAM_API int32_t DXAIT_STREAM_CALL dx_stream_create(
    uint32_t kind,
    dx_stream_t** out_stream);

// [DXAIT-CONTRACT] `native_device` is an ID3D12Device* on Windows. It is
// borrowed for the lifetime of the created stream and is never released by the
// caller through this API.
DXAIT_STREAM_API int32_t DXAIT_STREAM_CALL dx_stream_create_for_native_device(
    void* native_device,
    uint32_t kind,
    dx_stream_t** out_stream);

DXAIT_STREAM_API void DXAIT_STREAM_CALL dx_stream_destroy(dx_stream_t* stream);

DXAIT_STREAM_API int32_t DXAIT_STREAM_CALL dx_stream_record_event(
    dx_stream_t* stream,
    dx_event_t** out_event);

DXAIT_STREAM_API int32_t DXAIT_STREAM_CALL dx_event_wait(
    dx_event_t* event,
    uint32_t timeout_ms);

DXAIT_STREAM_API int32_t DXAIT_STREAM_CALL dx_event_is_complete(
    const dx_event_t* event);

DXAIT_STREAM_API uint64_t DXAIT_STREAM_CALL dx_event_sequence(
    const dx_event_t* event);

DXAIT_STREAM_API void DXAIT_STREAM_CALL dx_event_destroy(dx_event_t* event);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_STREAM_API_H */
