# TRACE: cross-DLL trace session contract

## Decision Tree

```text
[trace_session_create]
    |
    v
[validate output/name]
    ├── invalid -> return error; output remains null
    └── valid -> capture session metadata
        |
        v
[trace_begin]
    ├── invalid session/name -> error; no event appended
    └── valid -> append open span with request/thread IDs
        |
        v
[trace_end]
    ├── invalid span or already closed -> error/no duplicate close
    └── valid -> record end timestamp and duration
        |
        v
[trace_export]
    ├── invalid path/session -> error
    └── valid -> write complete Chrome Trace JSON atomically enough for a single process
```

## Truth Table

| Condition | Expected | Result |
|---|---|---|
| null session output | invalid-argument; output null | PASS |
| empty span name | invalid-argument; no event | PASS |
| valid begin | span handle and event created | PASS |
| valid end | duration recorded exactly once | PASS |
| double end | error; stored event unchanged | PASS |
| null span | invalid-handle | PASS |
| export to valid path | JSON document contains complete events | PASS |
| export invalid path | error; process continues | PASS |
| concurrent spans | all events retained without data race | PASS |
| tracing disabled at caller | caller avoids session calls | PASS |

## Race Conditions

- [x] Session event storage is protected by a mutex.
- [x] Span ownership is removed from the open-span map exactly once.
- [x] Export copies or holds the session lock while serializing.
- [ ] GPU timestamp calibration is pending for backend implementations.

## VERDICT

**PASS — host trace correlation and export behavior are specified; GPU timestamp integration remains pending.**
