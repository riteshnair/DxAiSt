#ifndef DXAIT_DX_TRACE_API_H
#define DXAIT_DX_TRACE_API_H

// [DXAIT-COMPONENT: dxtrace]
// [DXAIT-SUBSYSTEM: correlated trace ABI]
// [DXAIT-ABI: C99]

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXTRACE_BUILD)
#    define DXAIT_TRACE_API __declspec(dllexport)
#  elif defined(DXAIT_DXTRACE_USE)
#    define DXAIT_TRACE_API __declspec(dllimport)
#  else
#    define DXAIT_TRACE_API
#  endif
#  define DXAIT_TRACE_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_TRACE_API __attribute__((visibility("default")))
#  else
#    define DXAIT_TRACE_API
#  endif
#  define DXAIT_TRACE_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dx_trace_session_t dx_trace_session_t;
typedef struct dx_trace_span_t dx_trace_span_t;

DXAIT_TRACE_API int32_t DXAIT_TRACE_CALL dx_trace_session_create(
    const char* name,
    dx_trace_session_t** out_session);
DXAIT_TRACE_API void DXAIT_TRACE_CALL dx_trace_session_destroy(
    dx_trace_session_t* session);
DXAIT_TRACE_API int32_t DXAIT_TRACE_CALL dx_trace_begin(
    dx_trace_session_t* session,
    const char* category,
    const char* name,
    uint64_t request_id,
    dx_trace_span_t** out_span);
DXAIT_TRACE_API int32_t DXAIT_TRACE_CALL dx_trace_end(
    dx_trace_span_t* span);
DXAIT_TRACE_API void DXAIT_TRACE_CALL dx_trace_span_destroy(
    dx_trace_span_t* span);
DXAIT_TRACE_API int32_t DXAIT_TRACE_CALL dx_trace_export(
    const dx_trace_session_t* session,
    const char* path);
DXAIT_TRACE_API uint64_t DXAIT_TRACE_CALL dx_trace_event_count(
    const dx_trace_session_t* session);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_TRACE_API_H */
