# DXAiSt Current Gap Report

**Audited source state:** current working tree packaged as `DxAiSt_revamp_source.zip`
**Implemented slice:** `dx12_dxcore` logger/runtime foundation
**Validation host:** Linux sandbox; Windows/GPU validation deferred

## Executive summary

The ZIP is a codebase foundation, not a complete CUDA/ROCm replacement yet. The current implementation has working portable DLL slices for `dxcore`, `dxruntime`, `dxdevice`, `dxmemory`, `dxstream`, `dxtensor`, `dxgraph`, `dxtrace`, `dxio`, and `dxops`. These provide C99 contracts, host validation paths, Windows DX12 hooks, timestamped logs, and a ten-test portable suite. They are not yet a complete inference runtime or production GPU backend.

Everything below the implemented contracts remains incomplete, duplicated, or deferred; the new components still need production model/session wiring and full Windows/GPU validation. The existing DxAiSt modules are still present in their original feature-oriented organization; they have not yet been migrated behind the new stack contracts.

## 1. What is implemented now

| Component | State | Verified |
|---|---|---|
| `dx12_dxcore` shared target | In progress | CMake/Ninja host build succeeds |
| C99 logger ABI | Implemented | `test_dxcore_logging` passes |
| C++20 component logger | Implemented | Host test and generated logs verified |
| DLL-name environment controls | Implemented | `dx12_dxcore_*` and `dx12_dxmemory_*` tested |
| Timestamped local logs | Implemented | `build-core/logs/<component>_YYYYMMDD_HHMMSS_PID.log` verified |
| Independent debug/trace/perf/diag modes | Implemented | Mode bitset and enable checks tested |
| Portable runtime handle | Initial foundation | Backend metadata test passes; no DX12 device yet |
| Status ledger | Implemented | `docs/COMPONENT_STATUS.md` |
| Completion audit rules | Implemented | `docs/COMPLETION_AUDIT.md` |
| Truth table for logging | Implemented | `docs/TRACE_DEBUG_TRUTH_TABLE.md` |
| Cross-platform build prerequisites | Installed in sandbox | CMake, Ninja, Clang/LLVM, LLD, GCC/G++, MinGW |

## 2. Critical missing components

| Priority | Missing component | Consequence |
|---|---|---|
| P0 | Full DX12 execution backend | Device creation hooks exist, but no command-list/operator execution path |
| P0 | Versioned runtime/session lifecycle | No generic model-to-inference integration |
| P0 | Canonical tensor ABI | Buffers still lack shape/stride/layout/dtype semantics |
| P0 | Backend interface | HIP/ROCm and CPU cannot implement the same public contract |
| P0 | Memory allocator and residency manager | No unified suballocation, workspace, VRAM budget, eviction, aliasing, or ReBAR policy |
| P0 | Public streams/events | No portable async execution or cross-queue dependencies |
| P0 | Descriptor and resource-state service | Resource binding/barriers remain scattered in legacy code |
| P0 | Operator registry | No capability/dtype/shape-based kernel selection contract |
| P0 | Graph IR and execution-plan compiler | No unified fusion, specialization, workspace planning, or replay |
| P0 | Model/session API | Model file detection exists, but no model metadata import or execution session |
| P0 | Windows SDK/redistributable validation | Discovery module and vendor headers exist; Windows-host validation remains pending |
| P0 | GPU timestamp trace pipeline | Current core logging is host-side; it is not yet GPU performance tracing |

## 3. Backend gaps

### DirectX 12

The new `dx12_dxdevice` can enumerate DXGI adapters and open a native D3D12 device on Windows. `dx12_dxstream` has native command-queue/fence hooks, and `dx12_dxmemory` has committed-resource hooks. Missing work is descriptor allocation, resource-state/barrier ownership, command allocators/lists, shader compilation, PSO caching, dispatch submission, device removal recovery, and actual operator execution behind `dxops`.

### HIP/ROCm

There is no HIP/ROCm backend in the new structure. Missing work includes HIP runtime/device discovery, streams/events, device and pinned memory, peer access, kernel compilation, rocBLAS/hipBLAS adapters, MIOpen/hipDNN adapters where appropriate, RCCL collectives, rocFFT/rocRAND/rocSPARSE adapters, HIP graph support, and Windows SDK detection. HIP/ROCm support must be optional and capability-probed; it cannot be assumed merely because the host is AMD.

### CPU

There is no new CPU backend implementing the same tensor/operator/graph contracts. A CPU reference backend is required for correctness tests and unsupported-operator fallback, with optional SIMD implementations for common operations.

## 4. Runtime and framework gaps

| Area | Missing work |
|---|---|
| Tensor | Shape, strides, views, layouts, dtype conversion, quantization metadata, device ownership, aliasing, external memory import/export |
| Graph | Backend-neutral IR, validation, fusion, constant folding, dynamic-shape specialization, plan cache, replay |
| Operators | Registry, constraints, workspace declarations, deterministic modes, precision contracts, fallback selection |
| Sessions | Prefill/decode split, batching, continuous batching, cancellation, streaming outputs, quotas, request correlation |
| Model loading | Safe generic reader exists; ONNX importer, richer GGUF/Safetensors metadata, malformed-file validation, memory estimates remain |
| Memory | Pools, suballocation, staging, readback, workspace, residency, eviction, budget pressure callbacks, ReBAR mode selection |
| Streams | Direct/compute/copy queues, events, fence timelines, command allocator pools, dependency scheduling |
| Errors | Thread-safe structured error objects, backend codes, source location, device-loss report, no global last-error state |
| ABI | Struct versioning, symbol stability, binary compatibility tests, allocator callbacks, deprecation policy |

## 5. Kernel and library gaps

The new `dx12_dxops` now has portable reference copy, fill, and FP32 GEMM kernels. It still lacks native DX12/HIP dispatch and nearly all of the complete kernel coverage: GEMV/batched GEMM precision families, BF16/INT8/INT4 paths, convolution variants, pooling, normalizations, tensor movement, reductions, scans, top-k, embedding, gather/scatter, fused MLP, robust attention variants, diffusion blocks, audio feature extraction, vocoders, and deterministic RNG.

The following are also missing from the kernel lifecycle: offline compilation, shader reflection manifests, specialization constants, PSO cache invalidation, artifact versioning, reproducible shader builds, cross-vendor baseline kernels, SM 6.7 tuning profiles, capability-gated newer extensions, CPU reference comparisons, and shape-specific autotuning.

## 6. ReBAR and DirectStorage gaps

The new `dx12_dxio` provides a provider-neutral ranged-read API and portable fallback. Missing work includes DirectStorage capability discovery, queue ownership, cancellation, decompression, prefetch depth, ring buffers, eviction feedback, allocator ownership, I/O/compute overlap, and unified I/O traces.

The ReBAR path still needs detection, effective mapping-size measurement, cache/coherence rules, GPU-visible upload allocation, workload-specific bandwidth probes, weight/activation/KV policy selection, and automatic fallback when mapped reads underperform staged copies. ReBAR must improve performance without becoming a correctness requirement.

## 7. Tracing and performance gaps

The logger currently records timestamped host records. It does not yet provide:

- Request and graph correlation across DLLs.
- GPU timestamp queries with CPU/GPU clock calibration.
- Queue submit/start/complete timelines.
- Fence wait and synchronization stall durations.
- Allocation, residency, eviction, and VRAM-budget timelines.
- Disk-to-RAM-to-upload-to-VRAM transfer records.
- Shader/PSO/plan cache hit and compile records.
- Kernel variant, dispatch dimensions, dtype, workspace, and estimated FLOP metadata.
- PIX/ETW/vendor profiler integration.
- Chrome Trace JSON, binary trace, CSV aggregate, and comparison tooling.
- Measurement of tracing overhead in off/light/full/diagnostic modes.

## 8. Multimodal gaps

| Pipeline | Missing work |
|---|---|
| Text-to-text | Tokenizers, model adapters, prompt templates, batching, KV cache, sampling, grammar constraints, streaming, structured output |
| Text-to-speech | Text normalization, phoneme frontend, acoustic model, vocoder, resampling, PCM/WAV output, realtime streaming |
| Text-to-image | Text encoder, latent initialization, diffusion schedulers, U-Net/DiT, cross-attention, VAE decoder, image encoding |
| Image-to-image | Image decode/normalize, VAE encode, conditioning/masks, latent noise, denoising, VAE decode, output encoding |
| Shared media | JPEG/PNG/WebP, WAV/PCM, color conversion, resampling, CPU SIMD preprocessing, host/device staging |

## 9. Production and packaging gaps

The codebase now has configurable SDK-root discovery, Windows presets, and a vendored DirectX-Headers source with license notice. It still needs clean-machine Windows validation, imported SDK targets, DLL deployment manifests, shader artifact packaging, runtime DLL discovery, install/export targets, symbol/version reports, and release metadata.

It also needs thread-safety rules, lifetime tests, device-loss recovery, TDR-safe work partitioning, model parser fuzzing, overflow checks, decompression limits, path validation, security cleanup for the existing trusted-LAN XOR transport, static analysis, sanitizer-compatible host tests, WARP/software validation, and CI.

## 10. Current duplicated or unconnected areas

| Existing modules | Required action |
|---|---|
| `dxqueue`, `dxsched`, `dxgraph` | Merge responsibilities into stream scheduling plus graph planning |
| `dxmem`, `dxchunk`, `dxshard`, `dxcontext` | Consolidate under memory/residency/I/O policy layers |
| `dxstream` expert streaming and execution streams | Rename expert streaming; reserve `dxstream` for queues/events |
| `dxio`, `dxiocp`, network streaming | Unify through an async I/O provider interface |
| `dxblas`, `dxla`, `dxmath`, `dxattention`, `dxcache` | Move implementations behind `dxops` and canonical tensor contracts |
| `dxtriton` | Keep as optional offline/developer kernel-generation tooling |
| `dxmcp`, RAG, network services | Move outside the core accelerator runtime |
| Existing C API | Keep only as temporary compatibility surface; replace with versioned runtime/tensor/session ABI |

## 11. Next build queue

1. Add descriptor/resource-state ownership and command allocator/list management.
2. Add GPU timestamp queries and correlate them with `dxtrace` spans.
3. Add C++20 wrappers and production model/session wiring.
4. Expand dxops into native DX12/HIP dispatch with precision kernels.
5. Integrate DirectStorage queues, compression, and I/O/compute overlap.
6. Add ReBAR detection and residency policy.
7. Add model adapters and the first end-to-end text-to-text path.
8. Add Visual Studio/clang-cl presets and validate the Windows SDK/redistributable bundle on a Windows host.
9. Finish model metadata import and add the first end-to-end text-to-text session.
10. Add HIP/ROCm and multimodal pipelines after the contracts stabilize.

## Final verdict

The foundation is real and tested, but the project is still at the beginning of the revamp. The largest remaining work is the **runtime connective tissue and backend contracts**, followed by operator coverage and Windows hardware validation. The current package is suitable for continuing the codebase assembly and for a Windows developer to begin SDK/build integration; it is not yet a complete drop-in CUDA/ROCm-equivalent inference stack.
