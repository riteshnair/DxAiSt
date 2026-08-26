// [DXAIT-COMPONENT: dxops]
// [DXAIT-SUBSYSTEM: reference operator test]
// [DXAIT-TEST: copy, fill, GEMM, invalid inputs]

#include "dxait/dx_ops_api.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

int main() {
    const float source[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float copied[4]{};
    assert(dx_op_copy_f32(source, copied, 4u) == 0);
    for (int index = 0; index < 4; ++index) {
        assert(copied[index] == source[index]);
    }

    assert(dx_op_fill_f32(copied, 4u, 7.0f) == 0);
    for (float value : copied) {
        assert(value == 7.0f);
        (void) value;
    }

    const float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    const float b[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    float c[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    assert(dx_op_gemm_f32(a, b, c, 2u, 2u, 2u, 1.0f, 0.0f) == 0);
    assert(std::fabs(c[0] - 19.0f) < 1e-6f);
    assert(std::fabs(c[1] - 22.0f) < 1e-6f);
    assert(std::fabs(c[2] - 43.0f) < 1e-6f);
    assert(std::fabs(c[3] - 50.0f) < 1e-6f);

    assert(dx_op_copy_f32(nullptr, copied, 1u) != 0);
    assert(dx_op_fill_f32(nullptr, 1u, 1.0f) != 0);
    assert(dx_op_gemm_f32(a, b, c, 0u, 2u, 2u, 1.0f, 0.0f) != 0);
    assert(dx_op_gemm_f32(a, b, c, (std::numeric_limits<uint64_t>::max)(),
                          2u, 2u, 1.0f, 0.0f) != 0);
    return 0;
}
