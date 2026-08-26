# TRACE: stream and event contract

## Decision Tree

```text
[stream_create]
    |
    v
[Validate device/output and stream kind]
    ├── invalid -> return error; output remains null
    └── valid -> create stream state
                    |
                    v
              [event_record]
                    |
                    v
              [stream valid?]
                ├── no -> invalid-handle
                └── yes -> increment monotonic sequence and publish event
                    |
                    v
              [event_wait]
                ├── event complete -> return immediately
                ├── event pending -> wait/poll according to mode
                └── invalid event -> invalid-handle
```

## Truth Table

| Condition | Expected | Result |
|---|---|---|
| null stream output | invalid-argument; output null | PASS |
| unsupported stream kind | invalid-stream-kind | PASS |
| valid stream | handle published after full init | PASS |
| record on valid stream | event sequence increases | PASS |
| record on null stream | invalid-handle | PASS |
| wait on completed event | immediate success | PASS |
| wait on pending event | waits or returns timeout per API | PASS |
| wait on null event | invalid-handle | PASS |
| destroy stream with events | events become invalid only after documented ownership transition | PASS |

## Race Conditions

- [x] Sequence publication occurs under stream lock.
- [x] Event state is monotonic: pending -> complete, never backwards.
- [x] Wait does not hold the stream lock while blocking.
- [ ] DX12 fence and HIP event behavior require target-backend validation.

## VERDICT

**PASS — portable asynchronous semantics are specified; backend queue integration remains pending.**
