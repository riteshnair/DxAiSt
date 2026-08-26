// [DXAIT-COMPONENT: dxruntime]
// [DXAIT-SUBSYSTEM: runtime ABI]
// [DXAIT-IMPLEMENTATION: runtime handle, backend selection, and metadata]

#include "dxait/dx_runtime_api.h"
#include "dxait/dx_core_api.h"
#include "dxait/dx_device_api.h"

#include <cstring>
#include <memory>
#include <new>
#include <string>

struct dx_runtime_t {
    dx_component_logger_t* logger{nullptr};
    dx_device_manager_t* devices{nullptr};
    dx_backend_preference_t requested_backend{DX_BACKEND_AUTO};
    dx_backend_preference_t selected_backend{DX_BACKEND_CPU};
    std::string backend_name{"cpu"};

    ~dx_runtime_t() {
        dx_device_manager_destroy(devices);
        devices = nullptr;
        dx_component_logger_destroy(logger);
        logger = nullptr;
    }
};

namespace {

constexpr uint32_t kApiVersion = 1u;

bool valid_backend(uint32_t value) noexcept {
    return value <= static_cast<uint32_t>(DX_BACKEND_CPU);
}

bool has_dx12_device(const dx_device_manager_t* devices) {
    if (devices == nullptr) {
        return false;
    }
    const uint32_t count = dx_device_manager_count(devices);
    for (uint32_t index = 0u; index < count; ++index) {
        dx_device_info_t info{};
        info.struct_size = sizeof(info);
        if (dx_device_manager_get_info(devices, index, &info) == 0 &&
            info.backend == DX_DEVICE_BACKEND_DX12) {
            return true;
        }
    }
    return false;
}

void select_backend(dx_runtime_t& runtime, dx_backend_preference_t preference) {
    runtime.requested_backend = preference;
#ifdef _WIN32
    if (preference == DX_BACKEND_CPU) {
        runtime.selected_backend = DX_BACKEND_CPU;
        runtime.backend_name = "cpu";
    } else if (preference == DX_BACKEND_HIP) {
        runtime.selected_backend = DX_BACKEND_HIP;
        runtime.backend_name = "hip";
    } else if (has_dx12_device(runtime.devices)) {
        runtime.selected_backend = DX_BACKEND_DX12;
        runtime.backend_name = "dx12";
    } else {
        runtime.selected_backend = DX_BACKEND_CPU;
        runtime.backend_name = "cpu";
    }
#else
    // The Linux validation host has no D3D12/HIP runtime. The CPU backend is
    // selected so the ABI and metadata remain testable without fake devices.
    (void)preference;
    runtime.selected_backend = DX_BACKEND_CPU;
    runtime.backend_name = "cpu";
#endif
}

} // namespace

extern "C" {

DXAIT_RUNTIME_API int32_t DXAIT_RUNTIME_CALL dx_runtime_create(
    const dx_runtime_config_t* config,
    dx_runtime_t** out_runtime) {
    if (out_runtime == nullptr) {
        return -1;
    }
    *out_runtime = nullptr;

    const uint32_t requested = config == nullptr ? DX_BACKEND_AUTO
                              : config->backend_preference;
    if (!valid_backend(requested)) {
        return -2;
    }

    try {
        auto runtime = std::make_unique<dx_runtime_t>();
        if (dx_component_logger_create("dxruntime", &runtime->logger) != 0) {
            return -3;
        }
        if (dx_device_manager_create(&runtime->devices) != 0) {
            return -4;
        }
        select_backend(*runtime, static_cast<dx_backend_preference_t>(requested));
        std::string message = "runtime_create selected_backend=" + runtime->backend_name;
        dx_component_logger_write(runtime->logger,
                                   DX_COMPONENT_LEVEL_DEBUG,
                                   message.c_str(),
                                   __FILE__,
                                   __LINE__);
        *out_runtime = runtime.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -5;
    } catch (...) {
        return -6;
    }
}

DXAIT_RUNTIME_API void DXAIT_RUNTIME_CALL dx_runtime_destroy(dx_runtime_t* runtime) {
    delete runtime;
}

DXAIT_RUNTIME_API int32_t DXAIT_RUNTIME_CALL dx_runtime_get_info(
    const dx_runtime_t* runtime,
    dx_runtime_info_t* out_info) {
    if (runtime == nullptr || out_info == nullptr || out_info->struct_size < sizeof(dx_runtime_info_t)) {
        return -1;
    }
    std::memset(out_info, 0, sizeof(*out_info));
    out_info->struct_size = sizeof(*out_info);
    out_info->api_version = kApiVersion;
    out_info->selected_backend = static_cast<uint32_t>(runtime->selected_backend);
    out_info->logger_modes = runtime->logger == nullptr
                                ? DX_COMPONENT_LOG_NONE
                                : dx_component_logger_modes(runtime->logger);
    out_info->device_count = dx_device_manager_count(runtime->devices);
    std::strncpy(out_info->backend_name, runtime->backend_name.c_str(),
                 sizeof(out_info->backend_name) - 1u);
    return 0;
}

DXAIT_RUNTIME_API int32_t DXAIT_RUNTIME_CALL dx_runtime_component_enabled(
    const dx_runtime_t* runtime,
    uint32_t mode) {
    if (runtime == nullptr || runtime->logger == nullptr) {
        return 0;
    }
    return dx_component_logger_enabled(runtime->logger, mode);
}

} // extern "C"
