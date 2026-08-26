// [DXAIT-COMPONENT: dxmemory]
// [DXAIT-SUBSYSTEM: allocation test]
// [DXAIT-TEST: alignment, ownership, statistics, invalid inputs]

#include "dxait/dx_memory_api.h"

#include <cassert>
#include <cstdint>

int main() {
    dx_memory_pool_t* pool = nullptr;
    assert(dx_memory_pool_create(&pool) == 0);
    assert(pool != nullptr);

    dx_memory_desc_t desc{};
    desc.struct_size = sizeof(desc);
    desc.api_version = 1u;
    desc.bytes = 4096u;
    desc.alignment = 256u;
    desc.location = DX_MEMORY_HOST;

    dx_memory_allocation_t* allocation = nullptr;
    assert(dx_memory_alloc(pool, &desc, &allocation) == 0);
    assert(allocation != nullptr);
    assert(dx_memory_data(allocation) != nullptr);
    assert(dx_memory_size(allocation) == desc.bytes);
    assert(reinterpret_cast<uintptr_t>(dx_memory_data(allocation)) % desc.alignment == 0u);

    dx_memory_stats_t stats{};
    stats.struct_size = sizeof(stats);
    assert(dx_memory_get_stats(pool, &stats) == 0);
    assert(stats.allocation_count == 1u);
    assert(stats.live_bytes == desc.bytes);
    assert(stats.peak_bytes == desc.bytes);

    assert(dx_memory_free(pool, allocation) == 0);
    assert(dx_memory_free(pool, allocation) != 0);
    assert(dx_memory_get_stats(pool, &stats) == 0);
    assert(stats.live_bytes == 0u);

    dx_memory_desc_t bad = desc;
    bad.bytes = 0u;
    assert(dx_memory_alloc(pool, &bad, &allocation) != 0);
    bad.bytes = 64u;
    bad.alignment = 3u;
    assert(dx_memory_alloc(pool, &bad, &allocation) != 0);
    assert(dx_memory_alloc(nullptr, &desc, &allocation) != 0);

    dx_memory_pool_destroy(pool);
    return 0;
}
