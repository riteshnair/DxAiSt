// [DXAIT-COMPONENT: dxio]
// [DXAIT-SUBSYSTEM: asynchronous I/O provider ABI]
// [DXAIT-IMPLEMENTATION: provider selection and portable ranged reads]

#include "dxait/dx_io_api.h"
#include "dxait/dx_core_api.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <new>
#include <string>

struct dx_io_provider_t {
    dx_component_logger_t* logger{nullptr};
    dx_io_provider_kind_t kind{DX_IO_PROVIDER_PORTABLE};

    ~dx_io_provider_t() {
        dx_component_logger_destroy(logger);
        logger = nullptr;
    }
};

namespace {

bool valid_kind(uint32_t kind) noexcept {
    return kind <= static_cast<uint32_t>(DX_IO_PROVIDER_PORTABLE);
}

bool valid_path(const char* path) noexcept {
    return path != nullptr && path[0] != '\0';
}

void log_debug(dx_io_provider_t* provider, const std::string& message) noexcept {
    dx_component_logger_write(provider == nullptr ? nullptr : provider->logger,
                               DX_COMPONENT_LEVEL_DEBUG,
                               message.c_str(),
                               __FILE__,
                               __LINE__);
}

} // namespace

extern "C" {

DXAIT_IO_API int32_t DXAIT_IO_CALL dx_io_provider_create(
    uint32_t requested_kind,
    dx_io_provider_t** out_provider) {
    if (out_provider == nullptr || !valid_kind(requested_kind)) {
        return -1;
    }
    *out_provider = nullptr;
    try {
        auto provider = std::make_unique<dx_io_provider_t>();
        if (dx_component_logger_create("dxio", &provider->logger) != 0) {
            return -2;
        }
#ifdef _WIN32
        // DirectStorage is selected only when the build explicitly supplies
        // and enables its SDK/runtime. Otherwise the Win32 provider remains
        // the safe fallback for every Windows installation.
#  if defined(DXAIT_HAS_DIRECTSTORAGE)
        provider->kind = requested_kind == DX_IO_PROVIDER_WIN32
                             ? DX_IO_PROVIDER_WIN32 : DX_IO_PROVIDER_DIRECTSTORAGE;
#  else
        provider->kind = requested_kind == DX_IO_PROVIDER_DIRECTSTORAGE
                             ? DX_IO_PROVIDER_WIN32 : DX_IO_PROVIDER_WIN32;
#  endif
#else
        provider->kind = DX_IO_PROVIDER_PORTABLE;
#endif
        log_debug(provider.get(), "io_provider_kind=" + std::to_string(provider->kind));
        *out_provider = provider.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -3;
    } catch (...) {
        return -4;
    }
}

DXAIT_IO_API void DXAIT_IO_CALL dx_io_provider_destroy(
    dx_io_provider_t* provider) {
    delete provider;
}

DXAIT_IO_API uint32_t DXAIT_IO_CALL dx_io_provider_kind(
    const dx_io_provider_t* provider) {
    return provider == nullptr ? DX_IO_PROVIDER_AUTO : provider->kind;
}

DXAIT_IO_API int32_t DXAIT_IO_CALL dx_io_read_file(
    dx_io_provider_t* provider,
    const char* path,
    uint64_t offset,
    void* buffer,
    uint64_t capacity,
    uint64_t* out_bytes_read) {
    if (provider == nullptr || !valid_path(path) || buffer == nullptr ||
        out_bytes_read == nullptr) {
        return -1;
    }
    *out_bytes_read = 0u;
    if (capacity == 0u) {
        return -2;
    }

    try {
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open()) {
            return -3;
        }
        input.seekg(0, std::ios::end);
        const std::streamoff length = input.tellg();
        if (length < 0 || offset >= static_cast<uint64_t>(length)) {
            return 1;
        }
        input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        const uint64_t available = static_cast<uint64_t>(length) - offset;
        const uint64_t requested = (std::min)(available, capacity);
        input.read(static_cast<char*>(buffer), static_cast<std::streamsize>(requested));
        const std::streamsize read_count = input.gcount();
        *out_bytes_read = read_count > 0 ? static_cast<uint64_t>(read_count) : 0u;
        log_debug(provider, "io_read bytes=" + std::to_string(*out_bytes_read));
        return *out_bytes_read == requested ? 0 : 1;
    } catch (...) {
        return -4;
    }
}

} // extern "C"
