// [DXAIT-COMPONENT: dxshader]
// [DXAIT-SUBSYSTEM: DXC shader ABI]
// [DXAIT-IMPLEMENTATION: DXC file compiler and immutable blob]

#include "dxait/dx_shader_api.h"
#include "dxait/dx_core_api.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <vector>

#ifdef _WIN32
#include <dxcapi.h>
#include <windows.h>
#include <wrl/client.h>
#endif

struct dx_shader_blob_t {
#ifdef _WIN32
    Microsoft::WRL::ComPtr<IDxcBlob> blob;
#else
    std::vector<uint8_t> bytes;
#endif
};

namespace {

bool valid_text(const char* value) noexcept {
    return value != nullptr && value[0] != '\0';
}

#ifdef _WIN32
std::wstring widen_utf8(const char* value) {
    const int required = MultiByteToWideChar(CP_UTF8, 0, value, -1, nullptr, 0);
    if (required <= 1) {
        return {};
    }
    std::wstring result(static_cast<size_t>(required), L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, value, -1, result.data(), required) <= 0) {
        return {};
    }
    result.pop_back();
    return result;
}
#endif

} // namespace

extern "C" {

DXAIT_SHADER_API int32_t DXAIT_SHADER_CALL dx_shader_compile_file(
    const char* source_path,
    const char* entry_point,
    const char* target_profile,
    dx_shader_blob_t** out_blob) {
    if (!valid_text(source_path) || !valid_text(entry_point) ||
        !valid_text(target_profile) || out_blob == nullptr) {
        return -1;
    }
    *out_blob = nullptr;
#ifdef _WIN32
    try {
        Microsoft::WRL::ComPtr<IDxcUtils> utils;
        Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
        if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils))) ||
            FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)))) {
            return -2;
        }

        uint32_t code_page = DXC_CP_UTF8;
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> source;
        const std::wstring wide_source_path = widen_utf8(source_path);
        if (wide_source_path.empty() || FAILED(utils->LoadFile(wide_source_path.c_str(), &code_page, &source))) {
            return -3;
        }
        DxcBuffer buffer{};
        buffer.Ptr = source->GetBufferPointer();
        buffer.Size = source->GetBufferSize();
        buffer.Encoding = DXC_CP_UTF8;

        const std::wstring entry = widen_utf8(entry_point);
        const std::wstring profile = widen_utf8(target_profile);
        if (entry.empty() || profile.empty()) {
            return -4;
        }
        std::vector<LPCWSTR> arguments;
        arguments.push_back(L"-E");
        arguments.push_back(entry.c_str());
        arguments.push_back(L"-T");
        arguments.push_back(profile.c_str());
        arguments.push_back(L"-HV");
        arguments.push_back(L"2021");

        Microsoft::WRL::ComPtr<IDxcResult> result;
        if (FAILED(compiler->Compile(&buffer, arguments.data(),
                                     static_cast<uint32_t>(arguments.size()),
                                     nullptr, IID_PPV_ARGS(&result)))) {
            return -4;
        }
        HRESULT status = S_OK;
        if (FAILED(result->GetStatus(&status)) || FAILED(status)) {
            return -5;
        }
        auto blob = std::make_unique<dx_shader_blob_t>();
        if (FAILED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&blob->blob), nullptr))) {
            return -6;
        }
        *out_blob = blob.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -7;
    } catch (...) {
        return -8;
    }
#else
    (void)source_path;
    (void)entry_point;
    (void)target_profile;
    // DXC is a Windows-target dependency; never return a fake shader blob.
    return -2;
#endif
}

DXAIT_SHADER_API void DXAIT_SHADER_CALL dx_shader_blob_destroy(dx_shader_blob_t* blob) {
    delete blob;
}

DXAIT_SHADER_API const void* DXAIT_SHADER_CALL dx_shader_blob_data(
    const dx_shader_blob_t* blob) {
    if (blob == nullptr) {
        return nullptr;
    }
#ifdef _WIN32
    return blob->blob == nullptr ? nullptr : blob->blob->GetBufferPointer();
#else
    return blob->bytes.empty() ? nullptr : blob->bytes.data();
#endif
}

DXAIT_SHADER_API uint64_t DXAIT_SHADER_CALL dx_shader_blob_size(
    const dx_shader_blob_t* blob) {
    if (blob == nullptr) {
        return 0u;
    }
#ifdef _WIN32
    return blob->blob == nullptr ? 0u : static_cast<uint64_t>(blob->blob->GetBufferSize());
#else
    return static_cast<uint64_t>(blob->bytes.size());
#endif
}

} // extern "C"
