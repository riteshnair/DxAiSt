#ifndef DXAIT_DX_CORE_API_H
#define DXAIT_DX_CORE_API_H

// [DXAIT-COMPONENT: dxcore]
// [DXAIT-SUBSYSTEM: C99 diagnostics ABI]
// [DXAIT-ABI: C99]
//
// The header intentionally avoids windows.h and C++ types. The DLL component
// name is supplied by the caller and controls environment variables of the
// form dx12_<component>_<mode>=1.

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXCORE_BUILD)
#    define DXAIT_CORE_API __declspec(dllexport)
#  elif defined(DXAIT_DXCORE_USE)
#    define DXAIT_CORE_API __declspec(dllimport)
#  else
#    define DXAIT_CORE_API
#  endif
#  define DXAIT_CORE_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_CORE_API __attribute__((visibility("default")))
#  else
#    define DXAIT_CORE_API
#  endif
#  define DXAIT_CORE_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dx_component_logger_t dx_component_logger_t;

typedef enum dx_component_log_mode_t {
    DX_COMPONENT_LOG_NONE = 0u,
    DX_COMPONENT_LOG_DEBUG = 1u << 0u,
    DX_COMPONENT_LOG_TRACE = 1u << 1u,
    DX_COMPONENT_LOG_PERF = 1u << 2u,
    DX_COMPONENT_LOG_DIAGNOSTIC = 1u << 3u
} dx_component_log_mode_t;

typedef enum dx_component_log_level_t {
    DX_COMPONENT_LEVEL_DEBUG = 0,
    DX_COMPONENT_LEVEL_INFO = 1,
    DX_COMPONENT_LEVEL_WARNING = 2,
    DX_COMPONENT_LEVEL_ERROR = 3,
    DX_COMPONENT_LEVEL_TRACE = 4,
    DX_COMPONENT_LEVEL_PERFORMANCE = 5,
    DX_COMPONENT_LEVEL_DIAGNOSTIC = 6
} dx_component_log_level_t;

// Creates a logger and resolves dx12_<component>_<mode> environment values.
DXAIT_CORE_API int32_t DXAIT_CORE_CALL dx_component_logger_create(
    const char* component,
    dx_component_logger_t** out_logger);

DXAIT_CORE_API void DXAIT_CORE_CALL dx_component_logger_destroy(
    dx_component_logger_t* logger);

DXAIT_CORE_API uint32_t DXAIT_CORE_CALL dx_component_logger_modes(
    const dx_component_logger_t* logger);

DXAIT_CORE_API int32_t DXAIT_CORE_CALL dx_component_logger_enabled(
    const dx_component_logger_t* logger,
    uint32_t mode);

// `source_file` may be null. Logging failures are deliberately non-fatal.
DXAIT_CORE_API void DXAIT_CORE_CALL dx_component_logger_write(
    dx_component_logger_t* logger,
    uint32_t level,
    const char* message,
    const char* source_file,
    int32_t source_line);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_CORE_API_H */
