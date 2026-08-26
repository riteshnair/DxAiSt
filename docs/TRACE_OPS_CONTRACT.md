# TRACE: operator kernel contract

## Decision Tree

```text
[operator entry]
    |
    v
[validate pointers, dimensions, and overflow]
    ├── invalid -> return invalid-argument; no output mutation
    └── valid
        |
        v
[select implementation]
    ├── native backend available -> backend kernel
    ├── CPU reference enabled -> reference kernel
    └── no implementation -> unsupported-operation
        |
        v
[execute and trace]
    ├── success -> output complete and trace duration
    └── failure -> structured error; no false success
```

## Truth Table

| Condition | Expected | Result |
|---|---|---|
| null input/output | invalid-argument; no write | PASS |
| zero element count | invalid-size | PASS |
| GEMM dimension overflow | invalid-size | PASS |
| copy with overlapping ranges | documented memmove semantics | PASS |
| fill | every requested element receives value | PASS |
| GEMM | row-major result with alpha/beta | PASS |
| backend unavailable | CPU reference or unsupported status | PASS |
| trace/perf enabled | operation metadata and duration emitted | PASS |
| trace/perf disabled | no provider I/O from optional tracing | PASS |

## VERDICT

**PASS — reference operator behavior is specified; native DX12/HIP kernels remain pending.**
