# DXAiSt Component Status

**Status values:** `finished`, `in progress`, `pending`, `blocked`, `retired`
**Rule:** compilation alone never qualifies a component as finished.

**Mandatory protocol:** every row must be checked against `docs/COMPLETION_AUDIT.md` and `docs/VALIDATION_PROTOCOL.md`. Before a row becomes `finished`, record: name inventory, public header compile, declaration-to-definition map, CMake target map, DLL export/import-library audit, real consumer runtime call, failure-path test, numerical/reference test where applicable, shader/kernel lifecycle proof where applicable, trace/performance evidence, clean-package test, and rollback checkpoint. Any `FAIL`, `UNKNOWN`, or unrun Windows/GPU gate keeps the row `in progress` or `blocked`.

| DLL/component | Status | Public contract | Production call site | Tests | Perf trace | Notes |
|---|---|---|---|---|---|---|
| `dx12_dxcore.dll` | in progress | `dx_core_api.h`, `dx_log.hpp` | C ABI logger bridge | `test_dxcore_logging` passed | Host-side log records verified; GPU trace pending | Logger foundation and CMake target implemented; Windows DLL validation pending |
| `dx12_dxcapi.dll` | in progress | `dx_c_api.h` | C99 compatibility wrappers over component ABIs | `test_dxcapi_c`, `test_dxcapi_cpp` and full 15-test portable suite passed | Host-side only; GPU trace pending | Versioned C API implemented and 20/20 portable exports verified; Windows DLL/runtime/GPU validation pending |
| `dx12_dxruntime.dll` | in progress | `dx_runtime_api.h` | Runtime creates device inventory and selects backend | `test_dxruntime` passed | Host log records; GPU trace pending | Separate runtime DLL now links dxcore and dxdevice |
| `dx12_dxdevice.dll` | in progress | `dx_device_api.h` | Adapter inventory and executable-device open path | `test_dxdevice` passed | Device trace pending | DXGI/D3D12 hooks on Windows; CPU reference on host |
| `dx12_dxmemory.dll` | in progress | `dx_memory_api.h` | Portable aligned allocator path | `test_dxmemory` passed | Allocation trace pending | DX12 heaps/residency/ReBAR still pending |
| `dx12_dxstream.dll` | in progress | `dx_stream_api.h` | Host stream/event path; native queue hooks | `test_dxstream` passed | Queue/fence trace pending | DX12 command allocator/list lifecycle pending |
| `dx12_dxtensor.dll` | in progress | `dx_tensor_api.h` | Descriptor validation and layout helpers | `test_dxtensor_desc` passed | Tensor trace pending | Device-backed tensor storage pending |
| `dx12_dxgraph.dll` | in progress | `dx_graph_api.h` | Registry and immutable topological plan path | `test_dxgraph_api` passed | Graph trace pending | Fusion, shape specialization, execution pending |
| `dx12_dxops.dll` | in progress | `dx_ops_api.h` | Portable reference copy/fill/GEMM path | `test_dxops` passed | Host reference only; GPU kernel trace pending | Native DX12/HIP dispatch and full operator coverage pending |
| `dx12_dxmodel.dll` | in progress | `dx_model_api.h` | Safe container detection and bounded reads | `test_dxmodel` passed | Host I/O records; model-load trace pending | Full GGUF/Safetensors/ONNX metadata import pending |
| `dx12_dxio.dll` | in progress | `dx_io_api.h` | Portable ranged-read provider | `test_dxio` passed | Host I/O log records; transfer trace pending | DirectStorage queue/GPU decompression pending |
| `dx12_dxinfer.dll` | pending | pending | pending | pending | pending | Sessions/KV/batching/sampling |
| `dx12_dxmedia.dll` | pending | pending | pending | pending | pending | Speech/image/diffusion pipelines |
| `dx12_dxtrace.dll` | in progress | `dx_trace_api.h` | Host span capture and JSON export | `test_dxtrace_api` passed | Host spans; GPU timestamps pending | Cross-DLL trace service foundation |
| `dx12_dxnative.hpp` | in progress | C++20 facade header | RAII composition smoke test | `test_dxnative` passed | Inherits component traces | Convenience facade over the C99 ABI; binary adapter DLLs pending |
| `dx12_dxcompat.dll` | pending | pending | pending | pending | pending | ONNX Runtime/llama.cpp/optional providers |
| `dx12_dxhip.dll` | pending | pending | pending | pending | pending | Optional HIP/ROCm backend |
| `dx12_dxcpu.dll` | pending | pending | pending | pending | pending | Reference/SIMD backend |

## Required status evidence

Each row must include the exact evidence location for these fields, even when the value is `N/A`:

| Field | Required value |
|---|---|
| `public_declaration` | Header path and C99/C++20 consumer compile result |
| `implementation_definition` | Definition path and symbol-completeness result |
| `build_target` | CMake target and clean configure/build result |
| `export_or_link_symbol` | DLL/import-library/static-symbol report |
| `runtime_test` | Real consumer command and result, or named platform blocker |
| `reference_test` | CPU/reference comparison, or `N/A` with reason |
| `failure_test` | Null/invalid/overflow/missing-dependency result |
| `shader_kernel_test` | Source -> artifact -> reflection -> PSO -> dispatch result, or `N/A` |
| `trace_test` | Required host/GPU trace result, or named blocker |
| `package_entry` | Manifest path and clean-package result |
| `rollback_checkpoint` | Exact prior checkpoint and reversal command |

## Finished criteria

A row may be changed to `finished` only when all of the following are present:

- The public C99 and/or C++20 contract is documented and versioned, including `dx_c_api.h` compatibility consumers.
- The DLL has a production build target and no developer-local hard-coded paths.
- The implementation is called by a production path, not only a test or sample.
- Unit and integration tests cover success, invalid input, lifetime, concurrency, and failure paths.
- A performance benchmark records the component’s relevant latency, throughput, memory, and synchronization metrics.
- The component emits trace records when its DLL-scoped trace/performance variables are enabled.
- The changed-file audit finds no placeholder, stub, fake, dummy, dead, or unconnected production code.
- The component has a documented fallback and capability requirements.
