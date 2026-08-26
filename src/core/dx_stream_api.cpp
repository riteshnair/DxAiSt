// [DXAIT-COMPONENT: dxstream]
// [DXAIT-SUBSYSTEM: stream/event ABI]
// [DXAIT-IMPLEMENTATION: host timeline plus optional D3D12 queue/fence path]

#include "dxait/dx_stream_api.h"
#include "dxait/dx_core_api.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <d3d12.h>
#include <windows.h>
#include <wrl/client.h>
#endif

struct dx_stream_t {
    dx_component_logger_t* logger{nullptr};
    dx_stream_kind_t kind{DX_STREAM_HOST};
    mutable std::mutex mutex;
    uint64_t next_sequence{1u};
#ifdef _WIN32
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
#endif

    ~dx_stream_t() {
        dx_component_logger_destroy(logger);
        logger = nullptr;
    }
};

struct dx_event_t {
    uint64_t sequence{0u};
    mutable std::mutex mutex;
    std::condition_variable condition;
    bool complete{false};
#ifdef _WIN32
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    HANDLE wait_handle{nullptr};
#endif

    ~dx_event_t() {
#ifdef _WIN32
        if (wait_handle != nullptr) {
            CloseHandle(wait_handle);
            wait_handle = nullptr;
        }
#endif
    }
};

namespace {

bool valid_kind(uint32_t kind) noexcept {
    return kind <= static_cast<uint32_t>(DX_STREAM_HOST);
}

void log_debug(dx_stream_t* stream, std::string_view message) noexcept {
    std::string owned(message);
    dx_component_logger_write(stream == nullptr ? nullptr : stream->logger,
                               DX_COMPONENT_LEVEL_DEBUG,
                               owned.c_str(),
                               __FILE__,
                               __LINE__);
}

int32_t create_stream_internal(void* native_device,
                               uint32_t kind,
                               dx_stream_t** out_stream) {
    if (out_stream == nullptr || !valid_kind(kind)) {
        return -1;
    }
    *out_stream = nullptr;
    try {
        auto stream = std::make_unique<dx_stream_t>();
        stream->kind = static_cast<dx_stream_kind_t>(kind);
        if (dx_component_logger_create("dxstream", &stream->logger) != 0) {
            return -2;
        }
#ifdef _WIN32
        if (native_device != nullptr && stream->kind != DX_STREAM_HOST) {
            auto* device = static_cast<ID3D12Device*>(native_device);
            stream->device = device;
            D3D12_COMMAND_QUEUE_DESC description{};
            description.Type = stream->kind == DX_STREAM_COPY
                                  ? D3D12_COMMAND_LIST_TYPE_COPY
                                  : D3D12_COMMAND_LIST_TYPE_COMPUTE;
            description.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            description.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            description.NodeMask = 0u;
            const HRESULT result = device->CreateCommandQueue(
                &description, IID_PPV_ARGS(&stream->queue));
            if (FAILED(result)) {
                return -3;
            }
        }
#else
        (void)native_device;
#endif
        log_debug(stream.get(), "stream_created kind=" + std::to_string(kind));
        *out_stream = stream.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -4;
    } catch (...) {
        return -5;
    }
}

} // namespace

extern "C" {

DXAIT_STREAM_API int32_t DXAIT_STREAM_CALL dx_stream_create(
    uint32_t kind,
    dx_stream_t** out_stream) {
    return create_stream_internal(nullptr, kind, out_stream);
}

DXAIT_STREAM_API int32_t DXAIT_STREAM_CALL dx_stream_create_for_native_device(
    void* native_device,
    uint32_t kind,
    dx_stream_t** out_stream) {
    if (native_device == nullptr) {
#ifdef _WIN32
        return -1;
#else
        return create_stream_internal(nullptr, kind, out_stream);
#endif
    }
    return create_stream_internal(native_device, kind, out_stream);
}

DXAIT_STREAM_API void DXAIT_STREAM_CALL dx_stream_destroy(dx_stream_t* stream) {
    delete stream;
}

DXAIT_STREAM_API int32_t DXAIT_STREAM_CALL dx_stream_record_event(
    dx_stream_t* stream,
    dx_event_t** out_event) {
    if (stream == nullptr || out_event == nullptr) {
        return -1;
    }
    *out_event = nullptr;
    try {
        auto event = std::make_unique<dx_event_t>();
        {
            std::lock_guard<std::mutex> lock(stream->mutex);
            event->sequence = stream->next_sequence++;
        }
#ifdef _WIN32
        if (stream->queue != nullptr && stream->device != nullptr) {
            const HRESULT fence_result = stream->device->CreateFence(
                0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&event->fence));
            if (FAILED(fence_result)) {
                return -2;
            }
            event->wait_handle = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            if (event->wait_handle == nullptr) {
                return -3;
            }
            const HRESULT signal_result = stream->queue->Signal(event->fence.Get(), event->sequence);
            if (FAILED(signal_result)) {
                return -4;
            }
        } else {
            std::lock_guard<std::mutex> lock(event->mutex);
            event->complete = true;
        }
#else
        // The portable host implementation has no queued work, so recording
        // is complete immediately. DX12/HIP backends replace this with a
        // fence/event signal while preserving the same public contract.
        {
            std::lock_guard<std::mutex> lock(event->mutex);
            event->complete = true;
        }
#endif
        event->condition.notify_all();
        log_debug(stream, "event_recorded sequence=" + std::to_string(event->sequence));
        *out_event = event.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -5;
    } catch (...) {
        return -6;
    }
}

DXAIT_STREAM_API int32_t DXAIT_STREAM_CALL dx_event_wait(
    dx_event_t* event,
    uint32_t timeout_ms) {
    if (event == nullptr) {
        return -1;
    }
#ifdef _WIN32
    if (event->fence != nullptr && event->wait_handle != nullptr) {
        if (event->fence->GetCompletedValue() >= event->sequence) {
            return 0;
        }
        if (FAILED(event->fence->SetEventOnCompletion(event->sequence, event->wait_handle))) {
            return -2;
        }
        const DWORD timeout = timeout_ms == UINT32_MAX ? INFINITE : timeout_ms;
        const DWORD wait_result = WaitForSingleObject(event->wait_handle, timeout);
        return wait_result == WAIT_OBJECT_0 ? 0 : (wait_result == WAIT_TIMEOUT ? 1 : -3);
    }
#endif
    std::unique_lock<std::mutex> lock(event->mutex);
    if (timeout_ms == UINT32_MAX) {
        event->condition.wait(lock, [event] { return event->complete; });
        return 0;
    }
    const bool completed = event->condition.wait_for(
        lock, std::chrono::milliseconds(timeout_ms), [event] { return event->complete; });
    return completed ? 0 : 1;
}

DXAIT_STREAM_API int32_t DXAIT_STREAM_CALL dx_event_is_complete(
    const dx_event_t* event) {
    if (event == nullptr) {
        return 0;
    }
#ifdef _WIN32
    if (event->fence != nullptr) {
        return event->fence->GetCompletedValue() >= event->sequence ? 1 : 0;
    }
#endif
    std::lock_guard<std::mutex> lock(event->mutex);
    return event->complete ? 1 : 0;
}

DXAIT_STREAM_API uint64_t DXAIT_STREAM_CALL dx_event_sequence(
    const dx_event_t* event) {
    return event == nullptr ? 0u : event->sequence;
}

DXAIT_STREAM_API void DXAIT_STREAM_CALL dx_event_destroy(dx_event_t* event) {
    delete event;
}

} // extern "C"
