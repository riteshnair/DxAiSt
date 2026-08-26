# TRACE: I/O provider contract

## Decision Tree

```text
[io_provider_create]
    |
    v
[requested provider]
    ├── DirectStorage requested and SDK/runtime available -> DirectStorage provider
    ├── DirectStorage unavailable or auto -> Win32 overlapped provider
    └── non-Windows host -> portable file provider
        |
        v
[read request]
    ├── invalid path/range/buffer -> structured input error
    ├── short file -> end-of-file/short-read status with exact bytes
    ├── provider error -> provider error; no partial success claim
    └── valid -> read bytes, emit I/O trace, return count
```

## Truth Table

| Condition | Expected | Result |
|---|---|---|
| null provider/output | invalid-argument | PASS |
| empty path | invalid-path | PASS |
| offset beyond file | end-of-file/zero bytes | PASS |
| buffer too small | invalid-capacity; no read | PASS |
| exact read | success and exact byte count | PASS |
| short read | success-with-short-read and exact count | PASS |
| DirectStorage unavailable | Win32/portable fallback | PASS |
| provider destroy with no requests | safe shutdown | PASS |
| component trace enabled | request/provider/duration records | PASS |
| component trace disabled | no trace file activity from provider | PASS |

## VERDICT

**PASS — provider selection and fallback are specified; actual DirectStorage queue/GPU decompression integration remains pending on the Windows SDK.**
