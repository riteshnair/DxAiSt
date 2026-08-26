# Windows build and deployment

## Toolchain

Use Visual Studio 2022 with the Desktop development with C++ workload, a current Windows SDK, and an x64 native tools prompt. The project resolves Windows SDK headers and system libraries through the selected Visual Studio toolchain. It does not require copying traditional Windows SDK headers into the repository.

The optional redistributable SDK roots are selected in this order:

1. `vendor/AgilitySDK`, `vendor/DXC`, `vendor/DirectStorage`, and `vendor/HIP` when those directories exist.
2. Explicit CMake cache variables.
3. Environment variables with the same names.
4. System/toolchain installation where supported.

The explicit variables are:

```text
DXAIT_WINDOWS_SDK_ROOT
DXAIT_AGILITY_SDK_ROOT
DXAIT_DXC_SDK_ROOT
DXAIT_DIRECTSTORAGE_SDK_ROOT
DXAIT_HIP_SDK_ROOT
```

## Core configuration

```powershell
cmake -S . -B build-windows -G "Visual Studio 17 2022" -A x64 `
  -DDXAIT_BUILD_LEGACY=OFF `
  -DDXAIT_BUILD_REVAMP_CORE=ON `
  -DDXAIT_BUILD_CORE_TESTS=ON
cmake --build build-windows --config Release
ctest --test-dir build-windows -C Release --output-on-failure
```

The legacy inventory target remains available for migration work but is not required for the new DLL foundation.

## Runtime files

When approved redistributable files are present under the configured Agility, DXC, or DirectStorage roots, the CMake post-build deployment helper copies them beside the target binary according to the package layout. The helper does not copy Windows OS D3D12 runtime files or traditional Windows SDK headers.

## Debug and performance controls

Variables are keyed by DLL component name:

```text
dx12_dxcore_debug=1
dx12_dxruntime_debug=1
dx12_dxdevice_diag=1
dx12_dxmemory_perf=1
dx12_dxstream_trace=1
dx12_dxgraph_trace=1
dx12_dxtrace_perf=1
```

Log files are written to:

```text
<dll-directory>\logs\<component>_YYYYMMDD_HHMMSS_PID.log
```

Each DLL reads only its own variables. Disabled modes avoid file creation and message formatting at the component call site as far as the public API allows. GPU timestamps are not yet included in the portable foundation; they will be added to the same trace schema by the DX12 backend.

## Current expected status

The current source tree should build the portable contracts and tests. The following require a Windows machine with the relevant SDKs and hardware: DXGI enumeration, D3D12 device creation, D3D12 queues and fences, committed resources, Agility runtime loading, DXC compilation, DirectStorage I/O, ReBAR measurements, and HIP/ROCm execution.
