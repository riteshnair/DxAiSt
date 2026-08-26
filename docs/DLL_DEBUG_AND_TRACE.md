# DLL-scoped debug and tracing

## Naming contract

A component is the **DLL component name**. It is not a C++ class, namespace, operator, or source directory.

The canonical environment-variable form is:

```text
dx12_<dll_component>_<mode>=<value>
```

Examples:

```text
dx12_dxcore_debug=1
dx12_dxcore_trace=1
dx12_dxmemory_perf=1
dx12_dxio_diag=1
dx12_dxattention_debug=1
```

Values are enabled when they are non-empty and not one of `0`, `false`, `off`, or `no`, case-insensitively. The four initial modes are independent.

| Mode | Intended content |
|---|---|
| `debug` | Verbose component diagnostics, configuration, resource identity, and branch decisions |
| `trace` | API spans, graph events, queue submissions, fences, I/O requests, and correlated operations |
| `perf` | Durations, counters, bandwidth, dispatch counts, cache outcomes, and memory high-water marks |
| `diag` | Validation-layer output, device removal information, shader diagnostics, and numerical checks |

## Log location

When a mode is enabled, the DLL resolves its own loaded-module directory and creates:

```text
<dll-directory>\logs\<component>_YYYYMMDD_HHMMSS_PID.log
```

For example:

```text
C:\Apps\DxAiSt\bin\logs\dxcore_20260826_101530_18472.log
```

The log records are newline-delimited JSON. The first line identifies the component and active mode bitset. Each subsequent record includes timestamp, process ID, thread ID, component, level, source file, source line, and message.

If module-directory resolution or directory creation fails, the logger attempts `<current-working-directory>\logs`. If that also fails, logging is disabled without failing the inference operation.

## Runtime policy

Each DLL reads only its own component variables. For example, `dx12_dxmemory_perf=1` does not enable `dxcore` debug output. A future process-wide tracing controller may explicitly enable multiple variables, but the individual DLLs remain independently controllable.

Environment values are captured during explicit component initialization rather than in static constructors. This avoids filesystem activity under the Windows loader lock and makes DLL startup deterministic.

## Performance requirements

The disabled path must avoid file I/O, logger locks, timestamp formatting, and message construction at the call site. The full path must be thread-safe, buffered, timestamped, and non-fatal. Performance mode must eventually add GPU timestamp correlation; the initial core logger records host-side spans and is not yet a GPU profiler.

## Component audit

A component is not complete until its variable names, log path, trace schema, disabled-mode overhead, production call sites, tests, and failure behavior are documented in the component status ledger and pass the completion-audit gates.
