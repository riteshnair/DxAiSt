// [DXAIT-COMPONENT: dxshader]
// [DXAIT-SUBSYSTEM: DXC shader test]
// [DXAIT-TEST: argument validation and compiler availability]

#include "dxait/dx_shader_api.h"

#include <cassert>

int main() {
    dx_shader_blob_t* blob = nullptr;
    assert(dx_shader_compile_file(nullptr, "main", "cs_6_7", &blob) != 0);
    assert(blob == nullptr);
    assert(dx_shader_compile_file("missing.hlsl", "main", "cs_6_7", &blob) != 0);
    assert(blob == nullptr);
#ifndef _WIN32
    assert(dx_shader_compile_file("missing.hlsl", "main", "cs_6_7", &blob) == -2);
#endif
    assert(dx_shader_blob_data(nullptr) == nullptr);
    assert(dx_shader_blob_size(nullptr) == 0u);
    dx_shader_blob_destroy(nullptr);
    return 0;
}
