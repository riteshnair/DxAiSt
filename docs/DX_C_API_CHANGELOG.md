# DxAiSt C API compatibility changelog

## Record 001 - Versioned C API compatibility layer

| Field | Value |
|---|---|
| Prior state | `dx_c_api.h` declared a Windows-leaking legacy surface with no dedicated implementation target or C API tests |
| Files | `include/dxait/dx_c_api.h`, `src/core/dx_c_api.cpp`, `CMakeLists.txt`, `tests/test_dxcapi.c`, `tests/test_dxcapi.cpp` |
| New target | `dx12_dxcapi` |
| ABI | C99, opaque handles, `DXAIT_C_API_VERSION == 1`, `DXAIT_C_CALL`, explicit `dx_c_status_t` values |
| Implemented symbols | Version query, device lifecycle/info, queue lifecycle, both buffer constructors, buffer lifecycle/map/size/native resource, upload/download, F32 copy/fill/GEMM, all baseline `dx_la_*` functions, wait, device description, last error |
| Wrapper ownership | Compatibility device owns the DxAiSt execution handle and memory pool. Compatibility queue owns its stream and must not outlive its parent device. Compatibility buffer owns its memory allocation and must not outlive its parent device. |
| Transfer behavior | Host-visible allocations use direct mapped copies. On Windows, default device buffers use temporary upload/readback resources with a synchronized copy queue and state restoration; portable reference allocations use direct mapped copies. |
| Execution behavior | Current compatibility F32 operations call the validated DxAiSt reference operator ABI. Queue is accepted for API stability; no asynchronous command-list submission is claimed. |
| Tests | `test_dxcapi_c` C99 consumer; `test_dxcapi_cpp` C++20 consumer; existing portable suite; built artifact export audit |
| Portable evidence | Fresh `build-c-api`: 15/15 CTest tests passed; 20/20 compatibility symbols exported with C linkage |
| Windows evidence | Not run in the Linux sandbox. Windows DLL load, D3D12 execution, default-resource transfers, and GPU numerical proof remain pending. |
| Rollback | Remove `src/core/dx_c_api.cpp`, `tests/test_dxcapi.c`, and `tests/test_dxcapi.cpp`; remove `dx12_dxcapi` target and C API test option from `CMakeLists.txt`; restore the prior `dx_c_api.h`; rerun the pre-change build and test commands. |

## Record 003 - Legacy C API surface restoration

| Field | Value |
|---|---|
| Files | `include/dxait/dx_c_api.h`, `src/core/dx_c_api.cpp`, `tests/test_dxcapi.c` |
| Change | Restored source-compatible `dx_create_buffer(bytes, location, ...)` and all baseline `dx_la_*` declarations; added descriptor form `dx_create_buffer_ex`; implemented elementwise arithmetic, activations, RMSNorm, softmax, reductions, F16 GEMM dot2 reference path, and WMMA fallback |
| Compatibility | Baseline operation codes preserved: elementwise 0-3, activation 0-5, reduction 0-3. No `HRESULT`/`windows.h` dependency remains in the C99 header. |
| Portable evidence | `test_dxcapi_c`, `test_dxcapi_cpp`, and the full 15-test suite pass. |
| Windows evidence | Not run in the Linux sandbox; GPU/native kernel paths remain separately gated. |
| Rollback | Restore to Record 002 and remove the legacy wrapper/operator declarations and definitions. |
| Status | `implemented and portable-tested; native GPU dispatch pending` |

## Record 002 - Native transfer and adapter-selection hardening

| Field | Value |
|---|---|
| Files | `include/dxait/dx_memory_api.h`, `src/core/dx_memory_api.cpp`, `src/core/dx_device_api.cpp`, `src/core/dx_c_api.cpp`, `cmake/DXAITShaders.cmake`, `CMakeLists.txt`, `tests/test_dxcapi.c` |
| Change | Added synchronized D3D12 upload/readback staging copies; tracked allocation resource state and restored it after copies; fixed adapter-index open to use the enumerated IDXGI adapter; added SM 6.7 DXC artifact wiring for all current HLSL entry points; added default-buffer transfer regression coverage |
| Portable evidence | Fresh `build-c-api` passed 15/15 tests. DXC artifact target correctly deferred because no DXC executable exists on the non-Windows host. |
| Windows evidence | Not run in the Linux sandbox. Real D3D12 resource transitions, staging copies, adapter selection, shader compilation, and GPU numerical proof remain pending. |
| Rollback | Restore the files to Record 001 and remove `DXAITShaders.cmake` include/call and the `DXAIT_BUILD_SHADER_ARTIFACTS` option. |
| Status | `source implemented and portable-tested; Windows runtime pending` |

## Completion state

The versioned C API compatibility slice and source-side staging/adapter hardening are **implemented and portable-tested**. The `dx12_dxcapi.dll` component remains **in progress** until Windows runtime, DLL export/import-library, lifecycle failure, shader-artifact, and GPU/real-device gates pass.
