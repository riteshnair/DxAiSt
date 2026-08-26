#ifndef DXAIT_DX_IO_API_H
#define DXAIT_DX_IO_API_H

// [DXAIT-COMPONENT: dxio]
// [DXAIT-SUBSYSTEM: asynchronous I/O provider ABI]
// [DXAIT-ABI: C99]

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXIO_BUILD)
#    define DXAIT_IO_API __declspec(dllexport)
#  elif defined(DXAIT_DXIO_USE)
#    define DXAIT_IO_API __declspec(dllimport)
#  else
#    define DXAIT_IO_API
#  endif
#  define DXAIT_IO_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_IO_API __attribute__((visibility("default")))
#  else
#    define DXAIT_IO_API
#  endif
#  define DXAIT_IO_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dx_io_provider_t dx_io_provider_t;

typedef enum dx_io_provider_kind_t {
    DX_IO_PROVIDER_AUTO = 0,
    DX_IO_PROVIDER_DIRECTSTORAGE = 1,
    DX_IO_PROVIDER_WIN32 = 2,
    DX_IO_PROVIDER_PORTABLE = 3
} dx_io_provider_kind_t;

DXAIT_IO_API int32_t DXAIT_IO_CALL dx_io_provider_create(
    uint32_t requested_kind,
    dx_io_provider_t** out_provider);
DXAIT_IO_API void DXAIT_IO_CALL dx_io_provider_destroy(
    dx_io_provider_t* provider);
DXAIT_IO_API uint32_t DXAIT_IO_CALL dx_io_provider_kind(
    const dx_io_provider_t* provider);
DXAIT_IO_API int32_t DXAIT_IO_CALL dx_io_read_file(
    dx_io_provider_t* provider,
    const char* path,
    uint64_t offset,
    void* buffer,
    uint64_t capacity,
    uint64_t* out_bytes_read);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_IO_API_H */
