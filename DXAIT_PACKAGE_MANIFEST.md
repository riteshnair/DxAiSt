# DXAiSt source package manifest

**Package purpose:** transfer the current revamp source tree to a Windows laptop for native DX12 build work.
**Repository baseline:** `riteshnair/DxAiSt` / `Maxritz/DxAiSt`
**Source commit baseline:** `446e123`
**Package state:** includes the original repository inventory plus the new `dx12_dxcore` foundation.

## Included

| Directory/file group | Contents |
|---|---|
| `include/` | Existing DxAiSt public headers plus new C99 DLL-oriented runtime, device, memory, stream, tensor, graph, trace, I/O, operator, model, and shader headers plus `dx_native.hpp` |
| `src/` | Existing source modules plus new `src/core` logger, runtime, device, memory, stream, graph, trace, I/O, operator, model, and shader implementations |
| `src/shaders/` | Existing HLSL shader inventory |
| `tests/` | Existing tests plus portable tests for every new core DLL contract |
| `tools/` | Existing inspection, benchmark, and ingest tools |
| `docs/` | Component matrix, gap analysis, DLL logging contract, status ledger, completion audit, truth table |
| `cmake/` | Windows SDK and bundled-or-system SDK discovery module |
| `CMakePresets.json` | Reproducible Windows core and legacy build presets |
| `vendor/DirectX-Headers/` | Pinned MIT-licensed Microsoft DirectX-Headers source |
| `vendor/` notices | Third-party license and redistribution boundaries |
| `CMakeLists.txt` | Legacy build plus optional `dx12_dxcore` shared-library target |
| `README.md` and project guidance | Existing project documentation and contribution rules |
| `docs/BUILD_WINDOWS.md` | Windows toolchain, SDK, runtime, and log instructions |
| `DXAIT_PACKAGE_MANIFEST.md` | This package manifest |

## Excluded

Generated build directories (`build/`, `build-core/`), `.git/` metadata, generated logs, temporary benchmark output, and local SDK binaries are excluded. The package contains the vendored MIT-licensed DirectX-Headers source. It does not yet contain Agility, DXC, DirectStorage, HIP, or ROCm binaries because those SDK assets have not been staged in the sandbox. The package layout accepts approved redistributables under `vendor/AgilitySDK`, `vendor/DXC`, `vendor/DirectStorage`, and `vendor/HIP` with corresponding notices and version manifests.

## Current build options

For portable validation of the new core only:

```powershell
cmake -S . -B build-core -G Ninja `
  -DDXAIT_BUILD_LEGACY=OFF `
  -DDXAIT_BUILD_REVAMP_CORE=ON `
  -DDXAIT_BUILD_CORE_TESTS=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

For the full Windows build, install Visual Studio 2022 with C++ desktop development and a current Windows SDK. Windows headers and system libraries are resolved by the selected Windows toolchain. The CMake file accepts bundled or explicit roots for Agility, DXC, DirectStorage, and HIP/ROCm, and can copy approved redistributable runtime files when they are present.

## Verified in the sandbox

- The new core DLL targets configured and built with CMake/Ninja.
- `CMakePresets.json` and `docs/BUILD_WINDOWS.md` provide the Windows build path.
- All thirteen portable component tests passed: `test_dxcore_logging`, `test_dxruntime`, `test_dxtensor_desc`, `test_dxdevice`, `test_dxmemory`, `test_dxstream`, `test_dxgraph_api`, `test_dxtrace_api`, `test_dxio`, `test_dxops`, `test_dxmodel`, `test_dxshader`, and `test_dxnative`.
- DLL-scoped variables were verified for `dx12_dxcore_debug`, `dx12_dxcore_trace`, `dx12_dxcore_perf`, and `dx12_dxmemory_debug`.
- New source-side components include `dxruntime`, `dxdevice`, `dxmemory`, `dxstream`, `dxtensor`, `dxgraph`, `dxtrace`, `dxio`, `dxops`, `dxmodel`, and `dxshader`, plus the `dx_native.hpp` C++20 facade.
- Timestamped log files were generated under `build-core/logs/` during the host test.

## Deferred to Windows

- D3D12 device creation and adapter enumeration.
- HLSL/DXC compilation and DXIL validation.
- DirectStorage runtime and compression shader validation.
- ReBAR/GPU-upload memory measurements.
- SM 6.7/6.9/preview extension behavior.
- HIP/ROCm backend compilation and AMD hardware execution.
- PIX/GPU timestamp integration.
- Full legacy DxAiSt build and GPU test suite.
