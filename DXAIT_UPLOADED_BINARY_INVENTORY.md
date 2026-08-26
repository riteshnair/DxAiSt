# Uploaded DxAiSt binary inventory

## Source archive

The uploaded archive contains a Windows x64 Visual Studio Release build under `build-windows/Release`, the full DxAiSt source tree, public headers, CMake files, tests, and vendored DirectX-Headers.

## Runtime artifacts found

The Release directory contains the following DxAiSt component DLLs and matching import libraries:

| Component | DLL | Import library |
|---|---|---|
| Core diagnostics | `dx12_dxcore.dll` | `dx12_dxcore.lib` |
| Runtime | `dx12_dxruntime.dll` | `dx12_dxruntime.lib` |
| Device | `dx12_dxdevice.dll` | `dx12_dxdevice.lib` |
| Memory | `dx12_dxmemory.dll` | `dx12_dxmemory.lib` |
| Stream | `dx12_dxstream.dll` | `dx12_dxstream.lib` |
| Tensor | `dx12_dxtensor.dll` | `dx12_dxtensor.lib` |
| Graph | `dx12_dxgraph.dll` | `dx12_dxgraph.lib` |
| Trace | `dx12_dxtrace.dll` | `dx12_dxtrace.lib` |
| I/O | `dx12_dxio.dll` | `dx12_dxio.lib` |
| Operators | `dx12_dxops.dll` | `dx12_dxops.lib` |
| Model | `dx12_dxmodel.dll` | `dx12_dxmodel.lib` |
| Shader | `dx12_dxshader.dll` | `dx12_dxshader.lib` |

The archive also contains `dxcompiler.dll`, `dxil.dll`, `dstorage.dll`, `dstoragecore.dll`, and `D3D12/D3D12Core.dll` under the Release output. It contains a monolithic `dxait.lib` and many Windows test executables.

## Export and import verification

The component DLLs contain the expected unmangled C exports, and the matching `.lib` files expose corresponding `__imp_` import symbols. Verified examples include:

| DLL | Verified exports |
|---|---|
| `dx12_dxruntime.dll` | `dx_runtime_create`, `dx_runtime_destroy`, `dx_runtime_get_info`, `dx_runtime_component_enabled` |
| `dx12_dxdevice.dll` | device manager creation/count/info/open and executable-device functions |
| `dx12_dxmemory.dll` | pool creation, allocation/free, data/size/native-resource, statistics |
| `dx12_dxstream.dll` | stream creation, native-device stream creation, events, waits, sequence, destruction |
| `dx12_dxgraph.dll` | operator registry, graph creation/add/compile, plan inspection/destruction |
| `dx12_dxtrace.dll` | trace session/span creation, end/destroy, event count, JSON export |
| `dx12_dxio.dll` | provider creation/destroy/kind and ranged file read |
| `dx12_dxops.dll` | `dx_op_copy_f32`, `dx_op_fill_f32`, `dx_op_gemm_f32` |
| `dx12_dxmodel.dll` | model open/destroy/format/size/read |
| `dx12_dxshader.dll` | shader compile and blob data/size/destroy |

The import libraries contain normal Microsoft COFF import records and are suitable for link-time use from an x64 MSVC llama.cpp build. The DLLs also support runtime loading through `LoadLibraryW` and `GetProcAddress` because the public C exports are present.

## Compatibility conclusion

The uploaded archive provides substantially more than source: it contains a Windows x64 component runtime bundle, matching import libraries, SDK runtime DLLs, and a successful Visual Studio build tree. The llama.cpp adapter can therefore use either direct `.lib` linkage or a dynamic function-table loader. A thin adapter is still required to translate `ggml_tensor`, `ggml_backend_buffer_type_t`, backend graph execution, and ggml operation types into these DxAiSt contracts.

The archive includes generated build files and test executables, so the final llama.cpp integration should consume only a clean `include/`, `lib/`, and `bin/` runtime layout. The presence of a DLL and export is not by itself proof of complete GPU inference; the supplied Windows tests and actual target hardware still need to validate D3D12 queue/resource/shader behavior.
