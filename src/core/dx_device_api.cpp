// [DXAIT-COMPONENT: dxdevice]
// [DXAIT-SUBSYSTEM: adapter inventory ABI]
// [DXAIT-IMPLEMENTATION: DXGI adapter enumeration and CPU reference fallback]

#include "dxait/dx_device_api.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#endif

struct dx_execution_device_t {
    uint32_t backend{DX_DEVICE_BACKEND_UNKNOWN};
#ifdef _WIN32
    Microsoft::WRL::ComPtr<ID3D12Device> dx12_device;
#endif
};

struct dx_device_manager_t {
    dx_component_logger_t* logger{nullptr};
    std::vector<dx_device_info_t> devices;
#ifdef _WIN32
    std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter4>> adapters;
#endif

    ~dx_device_manager_t() {
        dx_component_logger_destroy(logger);
        logger = nullptr;
    }
};

namespace {

constexpr uint32_t kApiVersion = 1u;

void log_diagnostic(dx_device_manager_t& manager, const std::string& message) noexcept {
    dx_component_logger_write(manager.logger,
                               DX_COMPONENT_LEVEL_DIAGNOSTIC,
                               message.c_str(),
                               __FILE__,
                               __LINE__);
}

void log_debug(dx_device_manager_t& manager, const std::string& message) noexcept {
    dx_component_logger_write(manager.logger,
                               DX_COMPONENT_LEVEL_DEBUG,
                               message.c_str(),
                               __FILE__,
                               __LINE__);
}

void initialize_cpu_info(dx_device_info_t& info) {
    std::memset(&info, 0, sizeof(info));
    info.struct_size = sizeof(info);
    info.api_version = kApiVersion;
    info.backend = DX_DEVICE_BACKEND_CPU;
    info.capabilities = static_cast<uint32_t>(DX_DEVICE_CAP_CPU_REFERENCE);
    info.is_software = 1u;
    constexpr char kName[] = "CPU reference backend";
    std::memcpy(info.name, kName, sizeof(kName));
}

#ifdef _WIN32
void append_dxgi_adapters(dx_device_manager_t& manager) {
    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    HRESULT result = CreateDXGIFactory2(0u, IID_PPV_ARGS(&factory));
    if (FAILED(result)) {
        std::ostringstream message;
        message << "CreateDXGIFactory2 failed hr=0x" << std::hex
                << static_cast<unsigned long>(result);
        log_diagnostic(manager, message.str());
        return;
    }

    for (UINT index = 0u;; ++index) {
        Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter;
        result = factory->EnumAdapterByGpuPreference(index,
                                                       DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                       IID_PPV_ARGS(&adapter));
        if (result == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(result)) {
            log_diagnostic(manager, "EnumAdapterByGpuPreference failed index=" +
                                      std::to_string(index));
            continue;
        }

        DXGI_ADAPTER_DESC3 description{};
        if (FAILED(adapter->GetDesc3(&description))) {
            continue;
        }
        if ((description.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) != 0u) {
            continue;
        }

        dx_device_info_t info{};
        info.struct_size = sizeof(info);
        info.api_version = kApiVersion;
        info.backend = DX_DEVICE_BACKEND_DX12;
        info.vendor_id = description.VendorId;
        info.device_id = description.DeviceId;
        info.capabilities = static_cast<uint32_t>(DX_DEVICE_CAP_COMPUTE | DX_DEVICE_CAP_FP16);
        info.dedicated_video_memory = description.DedicatedVideoMemory;
        info.dedicated_system_memory = description.DedicatedSystemMemory;
        info.shared_system_memory = description.SharedSystemMemory;
        const int converted = WideCharToMultiByte(
            CP_UTF8, 0, description.Description, -1, info.name,
            static_cast<int>(sizeof(info.name)), nullptr, nullptr);
        if (converted <= 0) {
            info.name[0] = '\0';
        }
        manager.devices.push_back(info);
        manager.adapters.push_back(adapter);
    }
}
#endif

} // namespace

extern "C" {

DXAIT_DEVICE_API int32_t DXAIT_DEVICE_CALL dx_device_manager_create(
    dx_device_manager_t** out_manager) {
    if (out_manager == nullptr) {
        return -1;
    }
    *out_manager = nullptr;
    try {
        auto manager = std::make_unique<dx_device_manager_t>();
        // [DXAIT-CROSS-DLL] Only the stable C99 logger ABI crosses DLLs.
        if (dx_component_logger_create("dxdevice", &manager->logger) != 0) {
            return -2;
        }
#ifdef _WIN32
        append_dxgi_adapters(*manager);
#endif
        // The CPU entry is useful for correctness and unsupported operators.
        dx_device_info_t cpu{};
        initialize_cpu_info(cpu);
        manager->devices.push_back(cpu);
        log_debug(*manager, "device_count=" + std::to_string(manager->devices.size()));
        *out_manager = manager.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -3;
    } catch (...) {
        return -4;
    }
}

DXAIT_DEVICE_API void DXAIT_DEVICE_CALL dx_device_manager_destroy(
    dx_device_manager_t* manager) {
    delete manager;
}

DXAIT_DEVICE_API uint32_t DXAIT_DEVICE_CALL dx_device_manager_count(
    const dx_device_manager_t* manager) {
    return manager == nullptr ? 0u : static_cast<uint32_t>(manager->devices.size());
}

DXAIT_DEVICE_API int32_t DXAIT_DEVICE_CALL dx_device_manager_get_info(
    const dx_device_manager_t* manager,
    uint32_t index,
    dx_device_info_t* out_info) {
    if (manager == nullptr || out_info == nullptr || index >= manager->devices.size() ||
        out_info->struct_size < sizeof(dx_device_info_t)) {
        return -1;
    }
    *out_info = manager->devices[index];
    return 0;
}

DXAIT_DEVICE_API int32_t DXAIT_DEVICE_CALL dx_device_manager_open(
    const dx_device_manager_t* manager,
    uint32_t index,
    dx_execution_device_t** out_device) {
    if (manager == nullptr || out_device == nullptr || index >= manager->devices.size()) {
        return -1;
    }
    *out_device = nullptr;
    try {
        const dx_device_info_t& info = manager->devices[index];
        auto device = std::make_unique<dx_execution_device_t>();
        device->backend = info.backend;
#ifdef _WIN32
        if (info.backend == DX_DEVICE_BACKEND_DX12) {
            if (index >= manager->adapters.size() || manager->adapters[index] == nullptr) {
                return -2;
            }
            HRESULT result = D3D12CreateDevice(manager->adapters[index].Get(),
                                                D3D_FEATURE_LEVEL_12_0,
                                                IID_PPV_ARGS(&device->dx12_device));
            if (FAILED(result)) {
                return -2;
            }
        }
#endif
        *out_device = device.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -3;
    } catch (...) {
        return -4;
    }
}

DXAIT_DEVICE_API void DXAIT_DEVICE_CALL dx_execution_device_destroy(
    dx_execution_device_t* device) {
    delete device;
}

DXAIT_DEVICE_API uint32_t DXAIT_DEVICE_CALL dx_execution_device_backend(
    const dx_execution_device_t* device) {
    return device == nullptr ? DX_DEVICE_BACKEND_UNKNOWN : device->backend;
}

DXAIT_DEVICE_API void* DXAIT_DEVICE_CALL dx_execution_device_native_handle(
    const dx_execution_device_t* device) {
#ifdef _WIN32
    return device == nullptr || device->dx12_device == nullptr
               ? nullptr : static_cast<void*>(device->dx12_device.Get());
#else
    (void)device;
    return nullptr;
#endif
}

} // extern "C"
