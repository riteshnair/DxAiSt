// [DXAIT-COMPONENT: dxcapi]
// [DXAIT-IMPLEMENTATION: versioned C compatibility wrappers]
// [DXAIT-CPU-IMPACT: no changes to legacy CPU implementation]

#include "dxait/dx_c_api.h"
#include "dxait/dx_device_api.h"
#include "dxait/dx_memory_api.h"
#include "dxait/dx_ops_api.h"
#include "dxait/dx_runtime_api.h"
#include "dxait/dx_stream_api.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <vector>

struct dx_device_s {
    dx_execution_device_t* execution{nullptr};
    dx_memory_pool_t* memory{nullptr};
    dx_c_device_info_t info{};
};

struct dx_queue_s {
    dx_device_s* device{nullptr};
    dx_stream_t* stream{nullptr};
};

struct dx_buffer_s {
    dx_device_s* device{nullptr};
    dx_memory_allocation_t* allocation{nullptr};
};

namespace {

thread_local std::string last_error;

int32_t fail(dx_c_status_t status, const char* message) {
    last_error = message == nullptr ? "DxAiSt C API error" : message;
    return static_cast<int32_t>(status);
}

void clear_error() {
    last_error.clear();
}

int32_t component_result(int32_t result, const char* operation) {
    if (result == 0) {
        clear_error();
        return 0;
    }
    if (result == -1) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, operation);
    }
    if (result == -3) {
        return fail(DX_C_STATUS_ALLOCATION_FAILED, operation);
    }
    return fail(DX_C_STATUS_DEPENDENCY_FAILED, operation);
}

int32_t operator_result(int32_t result, const char* operation) {
    if (result == 0) {
        clear_error();
        return 0;
    }
    if (result == -1 || result == -2) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, operation);
    }
    return fail(DX_C_STATUS_INTERNAL_ERROR, operation);
}

bool valid_header(uint32_t struct_size, uint32_t api_version, uint32_t expected_size) {
    return struct_size >= expected_size && api_version == DXAIT_C_API_VERSION;
}

bool range_valid(uint64_t size, uint64_t offset, uint64_t bytes) {
    return offset <= size && bytes <= size - offset;
}

uint32_t map_location(uint32_t location) {
    switch (location) {
        case DX_C_BUFFER_DEFAULT:
            return DX_MEMORY_DEVICE;
        case DX_C_BUFFER_UPLOAD:
            return DX_MEMORY_UPLOAD;
        case DX_C_BUFFER_READBACK:
            return DX_MEMORY_READBACK;
        default:
            return UINT32_MAX;
    }
}

void cleanup_device(dx_device_s* device) {
    if (device == nullptr) {
        return;
    }
    dx_memory_pool_destroy(device->memory);
    device->memory = nullptr;
    dx_execution_device_destroy(device->execution);
    device->execution = nullptr;
    delete device;
}

int32_t validate_buffers(dx_device* device, dx_queue* queue,
                         const dx_buffer* out, const dx_buffer* a,
                         const dx_buffer* b = nullptr) {
    if (device == nullptr || queue == nullptr || queue->device != device ||
        out == nullptr || out->device != device || a == nullptr || a->device != device ||
        (b != nullptr && b->device != device)) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la: invalid handle ownership");
    }
    return 0;
}

int32_t read_bytes(dx_device* device, const dx_buffer* buffer, void* destination, uint64_t bytes) {
    if (buffer == nullptr || destination == nullptr || bytes > dx_buffer_size(buffer)) {
        return -1;
    }
    void* mapped = dx_buffer_map(const_cast<dx_buffer*>(buffer));
    if (mapped != nullptr) {
        std::memcpy(destination, mapped, static_cast<size_t>(bytes));
        return 0;
    }
    return dx_download(device, buffer, 0u, destination, bytes);
}

int32_t write_bytes(dx_device* device, dx_buffer* buffer, const void* source, uint64_t bytes) {
    if (buffer == nullptr || source == nullptr || bytes > dx_buffer_size(buffer)) {
        return -1;
    }
    void* mapped = dx_buffer_map(buffer);
    if (mapped != nullptr) {
        std::memcpy(mapped, source, static_cast<size_t>(bytes));
        return 0;
    }
    return dx_upload(device, buffer, 0u, source, bytes);
}

bool f32_bytes(uint64_t count, uint64_t* bytes) {
    return bytes != nullptr && count <= (std::numeric_limits<uint64_t>::max)() / sizeof(float) &&
           ((*bytes = count * sizeof(float)), true);
}

float activation_value(float x, int act, float alpha, bool* valid) {
    switch (act) {
        case 0: return std::max(0.0f, x);
        case 1: return 0.5f * x * (1.0f + std::erf(x / 1.41421356237f));
        case 2: return x / (1.0f + std::exp(-x));
        case 3: return std::tanh(x);
        case 4: return 1.0f / (1.0f + std::exp(-x));
        case 5: return x >= 0.0f ? x : alpha * x;
        default:
            if (valid != nullptr) *valid = false;
            return 0.0f;
    }
}

} // namespace

extern "C" {

DXAIT_C_API uint32_t DXAIT_C_CALL dx_c_api_version(void) {
    return DXAIT_C_API_VERSION;
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_create_device(uint32_t index, dx_device** out_device) {
    if (out_device == nullptr) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_create_device: out_device is null");
    }
    *out_device = nullptr;

    dx_device_manager_t* manager = nullptr;
    int32_t result = dx_device_manager_create(&manager);
    if (result != 0) {
        return component_result(result, "dx_create_device: device manager creation failed");
    }

    dx_device_info_t info{};
    info.struct_size = sizeof(info);
    info.api_version = DXAIT_C_API_VERSION;
    if (index >= dx_device_manager_count(manager) ||
        dx_device_manager_get_info(manager, index, &info) != 0) {
        dx_device_manager_destroy(manager);
        return fail(DX_C_STATUS_NOT_FOUND, "dx_create_device: device index not found");
    }

    dx_execution_device_t* execution = nullptr;
    result = dx_device_manager_open(manager, index, &execution);
    dx_device_manager_destroy(manager);
    if (result != 0) {
        return component_result(result, "dx_create_device: execution device open failed");
    }

    dx_memory_pool_t* memory = nullptr;
    void* native_device = dx_execution_device_native_handle(execution);
    result = native_device == nullptr
                 ? dx_memory_pool_create(&memory)
                 : dx_memory_pool_create_for_native_device(native_device, &memory);
    if (result != 0) {
        dx_execution_device_destroy(execution);
        return component_result(result, "dx_create_device: memory pool creation failed");
    }

    std::unique_ptr<dx_device_s> device(new (std::nothrow) dx_device_s{});
    if (!device) {
        dx_memory_pool_destroy(memory);
        dx_execution_device_destroy(execution);
        return fail(DX_C_STATUS_ALLOCATION_FAILED, "dx_create_device: allocation failed");
    }
    device->execution = execution;
    device->memory = memory;
    device->info.struct_size = sizeof(device->info);
    device->info.api_version = DXAIT_C_API_VERSION;
    device->info.backend = info.backend;
    device->info.vendor_id = info.vendor_id;
    device->info.device_id = info.device_id;
    device->info.capabilities = info.capabilities;
    device->info.is_software = info.is_software;
    device->info.dedicated_video_memory = info.dedicated_video_memory;
    device->info.dedicated_system_memory = info.dedicated_system_memory;
    device->info.shared_system_memory = info.shared_system_memory;
    std::memcpy(device->info.name, info.name, sizeof(device->info.name));
    *out_device = device.release();
    clear_error();
    return 0;
}

DXAIT_C_API void DXAIT_C_CALL dx_destroy_device(dx_device* device) {
    cleanup_device(device);
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_device_get_info(
    const dx_device* device,
    dx_c_device_info_t* out_info) {
    if (device == nullptr || out_info == nullptr ||
        !valid_header(out_info->struct_size, out_info->api_version, sizeof(*out_info))) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_device_get_info: invalid argument");
    }
    *out_info = device->info;
    clear_error();
    return 0;
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_device_queue(
    dx_device* device,
    dx_queue** out_queue) {
    if (device == nullptr || out_queue == nullptr) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_device_queue: invalid argument");
    }
    *out_queue = nullptr;

    dx_stream_t* stream = nullptr;
    const uint32_t backend = dx_execution_device_backend(device->execution);
    int32_t result = backend == DX_DEVICE_BACKEND_DX12
                         ? dx_stream_create_for_native_device(
                               dx_execution_device_native_handle(device->execution),
                               DX_STREAM_COMPUTE, &stream)
                         : dx_stream_create(DX_STREAM_HOST, &stream);
    if (result != 0) {
        return component_result(result, "dx_device_queue: stream creation failed");
    }

    std::unique_ptr<dx_queue_s> queue(new (std::nothrow) dx_queue_s{});
    if (!queue) {
        dx_stream_destroy(stream);
        return fail(DX_C_STATUS_ALLOCATION_FAILED, "dx_device_queue: allocation failed");
    }
    queue->device = device;
    queue->stream = stream;
    *out_queue = queue.release();
    clear_error();
    return 0;
}

DXAIT_C_API void DXAIT_C_CALL dx_destroy_queue(dx_queue* queue) {
    if (queue == nullptr) {
        return;
    }
    dx_stream_destroy(queue->stream);
    queue->stream = nullptr;
    delete queue;
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_create_buffer_ex(
    dx_device* device,
    const dx_c_buffer_desc_t* desc,
    dx_buffer** out_buffer) {
    if (device == nullptr || desc == nullptr || out_buffer == nullptr ||
        !valid_header(desc->struct_size, desc->api_version, sizeof(*desc)) ||
        desc->bytes == 0u) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_create_buffer: invalid argument");
    }
    *out_buffer = nullptr;
    const uint32_t location = map_location(desc->location);
    if (location == UINT32_MAX) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_create_buffer: invalid location");
    }

    dx_memory_desc_t memory_desc{};
    memory_desc.struct_size = sizeof(memory_desc);
    memory_desc.api_version = DXAIT_C_API_VERSION;
    memory_desc.bytes = desc->bytes;
    memory_desc.alignment = desc->alignment;
    memory_desc.location = location;
    memory_desc.flags = desc->flags;
    dx_memory_allocation_t* allocation = nullptr;
    const int32_t result = dx_memory_alloc(device->memory, &memory_desc, &allocation);
    if (result != 0) {
        return component_result(result, "dx_create_buffer: allocation failed");
    }

    std::unique_ptr<dx_buffer_s> buffer(new (std::nothrow) dx_buffer_s{});
    if (!buffer) {
        dx_memory_free(device->memory, allocation);
        return fail(DX_C_STATUS_ALLOCATION_FAILED, "dx_create_buffer: allocation failed");
    }
    buffer->device = device;
    buffer->allocation = allocation;
    *out_buffer = buffer.release();
    clear_error();
    return 0;
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_create_buffer(
    dx_device* device,
    uint64_t bytes,
    int location,
    dx_buffer** out_buffer) {
    dx_c_buffer_desc_t desc{};
    desc.struct_size = sizeof(desc);
    desc.api_version = DXAIT_C_API_VERSION;
    desc.bytes = bytes;
    desc.location = location < 0 ? UINT32_MAX : static_cast<uint32_t>(location);
    return dx_create_buffer_ex(device, &desc, out_buffer);
}

DXAIT_C_API void DXAIT_C_CALL dx_destroy_buffer(dx_buffer* buffer) {
    if (buffer == nullptr) {
        return;
    }
    if (buffer->device != nullptr && buffer->allocation != nullptr) {
        dx_memory_free(buffer->device->memory, buffer->allocation);
    }
    buffer->allocation = nullptr;
    delete buffer;
}

DXAIT_C_API void* DXAIT_C_CALL dx_buffer_map(dx_buffer* buffer) {
    return buffer == nullptr ? nullptr : dx_memory_data(buffer->allocation);
}

DXAIT_C_API void DXAIT_C_CALL dx_buffer_unmap(dx_buffer* buffer) {
    (void)buffer;
}

DXAIT_C_API uint64_t DXAIT_C_CALL dx_buffer_size(const dx_buffer* buffer) {
    return buffer == nullptr ? 0u : dx_memory_size(buffer->allocation);
}

DXAIT_C_API void* DXAIT_C_CALL dx_buffer_native_resource(const dx_buffer* buffer) {
    return buffer == nullptr ? nullptr : dx_memory_native_resource(buffer->allocation);
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_upload(
    dx_device* device,
    dx_buffer* destination,
    uint64_t offset,
    const void* source,
    uint64_t bytes) {
    if (device == nullptr || destination == nullptr || destination->device != device ||
        source == nullptr || !range_valid(dx_buffer_size(destination), offset, bytes)) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_upload: invalid argument");
    }
    const int32_t result = dx_memory_upload(
        device->memory, destination->allocation, offset, source, bytes);
    if (result == 0) {
        clear_error();
        return 0;
    }
    if (result == -1 || result == -2) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_upload: invalid range");
    }
    if (result == -3) {
        return fail(DX_C_STATUS_UNSUPPORTED, "dx_upload: device transfer unavailable");
    }
    return fail(DX_C_STATUS_DEVICE_ERROR, "dx_upload: device transfer failed");
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_download(
    dx_device* device,
    const dx_buffer* source,
    uint64_t offset,
    void* destination,
    uint64_t bytes) {
    if (device == nullptr || source == nullptr || source->device != device ||
        destination == nullptr || !range_valid(dx_buffer_size(source), offset, bytes)) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_download: invalid argument");
    }
    const int32_t result = dx_memory_download(
        device->memory, source->allocation, offset, destination, bytes);
    if (result == 0) {
        clear_error();
        return 0;
    }
    if (result == -1 || result == -2) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_download: invalid range");
    }
    if (result == -3) {
        return fail(DX_C_STATUS_UNSUPPORTED, "dx_download: device transfer unavailable");
    }
    return fail(DX_C_STATUS_DEVICE_ERROR, "dx_download: device transfer failed");
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_la_elementwise(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* a,
    dx_buffer* b, uint32_t count, int op, float alpha, float beta) {
    const int32_t handles = validate_buffers(device, queue, out, a, b);
    if (handles != 0 || op < 0 || op > 3) {
        return handles != 0 ? handles : fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_elementwise: invalid op");
    }
    uint64_t bytes = 0u;
    if (!f32_bytes(count, &bytes) || dx_buffer_size(out) < bytes ||
        dx_buffer_size(a) < bytes || dx_buffer_size(b) < bytes) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_elementwise: buffer too small");
    }
    std::vector<float> av(count), bv(count), ov(count, 0.0f);
    if (read_bytes(device, a, av.data(), bytes) != 0 || read_bytes(device, b, bv.data(), bytes) != 0 ||
        (beta != 0.0f && read_bytes(device, out, ov.data(), bytes) != 0)) {
        return fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_elementwise: buffer read failed");
    }
    for (uint32_t i = 0u; i < count; ++i) {
        float value = 0.0f;
        switch (op) {
            case 0: value = av[i] + bv[i]; break;
            case 1: value = av[i] - bv[i]; break;
            case 2: value = av[i] * bv[i]; break;
            case 3: value = bv[i] == 0.0f ? 0.0f : av[i] / bv[i]; break;
        }
        ov[i] = alpha * value + beta * ov[i];
    }
    return write_bytes(device, out, ov.data(), bytes) == 0
        ? 0 : fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_elementwise: buffer write failed");
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_la_activation(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* input,
    uint32_t count, int act, float alpha) {
    const int32_t handles = validate_buffers(device, queue, out, input);
    if (handles != 0) return handles;
    uint64_t bytes = 0u;
    if (!f32_bytes(count, &bytes) || dx_buffer_size(out) < bytes || dx_buffer_size(input) < bytes) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_activation: buffer too small");
    }
    bool valid = true;
    std::vector<float> values(count);
    if (read_bytes(device, input, values.data(), bytes) != 0) {
        return fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_activation: buffer read failed");
    }
    for (float& value : values) value = activation_value(value, act, alpha, &valid);
    if (!valid) return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_activation: invalid activation");
    return write_bytes(device, out, values.data(), bytes) == 0
        ? 0 : fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_activation: buffer write failed");
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_la_rmsnorm(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* input,
    dx_buffer* gamma, uint32_t rows, uint32_t dim, float eps) {
    const int32_t handles = validate_buffers(device, queue, out, input, gamma);
    if (handles != 0 || dim == 0u || eps < 0.0f) {
        return handles != 0 ? handles : fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_rmsnorm: invalid shape");
    }
    uint64_t elements = static_cast<uint64_t>(rows) * static_cast<uint64_t>(dim);
    uint64_t bytes = 0u;
    if (!f32_bytes(elements, &bytes) || !f32_bytes(dim, &elements) ||
        dx_buffer_size(out) < bytes || dx_buffer_size(input) < bytes ||
        dx_buffer_size(gamma) < elements) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_rmsnorm: buffer too small");
    }
    const uint64_t row_bytes = static_cast<uint64_t>(dim) * sizeof(float);
    std::vector<float> values(rows * dim), gamma_values(dim);
    if (read_bytes(device, input, values.data(), bytes) != 0 ||
        read_bytes(device, gamma, gamma_values.data(), elements) != 0) {
        return fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_rmsnorm: buffer read failed");
    }
    for (uint32_t row = 0u; row < rows; ++row) {
        float sum = 0.0f;
        for (uint32_t col = 0u; col < dim; ++col) {
            const float value = values[static_cast<size_t>(row) * dim + col];
            sum += value * value;
        }
        const float scale = 1.0f / std::sqrt(sum / static_cast<float>(dim) + eps);
        for (uint32_t col = 0u; col < dim; ++col) {
            values[static_cast<size_t>(row) * dim + col] *= scale * gamma_values[col];
        }
    }
    (void)row_bytes;
    return write_bytes(device, out, values.data(), bytes) == 0
        ? 0 : fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_rmsnorm: buffer write failed");
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_la_softmax(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* input,
    uint32_t rows, uint32_t dim) {
    const int32_t handles = validate_buffers(device, queue, out, input);
    if (handles != 0 || dim == 0u) return handles != 0 ? handles : fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_softmax: invalid shape");
    const uint64_t elements = static_cast<uint64_t>(rows) * dim;
    uint64_t bytes = 0u;
    if (!f32_bytes(elements, &bytes) || dx_buffer_size(out) < bytes || dx_buffer_size(input) < bytes) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_softmax: buffer too small");
    }
    std::vector<float> values(static_cast<size_t>(elements));
    if (read_bytes(device, input, values.data(), bytes) != 0) return fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_softmax: buffer read failed");
    for (uint32_t row = 0u; row < rows; ++row) {
        float maximum = values[static_cast<size_t>(row) * dim];
        for (uint32_t col = 1u; col < dim; ++col) maximum = std::max(maximum, values[static_cast<size_t>(row) * dim + col]);
        float sum = 0.0f;
        for (uint32_t col = 0u; col < dim; ++col) {
            float& value = values[static_cast<size_t>(row) * dim + col];
            value = std::exp(value - maximum);
            sum += value;
        }
        for (uint32_t col = 0u; col < dim; ++col) values[static_cast<size_t>(row) * dim + col] /= sum;
    }
    return write_bytes(device, out, values.data(), bytes) == 0
        ? 0 : fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_softmax: buffer write failed");
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_la_reduce(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* input,
    uint32_t rows, uint32_t dim, int op) {
    const int32_t handles = validate_buffers(device, queue, out, input);
    if (handles != 0 || dim == 0u || op < 0 || op > 3) return handles != 0 ? handles : fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_reduce: invalid argument");
    const uint64_t elements = static_cast<uint64_t>(rows) * dim;
    uint64_t bytes = 0u;
    if (!f32_bytes(elements, &bytes) || dx_buffer_size(out) < static_cast<uint64_t>(rows) * sizeof(float) || dx_buffer_size(input) < bytes) return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_reduce: buffer too small");
    std::vector<float> values(static_cast<size_t>(elements)), result(rows);
    if (read_bytes(device, input, values.data(), bytes) != 0) return fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_reduce: buffer read failed");
    for (uint32_t row = 0u; row < rows; ++row) {
        float value = values[static_cast<size_t>(row) * dim];
        for (uint32_t col = 1u; col < dim; ++col) {
            const float current = values[static_cast<size_t>(row) * dim + col];
            if (op == 0 || op == 3) value += current;
            else if (op == 1) value = std::max(value, current);
            else value = std::min(value, current);
        }
        result[row] = op == 3 ? value / static_cast<float>(dim) : value;
    }
    return write_bytes(device, out, result.data(), static_cast<uint64_t>(rows) * sizeof(float)) == 0
        ? 0 : fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_reduce: buffer write failed");
}

static float half_to_float(uint16_t value) {
    const uint32_t sign = (value & 0x8000u) << 16u;
    const uint32_t exponent = (value >> 10u) & 0x1fu;
    const uint32_t mantissa = value & 0x3ffu;
    uint32_t bits = 0u;
    if (exponent == 0u) {
        if (mantissa != 0u) {
            float subnormal = static_cast<float>(mantissa) / 1024.0f;
            return (value & 0x8000u) != 0u ? -subnormal * 0.00006103515625f : subnormal * 0.00006103515625f;
        }
        bits = sign;
    } else if (exponent == 0x1fu) {
        bits = sign | 0x7f800000u | (mantissa << 13u);
    } else {
        bits = sign | ((exponent + 112u) << 23u) | (mantissa << 13u);
    }
    float output;
    std::memcpy(&output, &bits, sizeof(output));
    return output;
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_la_gemm_f16_dot2(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* a,
    dx_buffer* b, uint32_t M, uint32_t N, uint32_t K) {
    const int32_t handles = validate_buffers(device, queue, out, a, b);
    if (handles != 0 || M == 0u || N == 0u || K == 0u) return handles != 0 ? handles : fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_gemm_f16_dot2: invalid shape");
    const uint64_t a_bytes = static_cast<uint64_t>(M) * K * sizeof(uint16_t);
    const uint64_t b_bytes = static_cast<uint64_t>(K) * N * sizeof(uint16_t);
    const uint64_t c_bytes = static_cast<uint64_t>(M) * N * sizeof(float);
    if (dx_buffer_size(a) < a_bytes || dx_buffer_size(b) < b_bytes || dx_buffer_size(out) < c_bytes) return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_la_gemm_f16_dot2: buffer too small");
    std::vector<uint16_t> av(static_cast<size_t>(M) * K), bv(static_cast<size_t>(K) * N);
    std::vector<float> cv(static_cast<size_t>(M) * N, 0.0f);
    if (read_bytes(device, a, av.data(), a_bytes) != 0 || read_bytes(device, b, bv.data(), b_bytes) != 0) return fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_gemm_f16_dot2: buffer read failed");
    for (uint32_t row = 0u; row < M; ++row) for (uint32_t col = 0u; col < N; ++col) for (uint32_t inner = 0u; inner < K; ++inner) cv[static_cast<size_t>(row) * N + col] += half_to_float(av[static_cast<size_t>(row) * K + inner]) * half_to_float(bv[static_cast<size_t>(inner) * N + col]);
    return write_bytes(device, out, cv.data(), c_bytes) == 0
        ? 0 : fail(DX_C_STATUS_DEVICE_ERROR, "dx_la_gemm_f16_dot2: buffer write failed");
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_la_gemm_f16_wmma(
    dx_device* device, dx_queue* queue, dx_buffer* out, dx_buffer* a,
    dx_buffer* b, uint32_t M, uint32_t N, uint32_t K) {
    return dx_la_gemm_f16_dot2(device, queue, out, a, b, M, N, K);
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_copy_f32(
    dx_queue* queue,
    const float* input,
    float* output,
    uint64_t count) {
    if (queue == nullptr || queue->stream == nullptr) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_copy_f32: queue is null");
    }
    return operator_result(dx_op_copy_f32(input, output, count), "dx_copy_f32 failed");
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_fill_f32(
    dx_queue* queue,
    float* output,
    uint64_t count,
    float value) {
    if (queue == nullptr || queue->stream == nullptr) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_fill_f32: queue is null");
    }
    return operator_result(dx_op_fill_f32(output, count, value), "dx_fill_f32 failed");
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_gemm_f32(
    dx_queue* queue,
    const float* a,
    const float* b,
    float* c,
    uint64_t m,
    uint64_t n,
    uint64_t k,
    float alpha,
    float beta) {
    if (queue == nullptr || queue->stream == nullptr) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_gemm_f32: queue is null");
    }
    return operator_result(dx_op_gemm_f32(a, b, c, m, n, k, alpha, beta),
                           "dx_gemm_f32 failed");
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_wait(dx_device* device) {
    if (device == nullptr || device->execution == nullptr) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_wait: device is null");
    }
    clear_error();
    return 0;
}

DXAIT_C_API int32_t DXAIT_C_CALL dx_device_desc(
    const dx_device* device,
    char* buffer,
    uint32_t capacity) {
    if (device == nullptr || buffer == nullptr || capacity == 0u) {
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_device_desc: invalid argument");
    }
    const int written = std::snprintf(
        buffer, capacity, "%s backend=%u vendor=0x%04x device=0x%04x",
        device->info.name, device->info.backend, device->info.vendor_id, device->info.device_id);
    if (written < 0 || static_cast<uint32_t>(written) >= capacity) {
        buffer[0] = '\0';
        return fail(DX_C_STATUS_INVALID_ARGUMENT, "dx_device_desc: buffer too small");
    }
    clear_error();
    return 0;
}

DXAIT_C_API const char* DXAIT_C_CALL dx_last_error(void) {
    return last_error.c_str();
}

} // extern "C"
