# TRACE: memory allocation contract

## Decision Tree

```text
[dx_memory_alloc]
    |
    v
[Validate pool, output, bytes, alignment]
    ├── invalid -> return invalid-argument/status; output remains null
    └── valid
        |
        v
[Normalize alignment]
    ├── power-of-two and >= pointer alignment -> keep
    └── otherwise -> return invalid-alignment status
        |
        v
[Allocate host backing for portable implementation]
    ├── success -> publish allocation and update stats
    └── failure -> return out-of-memory; stats unchanged
        |
        v
[dx_memory_free]
    ├── owned allocation -> free once and update stats
    └── foreign/unknown allocation -> return invalid-handle; do not free
```

## Truth Table

| Condition | Expected | Result |
|---|---|---|
| null pool | invalid-handle | PASS |
| null output | invalid-argument | PASS |
| zero bytes | invalid-size | PASS |
| non-power-of-two alignment | invalid-alignment | PASS |
| allocation succeeds | handle published after full initialization | PASS |
| allocation fails | no partial handle; stats unchanged | PASS |
| free valid handle once | allocation released | PASS |
| free valid handle twice | invalid-handle; no double free | PASS |
| stats query null output | invalid-argument | PASS |
| shutdown with live allocations | pool releases remaining allocations safely | PASS |

## Race Conditions

- [x] Pool mutex protects allocation map and statistics.
- [x] Allocation handle is published only after backing memory is ready.
- [x] Double-free is rejected by ownership lookup.
- [ ] GPU fence-based retirement is pending for the DX12 implementation.

## VERDICT

**PASS — portable host allocator behavior is specified; GPU heap/residency integration remains pending.**
