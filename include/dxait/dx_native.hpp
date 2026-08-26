#ifndef DXAIT_DX_NATIVE_HPP
#define DXAIT_DX_NATIVE_HPP

// [DXAIT-COMPONENT: dxnative]
// [DXAIT-SUBSYSTEM: C++20 convenience facade]
// [DXAIT-ABI: C++20 over stable C99 DLL contracts]

#include "dx_core_api.h"
#include "dx_device_api.h"
#include "dx_graph_api.h"
#include "dx_io_api.h"
#include "dx_memory_api.h"
#include "dx_runtime_api.h"
#include "dx_stream_api.h"
#include "dx_tensor_api.h"
#include "dx_trace_api.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace dxait {

inline void check(int32_t status, const char* operation) {
    if (status != 0) {
        throw std::runtime_error(std::string(operation) + " failed: " + std::to_string(status));
    }
}

class Runtime final {
public:
    explicit Runtime(dx_backend_preference_t preference = DX_BACKEND_AUTO) {
        dx_runtime_config_t config{};
        config.struct_size = sizeof(config);
        config.api_version = 1u;
        config.backend_preference = preference;
        config.component_name = "dxruntime";
        check(dx_runtime_create(&config, &handle_), "dx_runtime_create");
    }

    ~Runtime() { dx_runtime_destroy(handle_); }
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
    Runtime& operator=(Runtime&& other) noexcept {
        if (this != &other) {
            dx_runtime_destroy(handle_);
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    dx_runtime_info_t info() const {
        dx_runtime_info_t value{};
        value.struct_size = sizeof(value);
        check(dx_runtime_get_info(handle_, &value), "dx_runtime_get_info");
        return value;
    }

    dx_runtime_t* native_handle() const noexcept { return handle_; }

private:
    dx_runtime_t* handle_{nullptr};
};

class TensorDesc final {
public:
    TensorDesc() { dx_tensor_desc_init(&value_); }

    static TensorDesc contiguous(std::vector<uint64_t> shape, dx_tensor_dtype_t dtype) {
        TensorDesc result;
        check(dx_tensor_desc_init_contiguous(&result.value_,
                                             static_cast<uint32_t>(shape.size()),
                                             shape.data(), dtype),
              "dx_tensor_desc_init_contiguous");
        return result;
    }

    const dx_tensor_desc_t& get() const noexcept { return value_; }
    uint64_t bytes() const {
        uint64_t result = 0u;
        check(dx_tensor_required_bytes(&value_, &result), "dx_tensor_required_bytes");
        return result;
    }

private:
    dx_tensor_desc_t value_{};
};

class MemoryPool final {
public:
    MemoryPool() { check(dx_memory_pool_create(&handle_), "dx_memory_pool_create"); }
    ~MemoryPool() { dx_memory_pool_destroy(handle_); }
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    class Allocation final {
    public:
        Allocation() = default;
        Allocation(dx_memory_pool_t* pool, dx_memory_allocation_t* handle)
            : pool_(pool), handle_(handle) {}
        ~Allocation() { reset(); }
        Allocation(const Allocation&) = delete;
        Allocation& operator=(const Allocation&) = delete;
        Allocation(Allocation&& other) noexcept
            : pool_(std::exchange(other.pool_, nullptr)),
              handle_(std::exchange(other.handle_, nullptr)) {}
        Allocation& operator=(Allocation&& other) noexcept {
            if (this != &other) {
                reset();
                pool_ = std::exchange(other.pool_, nullptr);
                handle_ = std::exchange(other.handle_, nullptr);
            }
            return *this;
        }
        void* data() const noexcept { return dx_memory_data(handle_); }
        uint64_t size() const noexcept { return dx_memory_size(handle_); }
        void reset() noexcept {
            if (pool_ != nullptr && handle_ != nullptr) {
                (void)dx_memory_free(pool_, handle_);
            }
            pool_ = nullptr;
            handle_ = nullptr;
        }

    private:
        dx_memory_pool_t* pool_{nullptr};
        dx_memory_allocation_t* handle_{nullptr};
    };

    Allocation allocate(uint64_t bytes, dx_memory_location_t location = DX_MEMORY_HOST) {
        dx_memory_desc_t desc{};
        desc.struct_size = sizeof(desc);
        desc.api_version = 1u;
        desc.bytes = bytes;
        desc.location = location;
        dx_memory_allocation_t* allocation = nullptr;
        check(dx_memory_alloc(handle_, &desc, &allocation), "dx_memory_alloc");
        return Allocation(handle_, allocation);
    }

private:
    dx_memory_pool_t* handle_{nullptr};
};

class Stream final {
public:
    explicit Stream(dx_stream_kind_t kind = DX_STREAM_HOST) {
        check(dx_stream_create(kind, &handle_), "dx_stream_create");
    }
    ~Stream() { dx_stream_destroy(handle_); }
    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;

    class Event final {
    public:
        explicit Event(dx_event_t* handle) : handle_(handle) {}
        ~Event() { dx_event_destroy(handle_); }
        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;
        bool complete() const noexcept { return dx_event_is_complete(handle_) != 0; }
        void wait(uint32_t timeout_ms = UINT32_MAX) {
            check(dx_event_wait(handle_, timeout_ms), "dx_event_wait");
        }

    private:
        dx_event_t* handle_{nullptr};
    };

    Event record() {
        dx_event_t* event = nullptr;
        check(dx_stream_record_event(handle_, &event), "dx_stream_record_event");
        return Event(event);
    }

private:
    dx_stream_t* handle_{nullptr};
};

class TraceSession final {
public:
    explicit TraceSession(const char* name) {
        check(dx_trace_session_create(name, &handle_), "dx_trace_session_create");
    }
    ~TraceSession() { dx_trace_session_destroy(handle_); }
    TraceSession(const TraceSession&) = delete;
    TraceSession& operator=(const TraceSession&) = delete;

    class Span final {
    public:
        explicit Span(dx_trace_span_t* handle) : handle_(handle) {}
        ~Span() { dx_trace_span_destroy(handle_); }
        Span(const Span&) = delete;
        Span& operator=(const Span&) = delete;
        void end() {
            if (handle_ != nullptr) {
                check(dx_trace_end(handle_), "dx_trace_end");
            }
        }

    private:
        dx_trace_span_t* handle_{nullptr};
    };

    Span begin(const char* category, const char* name, uint64_t request_id = 0u) {
        dx_trace_span_t* span = nullptr;
        check(dx_trace_begin(handle_, category, name, request_id, &span), "dx_trace_begin");
        return Span(span);
    }

    void export_json(const char* path) const {
        check(dx_trace_export(handle_, path), "dx_trace_export");
    }

private:
    dx_trace_session_t* handle_{nullptr};
};

} // namespace dxait

#endif /* DXAIT_DX_NATIVE_HPP */
