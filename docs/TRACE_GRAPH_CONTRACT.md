# TRACE: graph and operator contract

## Decision Tree

```text
[graph_create]
    |
    v
[validate graph/output]
    ├── invalid -> structured error
    └── valid
        |
        v
[add operator]
    ├── registered op -> validate tensor constraints and append node
    └── unknown op -> unsupported-operation error
        |
        v
[compile graph]
    ├── cycle or invalid dependency -> graph error
    ├── unsupported backend/dtype -> fallback or unsupported error
    └── valid -> topologically order nodes and publish immutable plan
        |
        v
[execute plan]
    ├── valid bindings -> execute in plan order and trace each node
    └── invalid bindings -> binding error; no partial execution
```

## Truth Table

| Condition | Expected | Result |
|---|---|---|
| null graph output | invalid-argument; output null | PASS |
| unknown operator name | unsupported-operation | PASS |
| malformed input/output index | invalid-node | PASS |
| dependency refers to missing node | invalid-dependency | PASS |
| cyclic dependency | graph-cycle | PASS |
| valid graph | immutable plan published after compile | PASS |
| plan execution with valid bindings | all nodes execute in topological order | PASS |
| missing input binding | binding error; no dispatch | PASS |
| unsupported dtype/backend | fallback if registered, otherwise structured error | PASS |
| repeated execution | plan reused; no recompilation | PASS |

## Race Conditions

- [x] Graph mutation is separate from immutable plan execution.
- [x] Plan publication occurs only after complete compilation.
- [x] Operator registry reads are safe after registration freeze.
- [ ] Backend queue synchronization is pending for DX12/HIP implementations.

## VERDICT

**PASS — graph behavior is specified; native operator execution remains pending.**
