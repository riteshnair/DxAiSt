// [DXAIT-COMPONENT: dxmodel]
// [DXAIT-SUBSYSTEM: model file ABI]
// [DXAIT-IMPLEMENTATION: bounded container detection and ranged reads]

#include "dxait/dx_model_api.h"
#include "dxait/dx_io_api.h"
#include "dxait/dx_core_api.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <memory>
#include <new>
#include <string>

struct dx_model_t {
    dx_component_logger_t* logger{nullptr};
    dx_io_provider_t* provider{nullptr};
    uint64_t bytes{0u};
    dx_model_format_t format{DX_MODEL_FORMAT_UNKNOWN};
    std::string path;

    ~dx_model_t() {
        dx_io_provider_destroy(provider);
        provider = nullptr;
        dx_component_logger_destroy(logger);
        logger = nullptr;
    }
};

namespace {

uint64_t file_size(const char* path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        return 0u;
    }
    const std::streamoff size = input.tellg();
    return size > 0 ? static_cast<uint64_t>(size) : 0u;
}

dx_model_format_t detect_format(const char* path, const unsigned char* prefix, uint64_t count) {
    const std::string name(path == nullptr ? "" : path);
    const auto dot = name.find_last_of('.');
    const std::string extension = dot == std::string::npos ? "" : name.substr(dot);
    if (count >= 4u && std::memcmp(prefix, "GGUF", 4u) == 0) {
        return DX_MODEL_FORMAT_GGUF;
    }
    if (extension == ".safetensors") {
        return DX_MODEL_FORMAT_SAFETENSORS;
    }
    if (extension == ".onnx") {
        return DX_MODEL_FORMAT_ONNX;
    }
    return DX_MODEL_FORMAT_GENERIC;
}

} // namespace

extern "C" {

DXAIT_MODEL_API int32_t DXAIT_MODEL_CALL dx_model_open(
    const char* path,
    dx_model_t** out_model) {
    if (path == nullptr || path[0] == '\0' || out_model == nullptr) {
        return -1;
    }
    *out_model = nullptr;
    const uint64_t size = file_size(path);
    if (size == 0u) {
        return -2;
    }
    try {
        auto model = std::make_unique<dx_model_t>();
        model->path = path;
        model->bytes = size;
        if (dx_component_logger_create("dxmodel", &model->logger) != 0 ||
            dx_io_provider_create(DX_IO_PROVIDER_AUTO, &model->provider) != 0) {
            return -3;
        }
        unsigned char prefix[8]{};
        uint64_t read = 0u;
        (void)dx_io_read_file(model->provider, path, 0u, prefix, sizeof(prefix), &read);
        model->format = detect_format(path, prefix, read);
        *out_model = model.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -4;
    } catch (...) {
        return -5;
    }
}

DXAIT_MODEL_API void DXAIT_MODEL_CALL dx_model_destroy(dx_model_t* model) {
    delete model;
}

DXAIT_MODEL_API uint32_t DXAIT_MODEL_CALL dx_model_format(const dx_model_t* model) {
    return model == nullptr ? DX_MODEL_FORMAT_UNKNOWN : static_cast<uint32_t>(model->format);
}

DXAIT_MODEL_API uint64_t DXAIT_MODEL_CALL dx_model_size(const dx_model_t* model) {
    return model == nullptr ? 0u : model->bytes;
}

DXAIT_MODEL_API int32_t DXAIT_MODEL_CALL dx_model_read(
    dx_model_t* model,
    uint64_t offset,
    void* buffer,
    uint64_t capacity,
    uint64_t* out_bytes_read) {
    if (model == nullptr || model->provider == nullptr) {
        return -1;
    }
    return dx_io_read_file(model->provider, model->path.c_str(), offset,
                            buffer, capacity, out_bytes_read);
}

} // extern "C"
