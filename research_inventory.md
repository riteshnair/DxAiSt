# DxAiSt inventory notes

## Repository baseline

- Repository: `riteshnair/DxAiSt` (GitHub resolves to `Maxritz/DxAiSt`)
- Branch: `main`
- Audited commit: `446e123`
- Project description: native Windows-first Direct3D 12 GPU compute fabric for deep-learning inference.
- Language mix from GitHub page: approximately 95% C++ and 3% HLSL, with the remainder other files.
- Build system: CMake, C++20, MSVC-oriented, static `dxait` library.
- Current CMake hard-codes local SDK paths for Agility SDK, DXC, and DirectStorage, which prevents ready-made drop-in reuse without toolchain configuration cleanup.

## Existing public modules

`dxait.hpp`, `dxait_types.h`, `dxadapter.h`, `dxbarrier.hpp`, `dxjit.hpp`, `dxblas.hpp`, `dxmath.hpp`, `dxattention.hpp`, `dxquant.hpp`, `dxkv.hpp`, `dxcontext.hpp`, `dxdb.hpp`, `dxmcp.hpp`, `dxmodel.hpp`, `dxchunk.hpp`, `dxshard.hpp`, `dxcache.hpp`, `dxcollective.hpp`, `dxnetwork.hpp`, `dxrand.hpp`, `dxsched.hpp`, `dxgraph.hpp`, `dxspeculative.hpp`, `dxio.hpp`, `dxtriton.hpp`, `dxtrace.hpp`, `dxfft.hpp`, `dxla.hpp`, `dxstream.hpp`, `dxiocp.hpp`, `dx_c_api.h`.

Source-only core modules include adapter, runtime, queue, and memory implementations. There are 30 top-level source directories including shader sources.

## Existing functionality

The README claims support for GGUF and Safetensors loading, DirectStorage file streaming, streaming-MoE, IOCP network streaming, VRAM/RAM sharding, KV-cache offload and quantization, vector retrieval and MCP, tensor networking, runtime HLSL compilation, transformer decode, attention variants, speculative decoding, FFT, cache transforms, graph scheduling, and low-precision operations.

## Existing shader inventory

- Attention: FlashAttention and paged attention.
- BLAS: RDNA2 wave64 GEMV and RDNA4 WMMA GEMV.
- Convolution: Conv2D.
- FFT: radix-2 FFT.
- KV: RoPE.
- Math: elementwise and SiLU/SwiGLU.
- Model: MoE routing and speculative verification.
- Quantization: atomic INT4 GEMM.
- Sparse: CSR SpMV.

## Tests and tools

There are 32 test harnesses covering device initialization, memory, JIT/BLAS, graph/scheduler, math, attention, quantization, model formats, streaming/sharding, model loading, network, Triton-style JIT, FFT, cache, speculative decoding, tracing, low-precision operations, and DirectStorage/IOCP streaming. Tools are `dxinspect`, `dxbench`, and `dxai_ingest`.

## Current public C ABI assessment

The existing C API exposes device, queue, buffer, upload/download, a small `dx_la_*` operator set, wait, device description, and a global last-error string. It uses raw `HRESULT`, opaque handles, and includes `windows.h`. It does not yet expose a versioned runtime/session model, tensor descriptors, streams/events, graph/plan execution, backend selection, model loading, async operations, custom allocators, multimodal pipelines, structured diagnostics, or a stable cross-platform C99 header.

## Architectural conclusion

The project has many useful kernels and subsystems, but the integration boundary is still low-level and DXAiT-specific. The redesign should add a backend-neutral core with: runtime/device, allocator/residency, tensor ABI, operator registry, graph IR, compiler/plan cache, executor, model format adapters, modality pipelines, and a stable C99 ABI. DX12 should be the native Windows backend; HIP/ROCm should be an optional backend; CPU reference execution should support correctness and unsupported operations. DirectML should remain optional rather than foundational.

## Required new portability-kit areas

1. CUDA/ROCm equivalence layer: runtime, streams/events, memory, graphs, BLAS, tensor primitives, convolution, collectives, RNG, sparse, quantization, profiling, debugging, serialization, and deployment.
2. Hardware/system layer: DX12 feature caps, ReBAR detection and policy, VRAM budget/residency, DirectStorage plus overlapped Win32 fallback, PCIe/topology discovery, device removal recovery.
3. Native multimodal layer: text-to-text, text-to-speech, text-to-image, and image-to-image model graphs, preprocessing/postprocessing, tokenization, audio features/vocoders, image codecs/latent transforms, diffusion schedulers, samplers, batching, and streaming.
4. Integration layer: C99 ABI, C++20 RAII API, CMake package, DLL/static builds, ONNX Runtime adapter, model loader APIs, examples, profiling tools, and test/reference kernels.
