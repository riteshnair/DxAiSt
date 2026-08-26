# TRACE: portable resource contracts

## Input State

| Variable | Values | Source |
|---|---|---|
| backend | auto, dx12, hip, cpu | runtime configuration |
| requested bytes | zero or positive | allocation/tensor descriptor |
| rank | zero through supported maximum | tensor descriptor |
| shape entries | zero or positive | caller |
| dtype | supported or unsupported enum | tensor descriptor |
| stream | null/default or valid handle | API call |
| allocation policy | device, upload, readback, host | caller/backend default |
| backend capability | supported or unsupported | device probe |

## Decision Tree

```text
[API entry]
    |
    v
[Validate output pointer and struct size]
    ├── invalid -> return invalid-argument status
    └── valid
        |
        v
[Validate rank, shape, dtype, overflow]
    ├── invalid -> return invalid-tensor status
    └── valid
        |
        v
[Select backend]
    ├── DX12 supported -> DX12 resource path
    ├── HIP supported -> HIP allocation path
    └── otherwise -> CPU allocation path or explicit unsupported status
        |
        v
[Select stream]
    ├── valid caller stream -> use it
    └── null -> use backend default stream
        |
        v
[Allocate/reuse memory]
    ├── budget/resource available -> return owned allocation
    └── unavailable -> return out-of-memory status; do not partially publish handle
```

## Truth Table

| Condition | Expected | Actual | PASS? |
|---|---|---|---|
| null output handle | invalid-argument status; output remains null | design requirement | PASS |
| zero bytes | invalid-size status | design requirement | PASS |
| shape product overflows 64-bit | invalid-tensor status | design requirement | PASS |
| unsupported dtype | unsupported-dtype status | design requirement | PASS |
| null stream | backend default stream | design requirement | PASS |
| valid explicit stream | operation uses explicit stream | design requirement | PASS |
| DX12 unavailable and CPU enabled | CPU reference allocation/path | design requirement | PASS |
| backend unavailable and CPU disabled | backend-unavailable status | design requirement | PASS |
| allocation fails | no published partially initialized handle | design requirement | PASS |
| tensor view | metadata-only view; no copy | design requirement | PASS |
| release twice | safe invalid-handle/error behavior | design requirement | PASS |

## Race Conditions

- [x] Handle publication occurs only after complete initialization.
- [x] Refcount or explicit ownership must protect allocations and views.
- [x] Stream affinity is recorded in the resource contract.
- [ ] Backend GPU synchronization will be validated with DX12/HIP hardware tests.

## Load Conditions

- [x] Metadata validation is CPU-only and bounded.
- [x] Allocation must enforce configured budget limits.
- [ ] GPU occupancy and VRAM pressure require backend benchmarks.

## VERDICT

**PASS — API behavior is specified; backend resource execution remains pending.**
