# DXAiSt Revamp
## CUDA/ROCm-shaped Native Windows Inference Stack

**Status:** Proposed architecture and implementation specification
**Repository baseline:** `riteshnair/DxAiSt` / `Maxritz/DxAiSt`, commit `446e123`
**Primary targets:** Windows 10/11 x64, C++20 implementation, C99 ABI
**Backends:** Native DirectX 12 first; optional HIP/ROCm; CPU reference/fallback
**Modalities:** Text-to-text, text-to-speech, text-to-image, image-to-image
**Performance requirements:** Lean execution path, explicit memory movement, ReBAR-aware residency, DirectStorage ingestion, precision-tuned SM 6.7 kernels, full GPU/CPU performance tracing

---

## 1. Purpose

DXAiSt is to be rebuilt as a native Windows accelerator stack shaped like the CUDA and ROCm ecosystems, but with a clean DirectX 12 implementation as its primary Windows backend. The project is not to remain a collection of loosely related model features. It is to become a reusable portability kit that lets a native application adopt GPU inference without rebuilding the runtime services normally supplied by Python, PyTorch, CUDA, ROCm, cuBLAS, rocBLAS, cuDNN, MIOpen, NCCL, RCCL, CUDA Graphs, framework allocators, model loaders, profilers, and packaging systems.

The existing repository is a source inventory. Useful kernels and subsystems may be retained, rewritten, or discarded. The existing directory structure is not an architectural constraint.

> **Core principle:** implement one canonical abstraction per concern, keep backend-specific code behind explicit capability contracts, and make every optimization observable through trace data.

---

## 2. Design goals and non-goals

### 2.1 Goals

| Goal | Requirement |
|---|---|
| Native integration | A consumer can link a DLL, static library, or CMake package and run inference without Python. |
| Stable ABI | The C99 ABI uses opaque handles, versioned structs, explicit ownership, status codes, and no C++ types. |
| C++ ergonomics | The C++20 API adds RAII, typed descriptors, spans, graph builders, structured errors, and safe defaults. |
| Backend neutrality | Public code selects capabilities and policies, not vendor names or shader filenames. |
| DX12 performance | The fast path uses explicit D3D12 resources, queues, barriers, descriptors, fences, and command reuse. |
| HIP/ROCm reach | An optional HIP backend maps the same runtime, tensor, graph, and operator contracts to HIP/ROCm where installed and supported. |
| Cross-vendor operation | NVIDIA, AMD, and Intel DX12 adapters use the same public API with capability-gated kernel variants. |
| ReBAR awareness | The runtime detects and benchmarks large BAR/GPU-upload access; it never assumes ReBAR is enabled. |
| DirectStorage | Model-weight ingestion supports DirectStorage where available and an overlapped Win32 fallback where it is not. |
| Precision control | Dtypes, accumulation modes, quantization schemes, and numerical tolerances are explicit and testable. |
| Full tracing | Every inference can emit CPU spans, GPU timestamp spans, queue dependencies, memory transfers, kernel selection, and counters. |
| Lean structure | No duplicate tensor abstractions, no separate scheduler per feature, no model-specific runtime fork, and no mandatory generic graph framework on the hot path. |

### 2.2 Non-goals

The project will not attempt to reproduce every CUDA or ROCm API byte-for-byte. It will provide a practical semantic compatibility layer for the parts needed by native inference systems. It will not make DirectML the core execution engine. DirectML may be an optional compatibility provider for unsupported operators, but it must not define the runtime architecture or performance model.

---

## 3. Proposed stack layout

The repository should be reorganized into layers that correspond to the major CUDA/ROCm stack responsibilities.

```text
DxAiSt/
├── cmake/
├── include/dxait/
│   ├── dxait.h                 # umbrella C99 ABI
│   ├── dxait.hpp               # C++20 API
│   ├── runtime.h               # runtime and backend discovery
│   ├── device.h                # devices, capabilities, topology
│   ├── memory.h                # allocations, residency, mappings
│   ├── stream.h                # queues, events, dependencies
│   ├── tensor.h                # tensor descriptors and views
│   ├── graph.h                 # IR, capture, compilation, replay
│   ├── operator.h              # operator registry and dispatch
│   ├── model.h                 # model and tensor-file loading
│   ├── inference.h             # sessions, batching, cancellation
│   ├── trace.h                 # tracing and profile export
│   ├── io.h                    # DirectStorage and Win32 I/O
│   ├── multimodal.h            # text, audio, image, diffusion APIs
│   └── version.h
├── src/
│   ├── core/                   # handles, status, config, logging, utilities
│   ├── runtime/                # backend-neutral runtime
│   ├── backend/dx12/           # D3D12 device, queues, resources, descriptors
│   ├── backend/hip/             # optional HIP/ROCm implementation
│   ├── backend/cpu/             # scalar/SIMD reference implementation
│   ├── memory/                 # allocator, residency, workspace, staging
│   ├── stream/                 # stream/event graph and command scheduling
│   ├── tensor/                 # descriptors, views, layouts, conversion
│   ├── graph/                  # IR, planning, fusion, specialization, replay
│   ├── ops/                    # canonical operator implementations
│   ├── math/                   # BLAS-like and tensor primitives
│   ├── quant/                  # quantization and packed-weight utilities
│   ├── model/                  # GGUF, Safetensors, ONNX, generic loaders
│   ├── inference/              # sessions, batching, KV, sampling
│   ├── multimodal/             # text, speech, image, diffusion pipelines
│   ├── io/                     # DirectStorage, overlapped I/O, mmap, cache
│   ├── collective/             # multi-GPU and peer-transfer primitives
│   ├── trace/                  # CPU/GPU spans, counters, exporters
│   ├── tools/                  # inspect, benchmark, trace conversion
│   └── compat/                 # optional DirectML and ecosystem adapters
├── shaders/dx12/               # HLSL 6.x, shared includes, generated variants
├── kernels/hip/                # HIP C++ kernels, optional ROCm path
├── kernels/cpu/                # reference and SIMD kernels
├── adapters/                   # ONNX Runtime, llama.cpp, application adapters
├── tests/                      # unit, numerical, integration, stress, perf
├── samples/                    # one sample per integration level
└── docs/
```

The old `dxadapter`, `dxruntime`, `dxqueue`, and `dxmem` source-only split should be replaced by coherent public modules. Feature modules such as `dxcontext`, `dxcache`, `dxkv`, `dxstream`, and `dxchunk` should be merged where they represent one responsibility. The target is not a one-header-per-feature rule; the target is a small number of stable architectural boundaries.

---

## 4. Full CUDA/ROCm stack mapping

The table below defines the target semantic surface. “Implement” means native DxAiSt code. “Wrap” means adapt an existing backend library behind the DxAiSt contract. “Optional” means the capability is useful but not required for the baseline.

| Stack layer | CUDA analogue | ROCm analogue | DxAiSt target | DX12 implementation | HIP/ROCm implementation | Priority |
|---|---|---|---|---|---|---|
| Driver/runtime discovery | CUDA Driver/Runtime API | HIP runtime and ROCr/HSA layer | `runtime`, `device` | DXGI adapter enumeration, D3D12 device creation, feature negotiation | HIP device discovery and runtime initialization | P0 |
| Device properties | `cudaGetDeviceProperties` | `hipGetDeviceProperties` | `device_info` and capability bitset | Vendor-neutral D3D12 caps plus adapter metadata | HIP properties plus architecture metadata | P0 |
| Context ownership | CUDA context | HIP context/device state | `context` | One explicit device context with thread-safe ownership policy | One HIP device context per runtime | P0 |
| Streams/queues | CUDA streams | HIP streams | `stream` | Direct/compute/copy queues, command allocators, fence timeline | HIP streams and events | P0 |
| Events/timing | CUDA events | HIP events | `event`, `timestamp` | Fence values and timestamp queries | HIP events and profiling timestamps | P0 |
| Synchronization | stream waits, host callbacks | stream/event waits | dependency primitives | Fence waits, queue waits, CPU wait policy | HIP event/stream waits | P0 |
| Memory allocation | `cudaMalloc`, async allocator | `hipMalloc`, async allocator | `memory` | Heaps, placed resources, suballocation, upload/readback, GPU upload heaps | Device/managed/pinned allocations | P0 |
| Memory copies | `cudaMemcpyAsync` | `hipMemcpyAsync` | `copy`, `fill`, `cast` | Copy queue, compute copy, ReBAR-aware mapped paths | HIP memcpy and peer copies | P0 |
| Unified/managed memory | CUDA Unified Memory | HIP managed memory | `managed_policy` | Explicit residency and migration; no false promise of transparent UVM | HIP managed memory where supported | P1 |
| Memory residency | CUDA memory advise/prefetch | HMM/prefetch policies | `residency` | DXGI budget, MakeResident/Evict, priority, page policy | Backend-specific residency policy | P0 |
| Descriptors | CUDA handles | HIP library handles | `descriptor_cache` | Descriptor heaps, bindless/indexed descriptors, cached tables | Library handles and argument packing | P0 |
| Kernel launch | `<<<grid, block>>>` | HIP kernel launch | `kernel_dispatch` | Root signatures, PSOs, dispatch/indirect dispatch | HIP launch configuration | P0 |
| Kernel compilation | NVCC/NVRTC | HIPCC/clang | `compiler` | DXC offline/runtime HLSL compilation, reflection, PSO cache | HIPCC/clang or prebuilt kernels | P0 |
| Graph execution | CUDA Graphs | HIP Graphs | `graph` and `execution_plan` | Capture command sequences, specialize, cache, replay | HIP graph capture/replay | P1 |
| BLAS | cuBLAS | rocBLAS/hipBLAS | `math` / `blas` | Native GEMM/GEMV/batched GEMM kernels, optional vendor path | rocBLAS/hipBLAS adapter plus native fallback | P0 |
| DNN primitives | cuDNN | MIOpen/hipDNN | `nn` / `ops` | Native convolution, norm, activation, pooling, attention | MIOpen or native HIP kernels | P1 |
| Tensor primitives | cuTENSOR | hipTensor | `tensor_ops` | Layout-aware transpose, reduction, contraction, gather/scatter | hipTensor or native kernels | P1 |
| Quantization | TensorRT/CUDA kernels | MIGraphX/rocWMMA kernels | `quant` | INT8/INT4/Q4/Q8 packing and fused dequant/GEMM | HIP quantized kernels | P0 |
| Matrix acceleration | WMMA/Tensor Cores | rocWMMA/CDNA matrix cores | `matrix_caps` | Capability-gated wave/matrix paths and fallback tiling | rocWMMA where supported | P1 |
| Sparse math | cuSPARSE | rocSPARSE | `sparse` | CSR/COO/blocked sparse kernels | rocSPARSE adapter/native HIP | P2 |
| FFT | cuFFT | rocFFT | `fft` | Native radix kernels and tuned layouts | rocFFT adapter/native HIP | P2 |
| RNG | cuRAND | rocRAND | `rng` | Counter-based GPU RNG, deterministic seeds and streams | rocRAND/native HIP | P1 |
| Collectives | NCCL | RCCL | `collective` | Peer copies, topology-aware ring/tree, optional cross-device DX12 paths | RCCL adapter/native HIP | P2 |
| Communication | CUDA IPC, NVSHMEM | ROCSHMEM, IPC | `peer`, `network` | Shared handles, TCP/IOCP, optional RDMA abstraction | HIP IPC/peer access | P2 |
| Storage ingestion | cuFile/GDS | GPUDirect Storage | `io` | DirectStorage GPU path plus overlapped Win32 fallback | HIP staging and platform file I/O | P0 |
| Compression | nvCOMP | rocCOMP where available | `compression` | GPU decompression kernels / DirectStorage decompression capability | HIP decompression path | P1 |
| Model graphs | TensorRT, TorchInductor | MIGraphX, TorchInductor | `model`, `graph` | GGUF, Safetensors, ONNX, generic IR | Same graph contract | P0 |
| Inference runtime | TensorRT Runtime, PyTorch CUDA | MIGraphX, PyTorch ROCm | `inference` | Session, batching, KV cache, sampling, cancellation | Same session contract | P0 |
| Profiling | Nsight Systems/Compute, CUPTI | rocprof, Omnitrace | `trace`, `profile` | PIX markers/timestamps, ETW-compatible export, JSON/Chrome trace | HIP events and profiler adapters | P0 |
| Debugging | compute-sanitizer, Nsight | rocprof/rocgdb | `diagnostics` | DRED, debug layer, GPU validation, device-removal report | HIP diagnostics where available | P0 |
| Packaging | CUDA redistributables | ROCm/HIP SDK packages | `package` | CMake package, DLL/static, runtime DLL discovery, shader cache | Optional HIP package component | P0 |
| Language bindings | C, C++, Python, Fortran | C, C++, Python | C99, C++20, optional Rust/.NET | Stable C ABI and C++ wrappers | Same ABI where possible | P0 |

---

## 5. Lean canonical components

Each target component should own one concern. Existing modules should be merged according to the following rules.

| Canonical component | Absorbs or replaces | Public responsibility | Hot-path rule |
|---|---|---|---|
| `dxcore` | Parts of `dxait.hpp`, `dxait_types.h`, logging, config | Status, versioning, handles, backend registry, configuration, diagnostics | No heap allocation in dispatch path |
| `dxdevice` | `dxadapter`, `dxruntime` | Device creation, capabilities, topology, device loss | Capability discovery occurs once and is cached |
| `dxmemory` | `dxmem`, parts of `dxchunk`, `dxshard`, `dxcontext` | Allocator, staging, workspace, residency, ReBAR policy | Suballocation and pools avoid per-op OS allocations |
| `dxstream` | `dxqueue`, `dxsched`, parts of `dxgraph` | Queues, events, command reuse, dependencies | Preallocated allocators and descriptor caches |
| `dxtensor` | Tensor-like definitions currently scattered through operators | Shape, stride, layout, dtype, quant metadata, views | Views never copy; copies are explicit trace events |
| `dxgraph` | `dxgraph`, parts of `dxsched`, graph pieces in `dxcontext` | IR, validation, fusion, planning, replay | Compile once; replay many times |
| `dxops` | `dxblas`, `dxmath`, `dxattention`, `dxquant`, `dxfft`, `dxla`, `dxcache` | Canonical operators and kernel selection | Fused operators preferred over chained temporary tensors |
| `dxmodel` | `dxmodel`, parts of `dxai_ingest` | Model metadata, tensor loading, weight mapping | Zero-copy or streamed weights where safe |
| `dxio` | `dxio`, `dxchunk`, `dxstream`, `dxiocp` | DirectStorage, mmap, overlapped I/O, prefetch, eviction | I/O overlaps compute and exposes queue spans |
| `dxinfer` | `dxcontext`, `dxspeculative`, sampling portions of `dxmath` | Session, batching, KV cache, decode, cancellation | Separate prefill/decode plans |
| `dxcollective` | `dxcollective`, network portions of `dxnetwork` | Multi-GPU, peer access, all-reduce/all-gather | Topology-specific plan selected once |
| `dxtrace` | `dxtrace`, ad hoc timing and benchmark output | Unified CPU/GPU spans, counters, exporters | Disabled mode compiles to near-zero overhead |
| `dxmedia` | New | Audio/image preprocessing, vocoders, codecs, diffusion | Reuse tensor/graph/ops; no modality-specific runtime |
| `dxcompat` | Optional `dxmcp`, compatibility wrappers, DirectML path | External framework adapters and optional providers | Never required by the core runtime |

### 5.1 Modules to retire or demote

The project should retire the one-off `dxla` naming as a top-level architectural layer; its useful operators move into `dxops`. `dxtriton` should become an optional kernel-generation tool, not a runtime dependency. `dxmcp` should become an example/service adapter outside the inference core. `dxnetwork` should be split into a small `dxpeer` transport interface and an optional network service. `dxcontext`, `dxcache`, `dxkv`, and `dxstream` should be reduced to the responsibilities defined in `dxinfer`, `dxmemory`, and `dxio`.

---

## 6. Public API specification

### 6.1 C99 ABI requirements

The stable C header must not include C++ headers or expose `windows.h` directly. Windows-specific calling conventions and export macros belong in a small platform header. All structs begin with `struct_size` and `api_version` so the ABI can evolve.

```c
#ifdef __cplusplus
extern "C" {
#endif

typedef struct dx_context_t dx_context_t;
typedef struct dx_device_t dx_device_t;
typedef struct dx_stream_t dx_stream_t;
typedef struct dx_event_t dx_event_t;
typedef struct dx_memory_t dx_memory_t;
typedef struct dx_tensor_t dx_tensor_t;
typedef struct dx_graph_t dx_graph_t;
typedef struct dx_plan_t dx_plan_t;
typedef struct dx_model_t dx_model_t;
typedef struct dx_session_t dx_session_t;
typedef struct dx_trace_t dx_trace_t;

typedef struct dx_tensor_desc_t {
    uint32_t struct_size;
    uint32_t api_version;
    uint32_t rank;
    const uint64_t* shape;
    const int64_t* strides;
    uint32_t dtype;
    uint32_t layout;
    uint32_t flags;
    uint64_t byte_offset;
} dx_tensor_desc_t;

typedef struct dx_runtime_config_t {
    uint32_t struct_size;
    uint32_t api_version;
    uint32_t backend_preference;
    uint32_t validation;
    uint32_t tracing;
    uint32_t residency_policy;
    uint64_t vram_budget_bytes;
    uint64_t workspace_budget_bytes;
    const char* cache_directory;
} dx_runtime_config_t;

DXAIT_API dx_status_t dx_context_create(const dx_runtime_config_t*, dx_context_t**);
DXAIT_API dx_status_t dx_device_open(dx_context_t*, uint32_t index, dx_device_t**);
DXAIT_API dx_status_t dx_stream_create(dx_device_t*, uint32_t kind, dx_stream_t**);
DXAIT_API dx_status_t dx_tensor_create(dx_device_t*, const dx_tensor_desc_t*, dx_tensor_t**);
DXAIT_API dx_status_t dx_model_load(dx_device_t*, const char*, const dx_model_options_t*, dx_model_t**);
DXAIT_API dx_status_t dx_session_create(dx_model_t*, const dx_session_options_t*, dx_session_t**);
DXAIT_API dx_status_t dx_session_run(dx_session_t*, const dx_run_request_t*, dx_run_result_t*);
DXAIT_API dx_status_t dx_trace_export(dx_context_t*, const char*, uint32_t format);
DXAIT_API void dx_release(void* handle);

#ifdef __cplusplus
}
#endif
```

The API must provide asynchronous variants, explicit stream selection, structured error retrieval, allocator callbacks, external resource import/export, and cancellation. A global `last_error` string is not sufficient for production use.

### 6.2 C++20 API requirements

The C++ layer wraps every C handle in RAII types and provides `tensor_view`, `tensor`, `stream`, `graph_builder`, `execution_plan`, `model`, and `session` objects. The implementation should use `std::span`, `std::expected` or an equivalent project error type, `std::pmr` for controlled allocation, and compile-time descriptor validation where practical.

---

## 7. Tensor, graph, and execution contracts

### 7.1 Tensor contract

A tensor consists of a data allocation, shape, byte strides, element type, layout, quantization metadata, device, stream ownership, and lifetime token. A tensor view may change shape, strides, offset, or layout metadata without copying. Every copy, cast, materialization, and synchronization is explicit in the graph and trace.

Supported initial types:

| Category | Types |
|---|---|
| Floating point | FP32, FP16, BF16 where backend capability permits |
| Integer | INT8, UINT8, INT16, INT32, INT64 |
| Packed inference | INT4, Q4_0, Q4_K, Q5, Q6, Q8 families as model support requires |
| Auxiliary | BOOL/mask, token IDs, RNG state, opaque byte buffers |

### 7.2 Operator contract

Every operator declares input/output tensor constraints, supported dtypes, accumulation type, workspace requirement, determinism behavior, supported backends, and trace metadata. Kernel selection is a capability query followed by a cached plan decision.

Required P0 operators are copy/fill/cast, elementwise, reductions, softmax/log-softmax, RMSNorm, LayerNorm, GEMM/GEMV/batched GEMM, embedding, gather/scatter, transpose/reshape, RoPE, SiLU/SwiGLU, quantized dequantization, scaled dot-product attention, KV-cache append/read, top-k/top-p sampling, and random-number generation.

### 7.3 Graph and execution plan

The graph compiler performs validation, dependency analysis, layout propagation, constant folding, operator fusion, workspace planning, queue assignment, barrier planning, descriptor-table allocation, kernel specialization, and plan caching. The plan is immutable after compilation and can be replayed with new tensor bindings.

There must be two explicit execution modes:

| Mode | Purpose | Optimization |
|---|---|---|
| Eager | Debugging, dynamic shapes, one-off operators | Low setup cost, complete traceability |
| Planned | Repeated inference, prefill/decode, diffusion steps | Captured command reuse, fusion, preallocated workspace, minimal CPU submission |

---

## 8. DirectX 12, ReBAR, DirectStorage, and extension policy

### 8.1 DirectX 12 backend

The DX12 backend owns device creation, descriptor heaps, resource creation, resource-state tracking, barriers, command allocator reuse, pipeline state objects, shader reflection, queue synchronization, timestamp queries, device-removal diagnostics, and capability-gated dispatch.

The backend must support a portable baseline using standard D3D12 compute. Newer Agility SDK features are optional extensions selected by a capability table, not compile-time assumptions.

### 8.2 ReBAR and GPU-visible upload memory

ReBAR is treated as a system capability and performance mode, not a correctness requirement. At device initialization the runtime records:

| Capability | Required behavior |
|---|---|
| BAR/GPU-visible host access | Detect whether the adapter and allocation type support it. |
| Effective mapping size | Record the practical visible range and alignment. |
| Upload-heap read behavior | Benchmark sequential and random reads before enabling weight access mode. |
| Host coherence/cache policy | Encode flush/invalidate rules and avoid undefined CPU/GPU overlap. |
| Performance mode | Choose direct mapped reads, staged copies, or hybrid based on measured bandwidth. |
| Fallback | Use default-heap VRAM plus copy/upload staging when ReBAR is unavailable or slower. |

The memory manager must not assume that a GPU-visible upload heap is always faster than a default heap. It must measure and select a policy by workload class: weights, activations, KV cache, staging, and readback.

### 8.3 DirectStorage

DirectStorage is integrated as an asynchronous weight-ingestion provider. The interface must also support memory-mapped files and overlapped Win32 I/O. DirectStorage and Win32 paths produce the same `io_request` and `io_span` trace events.

The streaming pipeline is:

```text
file/model container
    -> async read request
    -> optional decompression/dequantization
    -> system staging or GPU-visible upload memory
    -> copy queue or direct GPU-access path
    -> resource-state transition
    -> execution-plan dependency
    -> compute dispatch
```

The implementation must support chunk sizes, prefetch depth, ring-buffer count, eviction priority, and queue selection as runtime policies. It must never use a single global command allocator for unrelated concurrent requests.

### 8.4 Capability-gated extensions

| Extension | Use | Baseline fallback | Promotion rule |
|---|---|---|---|
| Enhanced barriers | Lower barrier overhead and clearer state transitions | Legacy resource barriers | Enable after correctness and cross-vendor validation |
| Descriptor indexing/bindless | Reduce descriptor rebinding | Cached descriptor tables | Enable when supported and beneficial in trace |
| Indirect dispatch | Reduce CPU launch overhead | Direct dispatch | Promote for repeated dynamic work |
| Execute indirect | GPU-driven routing and compact work | CPU-generated dispatches | Promote only with measured CPU-submit savings |
| GPU upload heaps/ReBAR | Weight and staging access | Default/upload/readback copies | Select by measured bandwidth and latency |
| DirectStorage GPU path | Lower CPU I/O overhead | Overlapped Win32 read plus copy | Enable per volume and workload |
| Wave operations | Reductions, scans, softmax, attention | Shared-memory reductions | Select by wave capability and kernel benchmark |
| Shader Model 6.7 features | Specialized kernels and scheduling | SM baseline kernels | Select by compiler/runtime feature support |
| Work graphs | Optional irregular GPU-driven pipelines | Explicit dispatch graph | Experimental; require strong evidence and tooling |
| Matrix acceleration | GEMM and fused ML kernels | Vectorized/tiled HLSL | Select by capability and numerical validation |

---

## 9. Precision and kernel specification

Precision is an explicit contract, not an incidental shader property. Every operator records input type, accumulation type, output type, quantization scale format, and error tolerance.

| Kernel family | Baseline | Tuned variants | Numerical requirement |
|---|---|---|---|
| GEMM/GEMV | Tiled HLSL with FP32 accumulation where selected | SM 6.7 wave/matrix, packed F16/INT4, persistent decode GEMV | Relative/absolute tolerances by dtype |
| RMSNorm/LayerNorm | Two-pass reduction | Fused vector load, wave reduction, fused affine | Stable epsilon and deterministic option |
| Softmax | Max/sum reduction | Fused temperature/mask/causal path | No NaN/Inf on valid finite inputs |
| Attention | SDPA baseline | Flash, paged, chunked, GQA/MQA, sliding-window, fused RoPE | Reference-tested per mechanism |
| MLP | Separate GEMM/activation | Fused SwiGLU, quantized dequant + GEMM | Accumulation mode declared |
| Quantization | CPU/GPU packing and dequant | Fused dequant/GEMM, groupwise scales | Bit-exact packing format |
| Diffusion | Matmul, convolution, norm, attention | Fused residual blocks, tiled conv, latent reuse | Image quality and numeric checks |
| Audio | FFT, mel/filterbank, matmul, vocoder ops | Fused feature extraction and vocoder blocks | Audio spectral/error checks |
| Sampling | Top-k/top-p/temperature | Fused logits transform and selection | Seeded reproducibility mode |

Each kernel has a CPU reference implementation or an independently verifiable reference path. Benchmark results must include shape, dtype, backend, GPU, driver, shader/compiler version, and trace ID.

---

## 10. Multimodal runtime components

The runtime uses one tensor/graph/executor foundation for every modality.

| Pipeline | Required components | Reused foundation |
|---|---|---|
| Text-to-text | Tokenizer ABI, embedding, transformer blocks, RoPE, attention, KV cache, sampling, streaming tokens, prompt/decode plans | Tensor, graph, ops, inference, I/O, trace |
| Text-to-speech | Text normalization, tokenizer/phoneme frontend, acoustic model, duration/pitch/energy model, vocoder, WAV/PCM output | Tensor, graph, ops, streaming, trace |
| Text-to-image | Text encoder, latent initialization, diffusion scheduler, U-Net/transformer, cross-attention, VAE decoder, image output | Tensor, graph, attention, quantization, I/O, trace |
| Image-to-image | Image decode/normalize, VAE encode, conditioning, latent noise schedule, denoising graph, VAE decode | Tensor, graph, image I/O, diffusion ops, trace |
| Shared media | PNG/JPEG/WebP, WAV/PCM, color conversion, resampling, batching, memory pools | Memory, tensor, stream, trace |

Model-specific logic belongs in adapters that lower into the common graph and operator contracts. It must not create a second allocator, scheduler, or tracing system.

---

## 11. Full tracing and performance specification

Tracing is a first-class subsystem with a disabled mode that has negligible hot-path cost. The system must support per-request correlation IDs and nested spans across CPU threads, GPU queues, file I/O, model stages, and operators.

### 11.1 Required event classes

| Event class | Required fields |
|---|---|
| Runtime | process, thread, request ID, backend, device, driver, SDK/compiler versions |
| Graph | graph ID, plan ID, shape signature, specialization key, fusion groups |
| Operator | op name, input/output shapes, dtype, kernel variant, workspace, estimated FLOPs |
| Kernel | shader/HSACO identifier, PSO hash, dispatch dimensions, wave mode, occupancy hints |
| Queue | queue kind, command-list ID, allocator ID, submit time, fence value |
| GPU span | timestamp begin/end, queue, calibrated CPU time, duration |
| Memory allocation | heap, resource, size, alignment, residency, BAR mode, allocation site |
| Memory transfer | source/destination, bytes, queue, bandwidth, staging mode, overlap |
| I/O | path/container, offset, bytes, provider, compression, read latency, queue delay |
| Synchronization | wait reason, producer/consumer, fence/event, stall duration |
| Cache | shader/PSO/plan/model chunk hit or miss, key, compile/load time |
| Error | status, HRESULT/backend code, device removal data, breadcrumb/diagnostic ID |
| Quality | seed, tolerance mode, checksum, reference comparison result |

### 11.2 Trace outputs

The minimum output formats are Chrome Trace JSON, compact binary trace, human-readable summary, and CSV/JSON benchmark tables. Optional adapters may emit PIX marker data or integrate with ETW. The trace viewer must make it possible to answer:

1. Was the request compute-bound, memory-bound, I/O-bound, or CPU-submit-bound?
2. Which queue or fence introduced the stall?
3. Which kernels were selected and why?
4. How many bytes moved between disk, system RAM, upload memory, and VRAM?
5. Did ReBAR or DirectStorage improve this workload?
6. Did fusion reduce dispatch count and temporary allocations?
7. Which precision path affected accuracy or throughput?
8. Was the result limited by shader compilation, descriptor setup, residency, or synchronization?

### 11.3 Performance counters and modes

| Mode | Behavior |
|---|---|
| Off | No event construction on hot paths; compile-time or branch-elided where possible. |
| Errors | Device, allocation, model, and numerical failures only. |
| Lightweight | CPU spans, queue submits, operator metadata, aggregate memory counters. |
| Full | GPU timestamps, synchronization, allocation, I/O, kernel selection, cache events. |
| Diagnostic | Debug layer, validation, DRED, shader dumps, resource naming, reference checks. |
| Benchmark | Repeated warmup/measurement, percentile latency, throughput, bandwidth, variance, trace ID. |

Performance acceptance requires p50, p95, and p99 latency where applicable; tokens/s for decode; images/s or seconds/image for diffusion; real-time factor for speech; GPU utilization; memory high-water mark; dispatch count; bytes transferred; and CPU submission time.

---

## 12. Build, packaging, and deployment specification

The current hard-coded local SDK paths must be removed. CMake must support discovered or explicitly supplied SDK roots:

```text
-DXAIT_DX12=ON|OFF
-DXAIT_HIP=ON|OFF
-DXAIT_CPU=ON|OFF
-DXAIT_DIRECTSTORAGE=ON|OFF
-DXAIT_DXC=ON|OFF
-DXAIT_PIX=ON|OFF
-DXAIT_BUILD_TESTS=ON|OFF
-DXAIT_BUILD_TOOLS=ON|OFF
-DXAIT_BUILD_ADAPTERS=ON|OFF
-DXAIT_BUILD_SHARED=ON|OFF
```

The package must provide:

| Artifact | Requirement |
|---|---|
| `dxait_core` | Stable core and ABI. |
| `dxait_dx12` | DirectX 12 backend and HLSL kernels. |
| `dxait_hip` | Optional HIP/ROCm backend. |
| `dxait_cpu` | Reference and SIMD backend. |
| `dxait_ops` | Operator registry and kernels. |
| `dxait_model` | Model loaders and metadata. |
| `dxait_infer` | Session and modality runtime. |
| `dxait_trace` | Trace recording/export. |
| `dxait_compat` | Optional external adapters. |
| `dxait.dll` plus import library | C99 ABI distribution. |
| CMake config | `find_package(DXAiSt CONFIG REQUIRED)`. |
| Runtime manifest | Backend/shader/compiler/SDK version metadata. |

A clean consumer should not need to know where a local developer stored the Agility SDK, DXC, DirectStorage, or HIP SDK. Runtime probing and deployment packaging must make dependencies explicit and report missing optional capabilities clearly.

---

## 13. Validation and benchmark matrix

| Test group | Coverage |
|---|---|
| ABI | C99 compile tests, struct versioning, handle lifetime, DLL boundary, error propagation |
| Core correctness | Device, stream, event, allocator, descriptor, barrier, resource state |
| Tensor | Shapes, strides, views, dtype conversion, aliasing, layout transforms |
| Operators | CPU reference comparison across dtypes and backends |
| Graph | Validation, fusion, specialization, capture/replay, invalid dependency detection |
| Memory | Budget pressure, residency, eviction, ReBAR on/off, upload/readback, aliasing |
| I/O | DirectStorage, mmap, Win32 fallback, decompression, cancellation, partial reads |
| Models | GGUF, Safetensors, ONNX, malformed input, large model streaming |
| Modalities | Text, speech, text-to-image, image-to-image end-to-end smoke tests |
| Reliability | Device removal, TDR-safe chunking, shader cache invalidation, process shutdown |
| Performance | Kernel microbenchmarks, graph replay, memory bandwidth, I/O overlap, end-to-end latency |
| Cross-vendor | NVIDIA, AMD, Intel DX12 adapters; HIP/ROCm where installed; CPU fallback |
| Tracing | Event completeness, timestamp calibration, export validity, disabled-mode overhead |

A benchmark result is invalid unless it records the GPU adapter, vendor/device ID, driver, OS, SDK/compiler versions, backend, dtype, shape, model revision, warmup count, measurement count, and trace file or trace ID.

---

## 14. Implementation sequence

| Phase | Deliverable | Exit criterion |
|---|---|---|
| P0 | New core ABI, runtime, device, stream, memory, tensor | C99 sample creates device, tensor, upload, dispatch, readback, and trace |
| P1 | Graph, plan cache, operator registry, core math kernels | Repeated planned execution is faster than eager execution on target shapes |
| P2 | ReBAR-aware allocator and DirectStorage/Win32 I/O | Weight streaming overlaps with compute and produces comparable traces |
| P3 | Quantized GEMM, attention, KV cache, sampling | Text decode reference and performance tests pass |
| P4 | SM 6.7 tuning and capability-gated extensions | Tuned kernels beat baseline on measured target shapes without reducing portability |
| P5 | HIP/ROCm and CPU backends | Same graph/operator tests run through all available backends |
| P6 | Audio/image/diffusion pipelines | Four requested modalities run through common tensor/graph/runtime APIs |
| P7 | ONNX Runtime/llama.cpp/application adapters | External sample integrates without private DxAiSt internals |
| P8 | Packaging, diagnostics, benchmark suite, documentation | Clean machine deployment succeeds with optional dependencies reported |

---

## 15. Definition of done

The revamp is complete when a new native application can select a backend, load a supported model, create a session, run asynchronous inference, stream results, export a full performance trace, and receive structured errors using only the installed DxAiSt package and documented C99/C++20 APIs.

The implementation must demonstrate that the new structure is leaner than the current repository: fewer duplicate runtime concepts, no feature-specific schedulers, no hard-coded developer paths, no mandatory DirectML dependency, no global error state, and no hidden synchronization in performance mode. Every retained optimization must be justified by a benchmark and visible in the trace.

---

## References

[1]: https://docs.nvidia.com/cuda/ "CUDA Toolkit Documentation"

[2]: https://developer.nvidia.com/cuda/toolkit "NVIDIA CUDA Toolkit"

[3]: https://developer.nvidia.com/cupti "NVIDIA CUDA Profiling Tools Interface"

[4]: https://www.amd.com/en/developer/resources/rocm-hub/hip-sdk.html "AMD HIP SDK for Windows"

[5]: https://rocm.docs.amd.com/projects/install-on-windows/en/latest/ "ROCm HIP SDK on Windows"

[6]: https://rocm.docs.amd.com/projects/rocBLAS/en/docs-5.1.3/Windows_Install_Guide.html "rocBLAS Windows Installation Guide"

[7]: https://gpuopen.com/learn/using-d3d12-heap-type-gpu-upload/ "GPUOpen: Using D3D12 Heap Type GPU Upload"

[8]: https://devblogs.microsoft.com/directx/landing-page/ "Microsoft DirectX Developer Blog"

[9]: https://microsoft.github.io/DirectX-Specs/ "Microsoft DirectX Specifications"
