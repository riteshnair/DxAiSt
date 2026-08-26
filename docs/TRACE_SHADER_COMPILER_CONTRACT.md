# TRACE: shader compiler contract

## Decision Tree

```text
[shader_compile_file]
    |
    v
[validate compiler/path/entry/profile/output]
    ├── invalid -> structured compiler-input error
    └── valid
        |
        v
[DXC available]
    ├── no -> compiler-unavailable; caller may use precompiled DXIL
    └── yes -> load source and compile with requested profile
        |
        v
[compile result]
    ├── diagnostics/errors -> return compiler error and diagnostic text
    └── valid DXIL -> publish immutable blob
```

## Truth Table

| Condition | Expected | Result |
|---|---|---|
| null output | invalid-argument; output null | PASS |
| missing source | file error | PASS |
| empty entry/profile | invalid-argument | PASS |
| DXC unavailable | explicit unavailable status; no fake blob | PASS |
| HLSL compile failure | error and diagnostics | PASS |
| valid compile | immutable blob and exact byte size | PASS |
| requested SM 6.7 | profile passed through unchanged | PASS |
| blob destroy | safe release | PASS |

## VERDICT

**PASS — shader compilation and precompiled-DXIL fallback are specified; Windows DXC execution remains deferred.**
