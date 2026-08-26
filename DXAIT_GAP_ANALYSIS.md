# DXAiSt Gap Analysis
## What is missing, incomplete, duplicated, or not production-ready

**Baseline audited:** DxAiSt commit `446e123`
**Target:** A lean CUDA/ROCm-shaped native Windows inference stack with DirectX 12, optional HIP/ROCm, CPU fallback, multimodal pipelines, ReBAR, DirectStorage, precision-tuned kernels, and full performance tracing.

---

## Executive finding

DxAiSt already contains a substantial collection of DirectX 12 experiments and inference-oriented kernels. What it does not yet contain is the **coherent platform stack** that makes those capabilities reusable from arbitrary native applications.

The largest gaps are not another attention variant or another model-specific kernel. They are the common services that CUDA and ROCm users normally receive as a coordinated platform: a stable runtime and ABI, tensor semantics, allocator and residency policy, stream/event semantics, graph compilation, operator registration, model/session boundaries, multimodal preprocessing, packaging, diagnostics, and trustworthy trace data.

The current repository is also too feature-oriented. There is overlap between `dxmem`, `dxchunk`, `dxshard`, `dxcontext`, and `dxstream`; between `dxqueue`, `dxsched`, and `dxgraph`; and between `dxmath`, `dxla`, `dxblas`, `dxattention`, and `dxcache`. These should be consolidated around canonical stack layers.

---

## 1. Gap status legend

| Status | Meaning |
|---|---|
| **Missing** | No usable implementation or public contract exists. |
| **Incomplete** | Some code exists, but it is not general, stable, asynchronous, portable, or production-safe. |
| **Duplicated** | Multiple modules own overlapping responsibilities and should be merged. |
| **Unsafe** | Exists but has correctness, security, lifetime, or operational risks. |
| **Present** | Useful implementation exists and can be retained after interface cleanup. |
| **Replace** | Existing design should not be extended; rebuild behind the new architecture. |

---

## 2. Highest-priority missing platform layers

| Priority | Missing capability | Current evidence | Why it blocks drop-in use | Required action |
|---|---|---|---|---|
| P0 | Versioned runtime/session API | Current C ABI starts at device, queue, buffer, and `dx_la_*` calls | Applications cannot load a model and run a complete session through a stable contract | Add `context`, `device`, `stream`, `tensor`, `model`, `session`, `run_request`, and `run_result` handles |
| P0 | Canonical tensor ABI | Buffer APIs expose bytes but not shape, stride, layout, dtype, quantization, or ownership | Every adapter would invent its own tensor representation | Add one `dxtensor` descriptor and view model for C99 and C++20 |
| P0 | Backend interface | Current code is directly DX12-shaped | HIP/ROCm and CPU cannot implement the same graph/operators cleanly | Define backend vtable/service contracts and capability queries |
| P0 | Stable error model | Global `dx_last_error()` plus raw `HRESULT` | Errors are not composable, thread-safe, or rich enough for adapters | Add structured status, error object, backend code, message, source location, and device-loss diagnostics |
| P0 | Async execution contract | Many APIs hide waits or use implicit submission | Applications cannot overlap input, compute, output, and I/O predictably | Every operation accepts a stream and returns an event/future token |
| P0 | Allocator/residency service | Allocation and streaming logic is spread across modules | No single policy controls VRAM budgets, staging, aliasing, eviction, or ReBAR | Build `dxmemory` with pools, suballocation, workspace, residency, and budget telemetry |
| P0 | Descriptor/resource-state service | Resource and barrier logic is spread through kernels | Each feature can create redundant heaps, barriers, and synchronization | Centralize descriptors, resource states, barriers, and command recording |
| P0 | Operator registry | Operators are called through feature-specific classes | Graphs and adapters cannot select kernels by capability and dtype | Add registry with constraints, implementations, workspace, precision, and trace metadata |
| P0 | Execution-plan compiler | Current graph is mainly dependency/topological support | There is no unified fusion, workspace planning, specialization, or replay model | Add graph IR, compiler, plan cache, specialization key, and replay API |
| P0 | Clean build/package system | CMake hard-codes `E:/DXllama/...` SDK paths | A consumer cannot build or deploy on another machine without editing the project | Use `find_package`, configurable roots, imported targets, runtime discovery, and package config |
| P0 | Trace architecture | `ProfileScope` only wraps command-list events | No end-to-end correlation, GPU timestamp results, memory/I/O events, or export pipeline | Build one cross-layer trace service with disabled/light/full modes |

---

## 3. Complete CUDA/ROCm equivalence gap matrix

| CUDA/ROCm responsibility | Current DxAiSt | Gap | Required new component |
|---|---|---|---|
| Runtime initialization | Partial device construction | No backend registry, version negotiation, or runtime lifetime model | `dxcore`, `dxruntime` |
| Device properties | Adapter enumeration and AMD classification | No stable capability schema across backends | `dxdevice_info`, capability bitset |
| Context management | Direct device objects | No explicit context ownership or thread policy | `dxcontext` redesigned as runtime context, not long-context feature |
| Streams/queues | Queue and multi-queue scheduler | Semantics are not a single portable stream contract | `dxstream` with direct/compute/copy/HIP/CPU implementations |
| Events | Fences exist internally | No public event object, callback, or cross-stream wait contract | `dxevent` |
| Async memcpy | Upload/download helpers | Often hidden synchronization; no dependency token | `dxcopy` plus event return |
| Allocator | Basic buffer creation | No suballocator, pool, aliasing, workspace, or async free | `dxmemory` |
| Unified/managed memory | VRAM/RAM sharding and offload | Not transparent, not backend-neutral, no policy API | Explicit managed-memory policy and residency manager |
| Residency | Sharding/offload modules | No DXGI budget monitoring, priority, eviction, or pressure callbacks | `dxresidency` |
| Descriptor/handle libraries | Per-module root signatures | Duplicated descriptor setup and no global cache | `dxdescriptor` |
| Kernel launch | Direct dispatches | No generic kernel object, argument pack, indirect path, or launch metadata | `dxkernel` |
| Runtime compiler | DXC/JIT exists | No robust reflection/schema, cache versioning, offline artifact pipeline, or invalidation policy | `dxcompile` |
| CUDA/HIP graphs | Partial command graph | No full capture/compile/replay with resource binding | `dxgraph` and `dxplan` |
| BLAS | GEMM/GEMV code exists | Incomplete shape coverage, tuning, layout support, batched/strided APIs, and backend abstraction | `dxblas` rebuilt under `dxops` |
| DNN primitives | Attention/math/conv pieces | No complete convolution/pooling/normalization/operator family | `dxnnops` |
| Tensor contractions | No general tensor contraction API | Multimodal and arbitrary models need generalized layouts and contractions | `dxtensor_ops` |
| Quantization | Q4/Q8 work exists | Limited format coverage, calibration, packing metadata, and fused variants | `dxquant` rebuilt with model-format contracts |
| Matrix acceleration | RDNA-specific shader variants | No generic matrix capability abstraction; tuning is architecture-leaking | `dxmatrix_caps` and variant selector |
| Sparse | CSR SpMV shader exists | No public sparse tensor/format/operator API | `dxsparse` |
| FFT | Radix-2 implementation exists | No batched/strided/real/complex plan API | `dxfft` plan interface |
| RNG | PCG32 implementation exists | No distribution API, reproducibility contract, or stream ownership | `dxrng` |
| Collectives | Ring/all-reduce experiment | No topology discovery, peer access contract, error recovery, or transport abstraction | `dxcollective` |
| Peer memory | Partial multi-GPU concepts | No external handle import/export, peer capability matrix, or lifetime protocol | `dxpeer` |
| Network collectives | TCP/IOCP transport | Not a production collective transport; no TLS/RDMA/flow control | Keep as optional transport adapter, not core |
| Storage-to-GPU | DirectStorage and chunking exist | No provider-neutral I/O contract, cancellation, volume capability cache, or unified trace | `dxio` provider interface |
| GPU decompression | Not clearly exposed as a general service | No compression registry or compressed-weight path | `dxcompress` |
| Model import | GGUF/Safetensors | No ONNX importer or generic model IR boundary | `dxmodel` plus importer adapters |
| Runtime/session | Chat decode harness | No general session, batching, cancellation, reusable input/output binding | `dxinfer` |
| Profiling | Basic event markers | No timestamp query readback and correlation pipeline | `dxtrace`/`dxprofile` |
| Debugging | Some TDR/device-removal handling | No complete DRED, breadcrumb, debug-layer, shader-dump, validation mode | `dxdiagnostics` |
| Tooling | Inspect, bench, ingest tools | No trace viewer/converter, model validator, shader cache tool, or perf database | `dx tools` suite |
| Packaging | Local SDK copy commands | Hard-coded paths and no install/export config | CMake package and deployment manifest |
| Language ABI | Thin C wrapper | Not versioned, not model/session-oriented, includes Windows header | Rebuild C99 ABI from scratch |

---

## 4. Current modules that exist but require replacement or consolidation

| Current area | Assessment | Problem | Target disposition |
|---|---|---|---|
| `dxait.hpp` | Incomplete umbrella | Core types mix device, queue, buffer, fence, config, and guard concerns | Split into public `core`, `device`, `memory`, `stream`, and `diagnostics` APIs |
| `dx_c_api.h` | Too thin and Windows-leaky | Buffer-level API, raw `HRESULT`, global error, no tensor/session/model API | Replace with versioned C99 ABI; keep a compatibility header temporarily |
| `dxadapter` + `dxruntime` | Useful but coupled | Enumeration and construction are not a backend-neutral service | Merge under `dxdevice` with backend implementation below |
| `dxqueue` + `dxsched` | Duplicated | Both manage execution submission and synchronization | Merge under `dxstream`; use one scheduler |
| `dxgraph` + scheduler logic | Partial | Graph dependencies and queue scheduling are split | `dxgraph` owns plan creation; `dxstream` owns execution |
| `dxmem` + `dxchunk` + `dxshard` | Duplicated | Allocation, streaming, and offload policies overlap | Merge core memory services; keep model streaming as a policy layer |
| `dxcontext` + `dxkv` + `dxcache` | Duplicated | Long-context, KV, and cache responsibilities overlap | `dxinfer::kv_cache` plus `dxmemory::cache_policy` |
| `dxstream` current | Ambiguous | Name conflicts with execution streams and expert streaming | Rename expert streaming to `dxio::weight_streamer`; reserve `dxstream` for queues |
| `dxio` + `dxiocp` | Partially duplicated | Storage and network streaming are separate paths without common request ABI | One async I/O interface with DirectStorage, mmap, Win32, and IOCP providers |
| `dxblas` + `dxla` + `dxmath` | Duplicated | Low-precision and math layers overlap with no operator registry | Merge implementations under `dxops`, retain subnamespaces internally |
| `dxattention` + parts of `dxcache` | Feature-specific | Attention owns cache/paging concerns that should be runtime policies | Attention kernels consume generic tensor/KV descriptors |
| `dxtriton` | Optional tool | Runtime generator is too specific and can add complexity | Move to developer tools; generate packaged kernels, never required at runtime |
| `dxmcp` | Application/service layer | MCP/RAG server is not accelerator substrate | Move outside core into examples/adapters |
| `dxnetwork` | Unsafe/optional | XOR payload obfuscation is not production encryption and transport is not a collective library | Reduce to transport interface; use TLS/Windows security or mark trusted-LAN only |
| AMD architecture classification | Useful tuning input | Public interfaces must not expose AMD-only assumptions | Convert to capability profiles and private tuning database |

---

## 5. Missing multimodal components

| Modality | Missing or incomplete components |
|---|---|
| Text-to-text | General tokenizer ABI, vocabulary loaders, BPE/SentencePiece/Unigram support, prompt templates, embeddings, encoder/decoder session split, continuous batching, prefix cache, grammar-constrained decoding, logits processors, stop conditions, streaming callbacks, structured output, and model adapter contracts |
| Text-to-speech | Text normalization, language/phoneme frontend, tokenizer, acoustic model graph, duration/pitch/energy processing, vocoder, resampling, PCM/WAV output, real-time streaming, audio quality validation |
| Text-to-image | Text encoder adapter, tokenizer, latent allocator, diffusion scheduler family, noise generation, U-Net/DiT blocks, cross-attention, classifier-free guidance, VAE decode, image format output, seed/reproducibility, image quality checks |
| Image-to-image | Image decoders, color conversion, resize/crop/normalize, VAE encode/decode, conditioning/mask handling, latent noise injection, denoise strength, scheduler, output encoding |
| Shared media | JPEG/PNG/WebP, WAV/PCM, resampling, color spaces, batching, pinned staging buffers, CPU SIMD preprocessing, cancellation, and traceable host/device conversions |
| All modalities | Model metadata schema, shape specialization, dynamic dimensions, mixed-precision policy, artifact cache, model validation, streaming output, request cancellation, resource quotas, and quality/performance regression tests |

---

## 6. Missing precision and kernel coverage

The current shader set is valuable but narrow and architecture-specific. A production multimodal stack needs a systematic kernel inventory, not a list of successful experiments.

| Area | Missing work |
|---|---|
| GEMM | FP32/FP16/BF16/INT8/INT4 paths, batched and strided variants, transposed layouts, split-K, small-M/N decode GEMV, persistent decode kernels, epilogues, bias/activation fusion, tuning database |
| Convolution | 1D/2D/3D, depthwise/grouped, implicit GEMM, Winograd where useful, NHWC/NCHW layouts, fused activation/normalization |
| Attention | Robust causal/masked SDPA, prefill/decode split, paged KV, GQA/MQA, sliding-window, chunked prefill, flash variants, variable sequence lengths, dropout-disabled inference specialization |
| Normalization | RMSNorm, LayerNorm, GroupNorm, InstanceNorm, fused residual+norm, stable reduction and deterministic option |
| Tensor movement | Transpose, permute, reshape/view, contiguous conversion, gather, scatter, masked select, pad, slice, repeat, broadcast |
| Reductions | Sum, max, min, argmax, norm, prefix scan, segmented reduction, top-k support primitives |
| Quantization | Q4 variants, INT8 per-channel, groupwise scales, zero points, activation quantization, calibration, packing/unpacking, fused dequantization |
| Diffusion | Residual blocks, convolutional blocks, cross-attention, timestep embeddings, VAE operations, latent transforms, classifier-free guidance fusion |
| Audio | FFT plans, windowing, mel/filterbanks, normalization, upsampling, vocoder primitives, overlap-add |
| RNG | Counter-based Philox-like generator or equivalent, distributions, deterministic cross-backend mode, per-request seed state |
| Sparse | CSR/COO/blocked formats, sparse matmul, pruning metadata, dispatch policy |
| CPU fallback | Scalar correctness kernels plus SIMD implementations for core ops, with the same numerical test suite |
| Kernel lifecycle | Source metadata, reflection, artifact versioning, specialization constants, PSO/HSACO cache, invalidation, offline compilation, reproducible builds |

---

## 7. Missing ReBAR and DirectStorage engineering

### ReBAR

The current repository discusses large VRAM/RAM transfers but does not expose a complete ReBAR/GPU-upload-memory policy service. Missing pieces are detection, effective mapping-size measurement, allocation strategy selection, CPU cache/coherence rules, synchronization rules, per-workload benchmarking, and a fallback that is automatically selected when mapped reads are slower than staged copies.

### DirectStorage

The current DirectStorage path needs a provider-neutral request model, volume capability discovery, queue and allocator ownership, cancellation, partial-read handling, decompression integration, staging policy, prefetch/eviction feedback, and unified trace events. It must support a safe Win32 overlapped-I/O fallback and must not serialize unrelated requests through a single persistent command allocator.

---

## 8. Missing tracing and performance engineering

The current `ProfileScope` emits command-list event markers, but that is not a complete performance trace. The following are missing or need implementation:

| Trace capability | Required behavior |
|---|---|
| Request correlation | One ID across API call, graph plan, I/O, queues, kernels, outputs, and errors |
| CPU spans | Thread ID, start/end, nesting, allocator and submission time |
| GPU spans | Timestamp queries per queue, frequency calibration, CPU/GPU clock correlation |
| Queue timeline | Submit, start/complete, allocator, command-list, fence values, wait duration |
| Memory timeline | Allocation/free, heap, residency, eviction, budget, BAR mode, high-water mark |
| Transfer trace | Disk-to-RAM, RAM-to-upload, upload-to-VRAM, peer transfer, bytes, bandwidth, overlap |
| Kernel trace | Op, shader/PSO hash, dispatch dimensions, dtype, variant, workspace, estimated FLOPs |
| Compiler/cache trace | Shader compile, PSO creation, plan compilation, cache hit/miss, invalidation reason |
| Graph trace | Fusion groups, temporary allocations, barriers, dispatch count, replay count |
| Quality trace | Seed, checksum, reference comparison, tolerance, output statistics |
| Export | Chrome Trace JSON, binary trace, CSV/JSON aggregates, human-readable summary |
| Overhead measurement | Benchmark tracing off/light/full/diagnostic modes and publish overhead |
| Counter integration | Optional PIX/ETW, vendor tools, and backend profiler adapters without hard dependency |

The tracing system must be non-invasive. In disabled mode it should not allocate, format strings, take locks, or create GPU queries on the hot path.

---

## 9. Missing production engineering

| Area | Missing work |
|---|---|
| Thread safety | Define which handles are thread-safe, stream-affine, or externally synchronized |
| Lifetime | Reference-counted or explicit ownership rules for resources, descriptors, plans, model chunks, and events |
| Device loss | DRED/breadcrumb capture, error propagation, context invalidation, restart/reload policy |
| TDR control | Work partitioning, watchdog-safe kernels, timeout diagnostics, and recovery tests |
| Security | Remove default secrets, replace XOR transport for anything beyond trusted LAN, validate model inputs, avoid path traversal in loaders |
| Determinism | Seeded RNG, deterministic reductions where requested, cross-backend tolerance policy |
| Model safety | Bounds checking, overflow checking, malformed-file rejection, decompression limits, memory quota enforcement |
| ABI evolution | Struct sizes, version negotiation, symbol stability, deprecation policy, binary compatibility tests |
| Build portability | No developer paths; Visual Studio, clang-cl, CMake presets, optional SDK discovery, clean-machine builds |
| Continuous integration | CPU/WARP or software validation path, shader compilation checks, static analysis, sanitizer-compatible host tests |
| Documentation | API reference, integration guide, backend support matrix, performance methodology, trace interpretation guide |
| Licensing | Third-party model/parser/kernel license inventory and redistribution policy |
| Release process | Semantic versioning, artifact manifest, symbol map, reproducible build metadata, cache compatibility policy |

---

## 10. Missing tools and adapters

| Tool/adapter | Why needed |
|---|---|
| `dxait-inspect` | Enumerate adapters, capabilities, ReBAR mode, queues, memory budgets, backend availability, and SDK versions |
| `dxait-bench` | Run reproducible operator, graph, I/O, and end-to-end benchmarks with trace IDs |
| `dxait-trace` | Convert, summarize, filter, and compare traces; produce bottleneck reports |
| `dxait-shaderc` | Offline HLSL compilation, reflection, variant manifest generation, and cache packaging |
| `dxait-modelcheck` | Validate GGUF, Safetensors, ONNX, tensor bounds, dtype support, and memory estimates |
| `dxait-pack` | Build deployment bundles containing DLLs, shader artifacts, manifests, and optional backend libraries |
| ONNX Runtime adapter | Drop-in execution-provider-style integration for general model graphs |
| llama.cpp adapter | Reuse established GGUF/model and sampling ecosystem through the new tensor/backend contract |
| Native C sample | Demonstrate zero-C++ integration through the C99 ABI |
| C++ sample | Demonstrate graph/session APIs, custom operators, tracing, and streaming |
| HIP sample | Demonstrate the same model/graph contract through HIP/ROCm when installed |
| Multimodal samples | One minimal example for each requested pipeline |

---

## 11. What can be retained

| Existing asset | Keep? | Conditions |
|---|---|---|
| D3D12 adapter/device code | Yes | Move behind `dxdevice`; remove hard-coded assumptions and expose capability schema |
| Queue/fence implementation | Yes, rewrite | Adopt unified stream/event contract and allocator reuse |
| Barrier helper | Yes, rewrite | Centralize state tracking and support enhanced-barrier path when available |
| DXC/JIT | Yes, split | Keep compiler service; move generator tooling out of runtime hot path |
| GEMM/GEMV kernels | Yes, benchmark/rewrite | Add portable baseline, precision declarations, tuning metadata, and cross-vendor variants |
| Attention kernels | Yes, rewrite interfaces | Make them consume generic tensor/KV descriptors and support prefill/decode specialization |
| Quantization | Yes, expand | Add format metadata, packing validation, calibration, and fused paths |
| GGUF/Safetensors | Yes | Add safe bounds checking and generic model/tensor reader interface |
| DirectStorage | Yes, refactor | Unify with Win32 and IOCP provider API; add cancellation and traceability |
| Sharding/offload | Yes, merge | Make it a memory/residency policy rather than a separate model subsystem |
| Speculative decoding | Yes | Rebuild above generic session/graph/sampling APIs |
| FFT/RNG/sparse | Yes, promote selectively | Give each a plan/descriptor API and shared operator registry |
| Tools/tests | Yes, reorganize | Convert harnesses into layered unit, numerical, integration, stress, and benchmark suites |
| Network/MCP/RAG | Optional | Move out of core accelerator stack into adapters/examples |

---

## 12. Recommended implementation order

1. **Freeze and document the current repository as the inventory branch.** Do not add more feature modules to the current layout.
2. **Create the new core packages:** `dxcore`, `dxdevice`, `dxmemory`, `dxstream`, `dxtensor`, `dxtrace`.
3. **Replace the C ABI** with versioned runtime/device/stream/tensor/model/session handles.
4. **Build the operator registry and graph/plan compiler** using only a small P0 operator set.
5. **Port the best existing GEMM, norm, softmax, RoPE, attention, quantization, and KV kernels** behind the new contracts.
6. **Implement ReBAR-aware memory selection and DirectStorage/Win32 provider parity** with trace instrumentation.
7. **Add text-to-text first** as the vertical integration proof, then speech and diffusion pipelines using the same graph/tensor foundation.
8. **Add HIP/ROCm and CPU backends** only after the backend contracts and reference tests are stable.
9. **Add tuning, optional extensions, adapters, packaging, and full performance regression gates.**

---

## 13. Bottom line

The missing part is not primarily more algorithms. It is the **platform-grade connective tissue** that makes algorithms reusable: stable ABI, tensors, memory, streams, events, graphs, operator registration, backend selection, model/session boundaries, I/O policies, tracing, diagnostics, packaging, and adapters.

The current code can supply a meaningful portion of the first kernel inventory, but the runtime should be rebuilt around the CUDA/ROCm stack model. Any module that cannot fit cleanly into one of those canonical layers should be demoted to an optional adapter or removed from the core.

## References

[1]: https://docs.nvidia.com/cuda/ "CUDA Toolkit Documentation"

[2]: https://developer.nvidia.com/cupti "NVIDIA CUDA Profiling Tools Interface"

[3]: https://www.amd.com/en/developer/resources/rocm-hub/hip-sdk.html "AMD HIP SDK for Windows"

[4]: https://rocm.docs.amd.com/projects/install-on-windows/en/latest/ "ROCm HIP SDK on Windows"

[5]: https://rocm.docs.amd.com/projects/rocBLAS/en/docs-5.1.3/Windows_Install_Guide.html "rocBLAS Windows Installation Guide"

[6]: https://gpuopen.com/learn/using-d3d12-heap-type-gpu-upload/ "GPUOpen: Using D3D12 Heap Type GPU Upload"

[7]: https://microsoft.github.io/DirectX-Specs/ "Microsoft DirectX Specifications"
