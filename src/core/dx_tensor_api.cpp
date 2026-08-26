// [DXAIT-COMPONENT: dxtensor]
// [DXAIT-SUBSYSTEM: tensor descriptor ABI]
// [DXAIT-IMPLEMENTATION: descriptor validation and layout helpers]

#include "dxait/dx_tensor_api.h"

#include <cstring>
#include <limits>

namespace {

uint64_t dtype_bytes(uint32_t dtype) noexcept {
    switch (dtype) {
    case DX_TENSOR_DTYPE_F32: return 4u;
    case DX_TENSOR_DTYPE_F16:
    case DX_TENSOR_DTYPE_BF16: return 2u;
    case DX_TENSOR_DTYPE_I8:
    case DX_TENSOR_DTYPE_U8: return 1u;
    case DX_TENSOR_DTYPE_I32: return 4u;
    default: return 0u;
    }
}

bool multiply_overflow(uint64_t lhs, uint64_t rhs, uint64_t* result) noexcept {
    if (result == nullptr) {
        return true;
    }
    if (lhs != 0u && rhs > (std::numeric_limits<uint64_t>::max)() / lhs) {
        return true;
    }
    *result = lhs * rhs;
    return false;
}

} // namespace

extern "C" {

DXAIT_TENSOR_API void DXAIT_TENSOR_CALL dx_tensor_desc_init(dx_tensor_desc_t* desc) {
    if (desc == nullptr) {
        return;
    }
    std::memset(desc, 0, sizeof(*desc));
    desc->struct_size = sizeof(*desc);
    desc->api_version = DX_TENSOR_API_VERSION;
    desc->layout = DX_TENSOR_LAYOUT_STRIDED;
}

DXAIT_TENSOR_API int32_t DXAIT_TENSOR_CALL dx_tensor_desc_init_contiguous(
    dx_tensor_desc_t* desc,
    uint32_t rank,
    const uint64_t* shape,
    uint32_t dtype) {
    if (desc == nullptr || shape == nullptr || rank == 0u || rank > DX_TENSOR_MAX_RANK) {
        return -1;
    }
    if (dtype_bytes(dtype) == 0u && dtype != DX_TENSOR_DTYPE_I4_PACKED) {
        return -2;
    }

    dx_tensor_desc_init(desc);
    desc->rank = rank;
    desc->dtype = dtype;
    desc->layout = DX_TENSOR_LAYOUT_CONTIGUOUS_ROW_MAJOR;
    for (uint32_t index = 0u; index < rank; ++index) {
        if (shape[index] == 0u) {
            return -3;
        }
        desc->shape[index] = shape[index];
    }

    uint64_t stride = dtype == DX_TENSOR_DTYPE_I4_PACKED ? 1u : dtype_bytes(dtype);
    for (uint32_t index = rank; index-- > 0u;) {
        desc->strides_bytes[index] = static_cast<int64_t>(stride);
        uint64_t next_stride = 0u;
        if (multiply_overflow(stride, desc->shape[index], &next_stride) ||
            next_stride > static_cast<uint64_t>((std::numeric_limits<int64_t>::max)())) {
            return -4;
        }
        stride = next_stride;
    }
    return 0;
}

DXAIT_TENSOR_API int32_t DXAIT_TENSOR_CALL dx_tensor_desc_validate(
    const dx_tensor_desc_t* desc) {
    if (desc == nullptr || desc->struct_size < sizeof(dx_tensor_desc_t) ||
        desc->api_version != DX_TENSOR_API_VERSION || desc->rank == 0u ||
        desc->rank > DX_TENSOR_MAX_RANK) {
        return -1;
    }
    if (dtype_bytes(desc->dtype) == 0u && desc->dtype != DX_TENSOR_DTYPE_I4_PACKED) {
        return -2;
    }
    for (uint32_t index = 0u; index < desc->rank; ++index) {
        if (desc->shape[index] == 0u || desc->strides_bytes[index] <= 0) {
            return -3;
        }
    }
    uint64_t ignored = 0u;
    return dx_tensor_required_bytes(desc, &ignored);
}

DXAIT_TENSOR_API int32_t DXAIT_TENSOR_CALL dx_tensor_required_bytes(
    const dx_tensor_desc_t* desc,
    uint64_t* out_bytes) {
    if (desc == nullptr || out_bytes == nullptr || desc->rank == 0u ||
        desc->rank > DX_TENSOR_MAX_RANK) {
        return -1;
    }
    uint64_t maximum_offset = desc->byte_offset;
    for (uint32_t index = 0u; index < desc->rank; ++index) {
        if (desc->shape[index] == 0u || desc->strides_bytes[index] <= 0) {
            return -2;
        }
        uint64_t contribution = 0u;
        if (multiply_overflow(desc->shape[index] - 1u,
                              static_cast<uint64_t>(desc->strides_bytes[index]),
                              &contribution) ||
            maximum_offset > (std::numeric_limits<uint64_t>::max)() - contribution) {
            return -3;
        }
        maximum_offset += contribution;
    }
    const uint64_t element_bytes = desc->dtype == DX_TENSOR_DTYPE_I4_PACKED
                                      ? 1u : dtype_bytes(desc->dtype);
    if (element_bytes == 0u || maximum_offset >
        (std::numeric_limits<uint64_t>::max)() - element_bytes) {
        return -4;
    }
    *out_bytes = maximum_offset + element_bytes;
    return 0;
}

DXAIT_TENSOR_API int32_t DXAIT_TENSOR_CALL dx_tensor_is_contiguous(
    const dx_tensor_desc_t* desc) {
    if (dx_tensor_desc_validate(desc) != 0) {
        return 0;
    }
    const uint64_t element_bytes = desc->dtype == DX_TENSOR_DTYPE_I4_PACKED
                                      ? 1u : dtype_bytes(desc->dtype);
    uint64_t stride = element_bytes;
    for (uint32_t index = desc->rank; index-- > 0u;) {
        if (desc->strides_bytes[index] != static_cast<int64_t>(stride)) {
            return 0;
        }
        uint64_t next_stride = 0u;
        if (multiply_overflow(stride, desc->shape[index], &next_stride)) {
            return 0;
        }
        stride = next_stride;
    }
    return 1;
}

} // extern "C"
