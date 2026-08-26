// [DXAIT-COMPONENT: dxmodel]
// [DXAIT-SUBSYSTEM: model file test]
// [DXAIT-TEST: format detection, ranged reads, and safe failure]

#include "dxait/dx_model_api.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>

int main() {
    const char* gguf_path = "dxmodel_test.gguf";
    {
        std::ofstream output(gguf_path, std::ios::binary | std::ios::trunc);
        output.write("GGUF", 4);
        output.write("data", 4);
    }

    dx_model_t* model = nullptr;
    assert(dx_model_open(gguf_path, &model) == 0);
    assert(model != nullptr);
    assert(dx_model_format(model) == DX_MODEL_FORMAT_GGUF);
    assert(dx_model_size(model) == 8u);
    char buffer[8]{};
    uint64_t bytes = 0u;
    assert(dx_model_read(model, 4u, buffer, 4u, &bytes) == 0);
    assert(bytes == 4u && std::memcmp(buffer, "data", 4u) == 0);
    assert(dx_model_read(model, 99u, buffer, 4u, &bytes) == 1);
    assert(bytes == 0u);
    dx_model_destroy(model);

    assert(dx_model_open("missing-model.bin", &model) != 0);
    assert(model == nullptr);
    std::remove(gguf_path);
    return 0;
}
