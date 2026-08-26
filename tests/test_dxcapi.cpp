// [DXAIT-TEST: dxcapi-cxx20]
// [DXAIT-CPU-IMPACT: none]

#include "dxait/dx_c_api.h"

#include <cassert>
#include <cstdint>

int main() {
    static_assert(DXAIT_C_API_VERSION == 1u);
    assert(dx_c_api_version() == 1u);
    assert(dx_device_get_info(nullptr, nullptr) == DX_C_STATUS_INVALID_ARGUMENT);
    assert(dx_device_desc(nullptr, nullptr, 0u) == DX_C_STATUS_INVALID_ARGUMENT);

    dx_device* device = nullptr;
    assert(dx_create_device(0u, &device) == DX_C_STATUS_OK);
    assert(device != nullptr);

    dx_queue* queue = nullptr;
    assert(dx_device_queue(device, &queue) == DX_C_STATUS_OK);
    assert(queue != nullptr);

    const float input[2] = { 3.0f, 4.0f };
    float output[2] = {};
    assert(dx_copy_f32(queue, input, output, 2u) == DX_C_STATUS_OK);
    assert(output[0] == 3.0f && output[1] == 4.0f);

    dx_destroy_queue(queue);
    dx_destroy_device(device);
    return 0;
}
