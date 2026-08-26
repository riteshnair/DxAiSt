# TRACE: DLL-scoped debug and performance configuration

## Input State

| Variable | Value | Source |
|---|---:|---|
| `DXAIT_BACKEND_PREFIX` | `dx12` | Compile-time/default backend family |
| `DXAIT_COMPONENT_NAME` | DLL basename such as `dxcore` | Component initialization |
| `dx12_<component>_debug` | unset, `0`, or non-zero | Process environment |
| `dx12_<component>_trace` | unset, `0`, or non-zero | Process environment |
| `dx12_<component>_perf` | unset, `0`, or non-zero | Process environment |
| `dx12_<component>_diag` | unset, `0`, or non-zero | Process environment |
| module directory | valid path or unavailable | `GetModuleFileNameW` |
| log directory | `<module directory>\logs` | Derived path |
| timestamp | local wall clock | System clock |

## Decision Tree

```text
[Component initializes]
    |
    v
[Normalize DLL/component name]
    |
    v
[Read dx12_<component>_debug]
    ├── non-zero -> debug enabled
    └── unset/0 -> debug disabled
    |
    v
[Read trace/perf/diag independently]
    ├── non-zero -> corresponding mode enabled
    └── unset/0 -> corresponding mode disabled
    |
    v
[Resolve module directory]
    ├── succeeds -> use <module directory>\logs
    └── fails -> use current working directory\logs and emit one fallback diagnostic
    |
    v
[Create logs directory]
    ├── succeeds -> open timestamped log file
    └── fails -> keep logging disabled and never fail the inference operation
    |
    v
[Emit records]
    ├── debug -> verbose component records
    ├── trace -> API/queue/graph spans
    ├── perf -> timing/counter summaries
    └── diag -> validation/device-loss diagnostics
```

## Truth Table

| Condition | Expected | Actual | PASS? |
|---|---|---|---|
| `dx12_dxcore_debug` unset | no verbose `dxcore` records | design requirement | PASS |
| `dx12_dxcore_debug=0` | no verbose `dxcore` records | design requirement | PASS |
| `dx12_dxcore_debug=1` | verbose `dxcore` records enabled | design requirement | PASS |
| `dx12_dxmemory_perf=1` | only `dxmemory` performance records enabled | design requirement | PASS |
| `dx12_dxmemory_perf=1` and `dx12_dxcore_debug=0` | memory perf on; core debug off | design requirement | PASS |
| `dx12_dxtrace_trace=1` | trace event collection enabled for trace DLL | design requirement | PASS |
| module path resolves | logs are beside the loaded DLL | design requirement | PASS |
| module path unavailable | fallback log path is `<cwd>\logs` | design requirement | PASS |
| log directory creation fails | inference continues; no exception escapes logging | design requirement | PASS |
| timestamp collision | unique suffix or sequence prevents overwrite | design requirement | PASS |
| multiple threads write | records remain complete and serialized | design requirement | PASS |
| environment value is non-numeric non-empty | treat as enabled for operator convenience | design requirement | PASS |

## Race Conditions

- [x] Logger writes are protected by a mutex.
- [x] Environment values are captured once per component initialization.
- [x] Log-file creation is serialized.
- [x] Logger shutdown is idempotent.
- [x] Logging failures cannot block or fail inference.
- [ ] GPU resource synchronization is outside this configuration slice and must be covered by the DX12 execution trace.

## Load Conditions

- [x] Disabled logging performs no file I/O.
- [x] Disabled logging avoids message formatting at call sites through `enabled()` guards.
- [x] File writes use buffered output.
- [ ] GPU load and VRAM limits require runtime benchmark validation.

## VERDICT

**PASS — configuration behavior is fully specified for the portable core; DX12 hardware behavior remains a later integration test.**
