// [DXAIT-COMPONENT: dxruntime]
// [DXAIT-SUBSYSTEM: runtime ABI test]
// [DXAIT-TEST: backend selection, device inventory, and metadata]

#include "dxait/dx_runtime_api.h"

#include <cassert>
#include <cstring>

int main() {
    dx_runtime_config_t config{};
    config.struct_size = sizeof(config);
    config.api_version = 1u;
    config.backend_preference = DX_BACKEND_AUTO;
    config.component_name = "dxruntime";

    dx_runtime_t* runtime = nullptr;
    assert(dx_runtime_create(&config, &runtime) == 0);
    assert(runtime != nullptr);

    dx_runtime_info_t info{};
    info.struct_size = sizeof(info);
    assert(dx_runtime_get_info(runtime, &info) == 0);
    assert(info.api_version == 1u);
    assert(info.device_count >= 1u);
    assert(std::strlen(info.backend_name) > 0u);
#ifdef _WIN32
    assert(info.selected_backend == DX_BACKEND_DX12 || info.selected_backend == DX_BACKEND_CPU);
#else
    assert(info.selected_backend == DX_BACKEND_CPU);
#endif

    assert(dx_runtime_component_enabled(runtime, 0u) == 0 ||
           dx_runtime_component_enabled(runtime, 0u) == 1);
    dx_runtime_destroy(runtime);

    config.backend_preference = 99u;
    assert(dx_runtime_create(&config, &runtime) != 0);
    assert(runtime == nullptr);
    assert(dx_runtime_create(&config, nullptr) != 0);
    return 0;
}
