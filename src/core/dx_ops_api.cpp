// [DXAIT-COMPONENT: dxops]
// [DXAIT-SUBSYSTEM: reference operator ABI]
// [DXAIT-IMPLEMENTATION: CPU reference copy, fill, and GEMM]

#include "dxait/dx_ops_api.h"
#include "dxait/dx_core_api.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <string>

namespace {

bool matrix_size_ok(uint64_t lhs, uint64_t rhs, uint64_t* result) noexcept {
    if (result == nullptr || (lhs != 0u && rhs > (std::numeric_limits<uint64_t>::max)() / lhs)) {
        return false;
    }
    *result = lhs * rhs;
    return true;
}

void log_perf(const char* message) noexcept {
    dx_component_logger_t* logger = nullptr;
    if (dx_component_logger_create("dxops", &logger) == 0) {
        dx_component_logger_write(logger, DX_COMPONENT_LEVEL_PERFORMANCE,
                                   message, __FILE__, __LINE__);
        dx_component_logger_destroy(logger);
    }
}

} // namespace

extern "C" {

DXAIT_OPS_API int32_t DXAIT_OPS_CALL dx_op_copy_f32(
    const float* input,
    float* output,
    uint64_t count) {
    if (input == nullptr || output == nullptr) {
        return -1;
    }
    if (count > 0u && count > (std::numeric_limits<size_t>::max)() / sizeof(float)) {
        return -2;
    }
    std::memmove(output, input, static_cast<size_t>(count) * sizeof(float));
    return 0;
}

DXAIT_OPS_API int32_t DXAIT_OPS_CALL dx_op_fill_f32(
    float* output,
    uint64_t count,
    float value) {
    if (output == nullptr) {
        return -1;
    }
    for (uint64_t index = 0u; index < count; ++index) {
        output[index] = value;
    }
    return 0;
}

DXAIT_OPS_API int32_t DXAIT_OPS_CALL dx_op_gemm_f32(
    const float* a,
    const float* b,
    float* c,
    uint64_t m,
    uint64_t n,
    uint64_t k,
    float alpha,
    float beta) {
    if (a == nullptr || b == nullptr || c == nullptr || m == 0u || n == 0u || k == 0u) {
        return -1;
    }
    uint64_t mn = 0u;
    uint64_t mk = 0u;
    uint64_t kn = 0u;
    if (!matrix_size_ok(m, n, &mn) || !matrix_size_ok(m, k, &mk) ||
        !matrix_size_ok(k, n, &kn) || mn > (std::numeric_limits<size_t>::max)() / sizeof(float) ||
        mk > (std::numeric_limits<size_t>::max)() / sizeof(float) ||
        kn > (std::numeric_limits<size_t>::max)() / sizeof(float)) {
        return -2;
    }
    for (uint64_t row = 0u; row < m; ++row) {
        for (uint64_t column = 0u; column < n; ++column) {
            float sum = 0.0f;
            for (uint64_t inner = 0u; inner < k; ++inner) {
                sum += a[row * k + inner] * b[inner * n + column];
            }
            c[row * n + column] = alpha * sum + beta * c[row * n + column];
        }
    }
    log_perf("reference_gemm_f32_complete");
    return 0;
}

} // extern "C"
