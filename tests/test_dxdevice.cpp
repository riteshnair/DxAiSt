// [DXAIT-COMPONENT: dxdevice]
// [DXAIT-SUBSYSTEM: adapter inventory test]
// [DXAIT-TEST: enumeration, bounds, and CPU fallback]

#include "dxait/dx_device_api.h"

#include <cassert>
#include <cstring>

int main() {
    dx_device_manager_t* manager = nullptr;
    assert(dx_device_manager_create(&manager) == 0);
    assert(manager != nullptr);
    assert(dx_device_manager_count(manager) >= 1u);

    dx_device_info_t info{};
    info.struct_size = sizeof(info);
    assert(dx_device_manager_get_info(manager, 0u, &info) == 0);
    assert(info.api_version == 1u);
    assert(info.name[0] != '\0');

    dx_device_info_t invalid{};
    invalid.struct_size = sizeof(invalid);
    assert(dx_device_manager_get_info(manager, dx_device_manager_count(manager), &invalid) != 0);
    assert(dx_device_manager_get_info(manager, 0u, nullptr) != 0);
    assert(dx_device_manager_get_info(nullptr, 0u, &invalid) != 0);

    dx_execution_device_t* execution = nullptr;
    assert(dx_device_manager_open(manager, 0u, &execution) == 0);
    assert(execution != nullptr);
    assert(dx_execution_device_backend(execution) == info.backend);
#ifdef _WIN32
    if (info.backend == DX_DEVICE_BACKEND_DX12) {
        assert(dx_execution_device_native_handle(execution) != nullptr);
    }
#else
    assert(dx_execution_device_native_handle(execution) == nullptr);
#endif
    dx_execution_device_destroy(execution);
    assert(dx_device_manager_open(nullptr, 0u, &execution) != 0);

    dx_device_manager_destroy(manager);
    return 0;
}
