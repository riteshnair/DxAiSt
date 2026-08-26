// [DXAIT-COMPONENT: dxmemory]
// [DXAIT-SUBSYSTEM: allocation ABI]
// [DXAIT-IMPLEMENTATION: portable aligned allocator plus optional D3D12 resources]

#include "dxait/dx_memory_api.h"
#include "dxait/dx_core_api.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <unordered_map>
#include <limits>

#ifdef _WIN32
#include <d3d12.h>
#include <malloc.h>
#include <wrl/client.h>
#endif

struct dx_memory_allocation_t {
    void* data{nullptr};
    uint64_t bytes{0u};
    uint64_t alignment{0u};
    dx_memory_location_t location{DX_MEMORY_HOST};
#ifdef _WIN32
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    D3D12_RESOURCE_STATES state{D3D12_RESOURCE_STATE_COMMON};
#endif
};

struct dx_memory_pool_t {
    dx_component_logger_t* logger{nullptr};
    mutable std::mutex mutex;
    std::unordered_map<dx_memory_allocation_t*, std::unique_ptr<dx_memory_allocation_t>> allocations;
    dx_memory_stats_t stats{};
#ifdef _WIN32
    Microsoft::WRL::ComPtr<ID3D12Device> device;
#endif

    ~dx_memory_pool_t() {
        for (auto& entry : allocations) {
#ifdef _WIN32
            if (entry.second->data != nullptr && entry.second->resource != nullptr) {
                entry.second->resource->Unmap(0u, nullptr);
            }
#endif
            free_aligned(entry.second->data);
        }
        allocations.clear();
        dx_component_logger_destroy(logger);
        logger = nullptr;
    }

    static void free_aligned(void* pointer) noexcept {
#ifdef _WIN32
        _aligned_free(pointer);
#else
        std::free(pointer);
#endif
    }
};

namespace {

constexpr uint32_t kApiVersion = 1u;

bool is_power_of_two(uint64_t value) noexcept {
    return value != 0u && (value & (value - 1u)) == 0u;
}

bool range_valid(uint64_t size, uint64_t offset, uint64_t bytes) noexcept {
    return offset <= size && bytes <= size - offset;
}

void* allocate_aligned(uint64_t bytes, uint64_t alignment) noexcept {
#ifdef _WIN32
    return _aligned_malloc(static_cast<size_t>(bytes), static_cast<size_t>(alignment));
#else
    void* pointer = nullptr;
    if (posix_memalign(&pointer, static_cast<size_t>(alignment), static_cast<size_t>(bytes)) != 0) {
        return nullptr;
    }
    return pointer;
#endif
}

int32_t create_pool_internal(void* native_device, dx_memory_pool_t** out_pool) {
    if (out_pool == nullptr) {
        return -1;
    }
    *out_pool = nullptr;
    try {
        auto pool = std::make_unique<dx_memory_pool_t>();
#ifdef _WIN32
        if (native_device != nullptr) {
            pool->device = static_cast<ID3D12Device*>(native_device);
        }
#else
        (void)native_device;
#endif
        pool->stats.struct_size = sizeof(dx_memory_stats_t);
        pool->stats.api_version = kApiVersion;
        if (dx_component_logger_create("dxmemory", &pool->logger) != 0) {
            return -2;
        }
        *out_pool = pool.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -3;
    } catch (...) {
        return -4;
    }
}

void log_debug(dx_memory_pool_t* pool, const char* message) noexcept {
    dx_component_logger_write(pool == nullptr ? nullptr : pool->logger,
                               DX_COMPONENT_LEVEL_DEBUG,
                               message,
                               __FILE__,
                               __LINE__);
}

#ifdef _WIN32
int32_t allocate_d3d12_resource(dx_memory_pool_t* pool,
                                 const dx_memory_desc_t* desc,
                                 dx_memory_allocation_t& allocation) {
    if (pool->device == nullptr || desc->location == DX_MEMORY_HOST ||
        desc->location == DX_MEMORY_MANAGED) {
        return 1; // caller should use the host path
    }

    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = desc->location == DX_MEMORY_READBACK
                         ? D3D12_HEAP_TYPE_READBACK
                         : (desc->location == DX_MEMORY_UPLOAD || desc->location == DX_MEMORY_GPU_UPLOAD
                                ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT);
    properties.CreationNodeMask = 1u;
    properties.VisibleNodeMask = 1u;

    D3D12_RESOURCE_DESC resource_desc{};
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Width = desc->bytes;
    resource_desc.Height = 1u;
    resource_desc.DepthOrArraySize = 1u;
    resource_desc.MipLevels = 1u;
    resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count = 1u;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    const D3D12_RESOURCE_STATES initial_state =
        properties.Type == D3D12_HEAP_TYPE_DEFAULT
            ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ;
    allocation.state = initial_state;
    HRESULT result = pool->device->CreateCommittedResource(
        &properties, D3D12_HEAP_FLAG_NONE, &resource_desc, initial_state,
        nullptr, IID_PPV_ARGS(&allocation.resource));
    if (FAILED(result)) {
        return -1;
    }
    if (properties.Type != D3D12_HEAP_TYPE_DEFAULT) {
        result = allocation.resource->Map(0u, nullptr, &allocation.data);
        if (FAILED(result)) {
            allocation.resource.Reset();
            allocation.data = nullptr;
            return -2;
        }
    }
    return 0;
}
#endif

#ifdef _WIN32
int32_t execute_buffer_copy(dx_memory_pool_t* pool,
                            dx_memory_allocation_t* allocation,
                            uint64_t allocation_offset,
                            void* host_pointer,
                            uint64_t bytes,
                            bool upload) {
    if (pool == nullptr || allocation == nullptr || allocation->resource == nullptr ||
        host_pointer == nullptr || bytes == 0u || pool->device == nullptr) {
        return -1;
    }
    if (allocation_offset > allocation->bytes || bytes > allocation->bytes - allocation_offset ||
        bytes > static_cast<uint64_t>((std::numeric_limits<SIZE_T>::max)())) {
        return -2;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> staging;
    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = upload ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_READBACK;
    heap.CreationNodeMask = 1u;
    heap.VisibleNodeMask = 1u;
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = bytes;
    desc.Height = 1u;
    desc.DepthOrArraySize = 1u;
    desc.MipLevels = 1u;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1u;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    const D3D12_RESOURCE_STATES staging_state = upload
        ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;
    if (FAILED(pool->device->CreateCommittedResource(
            &heap, D3D12_HEAP_FLAG_NONE, &desc, staging_state, nullptr,
            IID_PPV_ARGS(&staging)))) {
        return -3;
    }

    if (upload) {
        void* mapped = nullptr;
        if (FAILED(staging->Map(0u, nullptr, &mapped)) || mapped == nullptr) {
            return -4;
        }
        std::memcpy(mapped, host_pointer, static_cast<size_t>(bytes));
        staging->Unmap(0u, nullptr);
    }

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    if (FAILED(pool->device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue)))) {
        return -5;
    }
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    if (FAILED(pool->device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&allocator)))) {
        return -6;
    }
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
    if (FAILED(pool->device->CreateCommandList(
            0u, D3D12_COMMAND_LIST_TYPE_COPY, allocator.Get(), nullptr,
            IID_PPV_ARGS(&list)))) {
        return -7;
    }

    const D3D12_RESOURCE_STATES prior_state = allocation->state;
    D3D12_RESOURCE_BARRIER before{};
    before.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    before.Transition.pResource = allocation->resource.Get();
    before.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    before.Transition.StateBefore = prior_state;
    before.Transition.StateAfter = upload ? D3D12_RESOURCE_STATE_COPY_DEST
                                          : D3D12_RESOURCE_STATE_COPY_SOURCE;
    list->ResourceBarrier(1u, &before);
    if (upload) {
        list->CopyBufferRegion(allocation->resource.Get(), allocation_offset,
                               staging.Get(), 0u, bytes);
    } else {
        list->CopyBufferRegion(staging.Get(), 0u, allocation->resource.Get(),
                               allocation_offset, bytes);
    }
    D3D12_RESOURCE_BARRIER after = before;
    after.Transition.StateBefore = before.Transition.StateAfter;
    after.Transition.StateAfter = prior_state;
    list->ResourceBarrier(1u, &after);
    if (FAILED(list->Close())) {
        return -8;
    }
    ID3D12CommandList* lists[] = { list.Get() };
    queue->ExecuteCommandLists(1u, lists);

    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    if (FAILED(pool->device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
        return -9;
    }
    HANDLE event_handle = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (event_handle == nullptr) {
        return -10;
    }
    const uint64_t fence_value = 1u;
    HRESULT result = queue->Signal(fence.Get(), fence_value);
    if (SUCCEEDED(result) && fence->GetCompletedValue() < fence_value) {
        result = fence->SetEventOnCompletion(fence_value, event_handle);
        if (SUCCEEDED(result)) {
            result = WaitForSingleObject(event_handle, INFINITE) == WAIT_OBJECT_0
                         ? S_OK : E_FAIL;
        }
    }
    CloseHandle(event_handle);
    if (FAILED(result)) {
        return -11;
    }

    allocation->state = prior_state;
    if (!upload) {
        const void* mapped = nullptr;
        if (FAILED(staging->Map(0u, nullptr, &mapped)) || mapped == nullptr) {
            return -12;
        }
        std::memcpy(host_pointer, mapped, static_cast<size_t>(bytes));
        staging->Unmap(0u, nullptr);
    }
    return 0;
}
#endif

} // namespace

extern "C" {

DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_pool_create(
    dx_memory_pool_t** out_pool) {
    return create_pool_internal(nullptr, out_pool);
}

DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_pool_create_for_native_device(
    void* native_device,
    dx_memory_pool_t** out_pool) {
#ifdef _WIN32
    if (native_device == nullptr) {
        return -1;
    }
#else
    (void)native_device;
#endif
    return create_pool_internal(native_device, out_pool);
}

DXAIT_MEMORY_API void DXAIT_MEMORY_CALL dx_memory_pool_destroy(
    dx_memory_pool_t* pool) {
    delete pool;
}

DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_alloc(
    dx_memory_pool_t* pool,
    const dx_memory_desc_t* desc,
    dx_memory_allocation_t** out_allocation) {
    if (pool == nullptr || desc == nullptr || out_allocation == nullptr) {
        return -1;
    }
    *out_allocation = nullptr;
    if (desc->struct_size < sizeof(dx_memory_desc_t) || desc->api_version != kApiVersion ||
        desc->bytes == 0u) {
        return -2;
    }
    const uint64_t alignment = desc->alignment == 0u ? 256u : desc->alignment;
    if (!is_power_of_two(alignment) || alignment < sizeof(void*)) {
        return -3;
    }

    try {
        auto allocation = std::make_unique<dx_memory_allocation_t>();
        allocation->bytes = desc->bytes;
        allocation->alignment = alignment;
        allocation->location = static_cast<dx_memory_location_t>(desc->location);
#ifdef _WIN32
        const int32_t d3d12_result = allocate_d3d12_resource(pool, desc, *allocation);
        if (d3d12_result < 0) {
            std::lock_guard<std::mutex> lock(pool->mutex);
            ++pool->stats.allocation_failures;
            return -4;
        }
        const bool native_resource = d3d12_result == 0;
#else
        constexpr bool native_resource = false;
#endif
        if (!native_resource) {
            allocation->data = allocate_aligned(desc->bytes, alignment);
            if (allocation->data == nullptr) {
                std::lock_guard<std::mutex> lock(pool->mutex);
                ++pool->stats.allocation_failures;
                return -5;
            }
        }

        auto* handle = allocation.get();
        {
            std::lock_guard<std::mutex> lock(pool->mutex);
            pool->allocations.emplace(handle, std::move(allocation));
            ++pool->stats.allocation_count;
            pool->stats.live_bytes += desc->bytes;
            pool->stats.peak_bytes = (std::max)(pool->stats.peak_bytes, pool->stats.live_bytes);
        }
        *out_allocation = handle;
        return 0;
    } catch (const std::bad_alloc&) {
        std::lock_guard<std::mutex> lock(pool->mutex);
        ++pool->stats.allocation_failures;
        return -6;
    } catch (...) {
        return -7;
    }
}

DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_free(
    dx_memory_pool_t* pool,
    dx_memory_allocation_t* allocation) {
    if (pool == nullptr || allocation == nullptr) {
        return -1;
    }
    std::unique_ptr<dx_memory_allocation_t> owned;
    {
        std::lock_guard<std::mutex> lock(pool->mutex);
        const auto iterator = pool->allocations.find(allocation);
        if (iterator == pool->allocations.end()) {
            return -2;
        }
        pool->stats.live_bytes -= iterator->second->bytes;
        owned = std::move(iterator->second);
        pool->allocations.erase(iterator);
    }
#ifdef _WIN32
    if (owned->data != nullptr && owned->resource != nullptr) {
        owned->resource->Unmap(0u, nullptr);
    }
#endif
    dx_memory_pool_t::free_aligned(owned->data);
    return 0;
}

DXAIT_MEMORY_API void* DXAIT_MEMORY_CALL dx_memory_data(
    dx_memory_allocation_t* allocation) {
    return allocation == nullptr ? nullptr : allocation->data;
}

DXAIT_MEMORY_API uint64_t DXAIT_MEMORY_CALL dx_memory_size(
    const dx_memory_allocation_t* allocation) {
    return allocation == nullptr ? 0u : allocation->bytes;
}

DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_upload(
    dx_memory_pool_t* pool,
    dx_memory_allocation_t* destination,
    uint64_t offset,
    const void* source,
    uint64_t bytes) {
    if (pool == nullptr || destination == nullptr || source == nullptr ||
        !range_valid(destination->bytes, offset, bytes)) {
        return -1;
    }
    if (destination->data != nullptr) {
        std::memcpy(static_cast<unsigned char*>(destination->data) + offset,
                    source, static_cast<size_t>(bytes));
        return 0;
    }
#ifdef _WIN32
    std::lock_guard<std::mutex> lock(pool->mutex);
    return execute_buffer_copy(pool, destination, offset,
                               const_cast<void*>(source), bytes, true);
#else
    (void)pool;
    return -3;
#endif
}

DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_download(
    dx_memory_pool_t* pool,
    const dx_memory_allocation_t* source,
    uint64_t offset,
    void* destination,
    uint64_t bytes) {
    if (pool == nullptr || source == nullptr || destination == nullptr ||
        !range_valid(source->bytes, offset, bytes)) {
        return -1;
    }
    if (source->data != nullptr) {
        std::memcpy(destination,
                    static_cast<const unsigned char*>(source->data) + offset,
                    static_cast<size_t>(bytes));
        return 0;
    }
#ifdef _WIN32
    std::lock_guard<std::mutex> lock(pool->mutex);
    return execute_buffer_copy(pool, const_cast<dx_memory_allocation_t*>(source), offset,
                               destination, bytes, false);
#else
    (void)pool;
    return -3;
#endif
}

DXAIT_MEMORY_API void* DXAIT_MEMORY_CALL dx_memory_native_resource(
    const dx_memory_allocation_t* allocation) {
#ifdef _WIN32
    return allocation == nullptr || allocation->resource == nullptr
               ? nullptr : static_cast<void*>(allocation->resource.Get());
#else
    (void)allocation;
    return nullptr;
#endif
}

DXAIT_MEMORY_API int32_t DXAIT_MEMORY_CALL dx_memory_get_stats(
    const dx_memory_pool_t* pool,
    dx_memory_stats_t* out_stats) {
    if (pool == nullptr || out_stats == nullptr || out_stats->struct_size < sizeof(dx_memory_stats_t)) {
        return -1;
    }
    std::lock_guard<std::mutex> lock(pool->mutex);
    *out_stats = pool->stats;
    return 0;
}

} // extern "C"
