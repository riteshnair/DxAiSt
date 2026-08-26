// [DXAIT-COMPONENT: dxtensor]
// [DXAIT-SUBSYSTEM: tensor descriptor test]
// [DXAIT-TEST: validation, overflow, and layout semantics]

#include "dxait/dx_tensor_api.h"

#include <cassert>
#include <cstdint>
#include <limits>

int main() {
    const uint64_t shape[3] = {2u, 3u, 4u};
    dx_tensor_desc_t desc{};
    assert(dx_tensor_desc_init_contiguous(&desc, 3u, shape, DX_TENSOR_DTYPE_F32) == 0);
    assert(dx_tensor_desc_validate(&desc) == 0);
    assert(dx_tensor_is_contiguous(&desc) == 1);

    uint64_t bytes = 0u;
    assert(dx_tensor_required_bytes(&desc, &bytes) == 0);
    assert(bytes == 2u * 3u * 4u * sizeof(float));

    desc.strides_bytes[1] += 4;
    assert(dx_tensor_desc_validate(&desc) == 0);
    assert(dx_tensor_is_contiguous(&desc) == 0);

    dx_tensor_desc_t invalid{};
    dx_tensor_desc_init(&invalid);
    invalid.rank = 1u;
    invalid.dtype = DX_TENSOR_DTYPE_F32;
    invalid.shape[0] = (std::numeric_limits<uint64_t>::max)();
    invalid.strides_bytes[0] = 4;
    assert(dx_tensor_required_bytes(&invalid, &bytes) != 0);

    assert(dx_tensor_desc_init_contiguous(nullptr, 3u, shape, DX_TENSOR_DTYPE_F32) != 0);
    assert(dx_tensor_desc_init_contiguous(&desc, 0u, shape, DX_TENSOR_DTYPE_F32) != 0);
    return 0;
}
