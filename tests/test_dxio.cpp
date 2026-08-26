// [DXAIT-COMPONENT: dxio]
// [DXAIT-SUBSYSTEM: I/O provider test]
// [DXAIT-TEST: fallback selection, ranged reads, and error handling]

#include "dxait/dx_io_api.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>

int main() {
    const char* path = "dxio_test.bin";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "abcdef";
    }

    dx_io_provider_t* provider = nullptr;
    assert(dx_io_provider_create(DX_IO_PROVIDER_DIRECTSTORAGE, &provider) == 0);
    assert(provider != nullptr);
#ifdef _WIN32
    assert(dx_io_provider_kind(provider) == DX_IO_PROVIDER_WIN32 ||
           dx_io_provider_kind(provider) == DX_IO_PROVIDER_DIRECTSTORAGE);
#else
    assert(dx_io_provider_kind(provider) == DX_IO_PROVIDER_PORTABLE);
#endif

    char buffer[8]{};
    uint64_t bytes = 0u;
    assert(dx_io_read_file(provider, path, 1u, buffer, 3u, &bytes) == 0);
    assert(bytes == 3u && std::memcmp(buffer, "bcd", 3u) == 0);
    assert(dx_io_read_file(provider, path, 4u, buffer, 8u, &bytes) == 0);
    assert(bytes == 2u && std::memcmp(buffer, "ef", 2u) == 0);
    assert(dx_io_read_file(provider, path, 99u, buffer, 8u, &bytes) == 1);
    assert(bytes == 0u);
    assert(dx_io_read_file(provider, nullptr, 0u, buffer, 1u, &bytes) != 0);
    assert(dx_io_read_file(provider, path, 0u, buffer, 0u, &bytes) != 0);
    dx_io_provider_destroy(provider);
    std::remove(path);
    return 0;
}
