# DxAiSt declaration-to-runtime validation protocol

This protocol is mandatory for every new or changed DxAiSt component, public API, DLL, shader, kernel, adapter, and package. **Compilation is evidence of syntax only. It is never evidence of implementation or completion.**

## Completion rule

A surface may be marked `finished` only when every applicable gate below is `PASS`. If a gate cannot run because the environment lacks Windows, a GPU, a driver, or an SDK, the surface is `blocked` or `in progress`, not `finished`.

## Gate 0 - Inventory before design

Create an inventory before writing code. Search for all related names, aliases, headers, source files, targets, generated artifacts, tests, documentation, and package entries. Include both the requested name and likely variants, for example:

```text
dx_c_api.h
dx-c-api.h
dx_capi
dx_create_device
DX_CAPI
```

Record the inventory in the change log. A missing or empty header is a gap, not an implementation.

## Gate 1 - Public declaration

| Check | Required evidence |
|---|---|
| Header exists | Exact path and package entry |
| API shape | Types, enums, structs, function signatures, calling convention |
| ABI version | Version constant and struct-size rules where applicable |
| Ownership | Create/destroy, borrowed/owned handles, allocator responsibility |
| Threading | Thread-safe, externally synchronized, stream affinity, reentrancy |
| Errors | Return codes, null/invalid/overflow behavior, device-loss behavior |
| Capability rules | Required feature bits and fallback behavior |
| Symbol policy | Export name, visibility, deprecation/version policy |

Every declaration must compile as C99 and as C++20 from a clean consumer translation unit.

## Gate 2 - Implementation completeness

For every declared function, prove one of the following:

| Result | Meaning |
|---|---|
| Implemented | A non-placeholder definition exists and is reachable |
| Deliberately unsupported | Returns a documented error and is excluded from completion claims |
| Optional | Feature-gated definition and false-path behavior are tested |
| Missing | No definition; component cannot be finished |

Search declarations against definitions. Do not infer implementation from similarly named C++ classes, headers, tests, or comments.

## Gate 3 - Build and target wiring

Prove that the implementation is included in the intended CMake target, compiles with the target’s actual language standard, links required dependencies, and is installed or packaged when the contract is public. Verify that no developer-local absolute paths are required.

For DLLs, record the exact target name, output filename, import library, runtime dependencies, and configuration used.

## Gate 4 - Export and ABI audit

Inspect the built artifact, not only the source:

| Artifact | Required inspection |
|---|---|
| Windows DLL | Export table contains every public symbol with the exact undecorated name and calling convention |
| Import library | Contains the same public symbols |
| Static library | Defined symbols are present and duplicate/conflicting exports are absent |
| Header | Consumer can link a C99 caller without C++ name mangling |
| C++ wrapper | RAII lifetime does not change C ABI ownership |

Maintain a machine-readable symbol report for each release candidate.

## Gate 5 - Runtime smoke test

Run a real consumer against the built artifact. The test must load the actual DLL, resolve symbols, create the smallest valid object, execute one valid operation, destroy objects in reverse ownership order, and verify the documented error path. A source-level mock does not satisfy this gate.

If the platform cannot run, record the exact blocker and leave the status `blocked`.

## Gate 6 - Numerical and semantic validation

For numerical operations, compare the real backend result with a trusted CPU/reference result using fixed, boundary, random, and adversarial inputs. Record dtype, shape, strides, tolerances, NaN/Inf policy, deterministic mode, and seed. Test both contiguous and rejected non-contiguous layouts where the contract distinguishes them.

A plausible output without a reference comparison is not validation.

## Gate 7 - Shader/kernel lifecycle

Every shader or kernel must pass all applicable checks:

1. Source file exists and is listed in the build.
2. Compiler/toolchain version and flags are recorded.
3. Real DXIL/CSO artifact is generated.
4. Reflection confirms entry point, thread-group size, resources, root-signature bindings, and expected types.
5. Artifact version/hash is recorded and stale artifacts are rejected.
6. Pipeline state/root signature creation succeeds.
7. Dispatch uses the correct resource states, descriptors, dimensions, barriers, and synchronization.
8. At least one real tensor shape executes on a Windows GPU or WARP.
9. CPU/reference comparison passes.
10. Capability false paths select a tested fallback.

A shader source, embedded byte array, compile target, or dispatch stub alone is not a kernel implementation.

## Gate 8 - Resource and failure validation

Test null handles, invalid descriptors, zero sizes, overflow sizes, alignment failures, double destroy, use-after-destroy rejection, allocation failure, missing optional DLLs, missing shader artifacts, device removal, timeout, and cancellation where applicable. Confirm cleanup after every failure.

## Gate 9 - Trace and performance evidence

Trace evidence must identify request, component, API symbol, backend, adapter, queue, operation/kernel, dimensions, dtype, memory movement, synchronization, result, and duration. Performance evidence must include cold and warm runs, p50/p95/p99, throughput, memory high-water mark, transfer bytes, dispatch count, and trace ID. Measure trace overhead with tracing off, light, full, and diagnostic modes.

Host logging alone does not satisfy GPU trace validation.

## Gate 10 - Integration and package validation

Run a real integration path, not only a unit test. Verify the package on a clean machine or isolated environment with only declared runtime dependencies. Check DLL search paths, redistributables, shader artifacts, import libraries, headers, license files, install/export metadata, and version reports.

## Mandatory status fields

Every status row must include:

```text
surface_name
surface_kind
public_declaration
implementation_definition
build_target
export_or_link_symbol
runtime_test
reference_test
failure_test
trace_test
package_entry
environment_blocker
status
```

Allowed status values are `finished`, `in progress`, `blocked`, `pending`, and `retired`.

## Prohibited claims

Do not say `implemented`, `working`, `complete`, `production-ready`, `GPU-ready`, or `drop-in` when only a header, source stub, compile, static test, host fallback, symbol string, or mocked result exists. Use precise wording such as `contract drafted`, `compiled`, `portable path validated`, `Windows runtime pending`, or `GPU dispatch pending`.

## Required change record

Every mutation must record the prior checkpoint, exact files and symbols, reason, dependencies, CPU impact, validation commands and results, environment limits, package impact, and exact rollback action. Update the status ledger only after the relevant gates are rerun.
