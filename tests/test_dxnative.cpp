// [DXAIT-COMPONENT: dxnative]
// [DXAIT-SUBSYSTEM: C++20 facade test]
// [DXAIT-TEST: RAII wrappers and cross-DLL composition]

#include "dxait/dx_native.hpp"

#include <cassert>
#include <cstdio>

int main() {
    dxait::Runtime runtime;
    const auto runtime_info = runtime.info();
    assert(runtime_info.device_count >= 1u);

    const auto tensor = dxait::TensorDesc::contiguous({2u, 2u}, DX_TENSOR_DTYPE_F32);
    assert(tensor.bytes() == 16u);

    dxait::MemoryPool memory;
    auto allocation = memory.allocate(256u);
    assert(allocation.data() != nullptr && allocation.size() == 256u);

    dxait::Stream stream;
    auto event = stream.record();
    assert(event.complete());
    event.wait();

    dxait::TraceSession trace("cpp-facade-test");
    auto span = trace.begin("test", "facade_operation", 7u);
    span.end();
    trace.export_json("dxnative_test.json");
    std::remove("dxnative_test.json");
    return 0;
}
