# DXAiSt Completion Audit Rules

> The mandatory detailed procedure is in `docs/VALIDATION_PROTOCOL.md`. This file defines the release verdict and must be read with that protocol.

## 1. Completion states

| State | Meaning | Allowed claim |
|---|---|---|
| `pending` | Not started or not yet scoped | No implementation claim |
| `in progress` | Code or contract is actively being developed | Partial implementation only |
| `blocked` | Depends on unavailable SDK, hardware, or external decision | Blocker must be named |
| `finished` | All audit gates pass | Component may be called complete |
| `retired` | Deliberately removed or merged | No new callers permitted |

## 2. Mandatory audit gates

A component is complete only when every gate below is marked `PASS`.

### Gate A — Contract

- Public header exists at the expected path and is included in the package.
- The requested name and variants were searched before implementation, including `dx_c_api.h`, `dx-c-api.h`, `dx_capi`, and related symbols.
- The header is not empty, skeletal, or only a legacy declaration: every public function is classified as implemented, deliberately unsupported, optional, or missing.
- Every declaration has a documented C99 or C++20 contract and compiles from a clean consumer translation unit in both languages where applicable.
- ABI structs are versioned with size fields where applicable.
- Ownership, thread-safety, stream affinity, lifetime, and error behavior are documented.
- DLL/component name, exact export name, calling convention, and environment-variable names are documented.

### Gate B — Implementation

- A declaration-to-definition report proves that every public symbol has a matching definition or an explicit unsupported status.
- The component has a production DLL or library target.
- The implementation is reachable from a non-test production path.
- No header, source file, shader, kernel, or API is counted as implemented solely because a similarly named legacy class or test exists.
- No hard-coded developer paths exist.
- No hidden global synchronization or hidden device wait exists in performance mode.
- All external resources and optional SDKs are capability checked.

### Gate C — Truth table

Before any branch-heavy implementation, record:

| Condition | True path | False path | Inputs covered | Result |
|---|---|---|---|---|
| Capability available | optimized implementation | portable fallback | supported and unsupported adapters | PASS/FAIL |
| Debug mode enabled | verbose records | no debug file I/O | unset, `0`, `1`, non-numeric | PASS/FAIL |
| Trace/perf mode enabled | events/timestamps | zero or near-zero overhead | off/light/full | PASS/FAIL |
| Valid input | operation executes | structured error | boundary, overflow, null, malformed | PASS/FAIL |
| Resource available | reuse/dispatch | allocation or fallback | budget pressure and normal budget | PASS/FAIL |

### Gate D — Flow trace

Each component must document one happy path and one failure path. Every decision point must include expected versus actual values. Any divergence is marked `FAIL` until fixed or deliberately documented as a fallback.

```text
[API entry]
    -> [validate handle and descriptor]
        -> valid: [select capability]
            -> supported: [select optimized path]
            -> unsupported: [select portable fallback]
        -> invalid: [return structured error]
    -> [record trace span]
    -> [submit or execute]
    -> [record completion and metrics]
```

### Gate E — Tests

- C99 compile and ABI boundary test for every public C header, including umbrella/compatibility headers such as `dx_c_api.h`.
- C++20 API and RAII lifetime test.
- Declaration-to-definition completeness test with a zero-unresolved-public-symbol result, except symbols explicitly listed as unsupported.
- Built-artifact export/import-library audit with exact names and calling conventions.
- Valid operation test.
- Invalid/null/overflow input tests.
- Repeated execution and resource reuse test.
- Multi-thread and multi-stream test where applicable.
- Device-loss or backend-error test where applicable.
- CPU/reference comparison for numerical components.
- Cross-vendor test matrix or explicit hardware blocker.

### Gate F — Performance evidence

The benchmark must record GPU, vendor/device ID, driver, OS, SDK/compiler versions, backend, dtype, shapes, warmup count, measurement count, p50/p95/p99 latency, throughput, memory high-water mark, bytes transferred, dispatch count, CPU submission time, and trace ID.

### Gate G — Trace evidence

With `dx12_<dll_component>_trace=1` or `dx12_<dll_component>_perf=1`, the component must emit timestamped records in the DLL-local `logs` folder. The trace must identify the request, operation, backend, queue/stream, kernel or provider, memory movement, and completion duration.

### Gate H — Stub and dead-code audit

For all new and changed files, search for:

```text
TODO FIXME XXX stub placeholder mock fake dummy noop NOT IMPLEMENTED unimplemented
```

For every match, record file, line, severity, current behavior, expected behavior, and one action: `Remove`, `Wire and test`, or `Guard and warn`.

Every new header/module must be included by production code and invoke at least one defined symbol. Every public header must have a clean consumer compile test. Every declared public symbol must be matched to a definition, target, export/import entry, runtime call, or an explicit unsupported record. Every shader/kernel must be listed in the build, compile to a real artifact, pass reflection and binding checks, bind to a pipeline, and be exercised with real tensor shapes before it can be called complete.

## 3. Audit report format

Each release or milestone must produce:

| Component | Gate A | Gate B | Gate C | Gate D | Gate E | Gate F | Gate G | Gate H | Verdict |
|---|---|---|---|---|---|---|---|---|---|
| `dx12_<component>.dll` | PASS/FAIL | PASS/FAIL | PASS/FAIL | PASS/FAIL | PASS/FAIL | PASS/FAIL | PASS/FAIL | PASS/FAIL | FINISHED/PENDING/BLOCKED |

## 4. Mandatory pre-completion checklist

Before changing any status row to `finished`, attach or record all of the following:

| Check | Required result |
|---|---|
| Name inventory | Requested API/component names and variants searched; no unreviewed sibling surface remains |
| Public headers | C99/C++20 clean-consumer compile passes for every public header |
| Declaration map | Every declaration maps to exactly one definition or explicit unsupported record |
| Target map | Every definition is in the intended CMake target |
| Export map | DLL export and import-library symbols match declarations exactly |
| Runtime map | Real consumer loads/links the artifact and calls one success path |
| Error map | Null, invalid, missing dependency, unsupported capability, and cleanup paths tested |
| Numerical map | Reference comparison passes for numerical operations |
| Shader/kernel map | Source -> compiler -> DXIL/CSO -> reflection -> PSO -> dispatch -> real shape |
| Trace map | Required host/GPU trace records and trace-overhead measurements exist |
| Package map | Clean deployment contains all required files and no accidental build artifacts |
| Rollback map | Exact file/symbol rollback and previous checkpoint recorded |

Any `FAIL`, `UNKNOWN`, or unrun platform-specific gate means the row remains `in progress` or `blocked`.

## 5. Prohibited completion claims

A component must not be called finished because it merely compiles, has a header, passes a single smoke test, produces a plausible output without a reference check, logs only CPU time, uses a fake/dummy workload, has a source or embedded shader without a real dispatch, or has a declaration without a verified definition and export.
