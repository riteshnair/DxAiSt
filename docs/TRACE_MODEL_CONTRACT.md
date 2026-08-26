# TRACE: model container contract

## Decision Tree

```text
[model_open]
    |
    v
[validate path/output]
    ├── invalid -> invalid-argument/path
    └── valid -> open through dxio provider
        |
        v
[detect format]
    ├── GGUF magic -> GGUF metadata path
    ├── SafeTensors JSON header -> SafeTensors path
    ├── ONNX protobuf signature/extension -> ONNX path
    └── unknown -> generic binary asset or unsupported-format per policy
        |
        v
[read range]
    ├── offset/capacity invalid -> no read
    ├── provider failure -> provider error
    └── valid -> exact/short byte count and trace record
```

## Truth Table

| Condition | Expected | Result |
|---|---|---|
| null output/path | invalid-argument | PASS |
| missing file | open error | PASS |
| GGUF magic | format=GGUF | PASS |
| SafeTensors header | format=SafeTensors | PASS |
| ONNX extension/magic | format=ONNX | PASS |
| unknown extension | generic/unknown format, no unsafe execution | PASS |
| ranged read | exact byte count | PASS |
| malformed header | format remains unknown/error; no out-of-bounds read | PASS |
| oversized read request | bounded by caller capacity | PASS |
| model destroy | closes provider-owned state | PASS |

## VERDICT

**PASS — safe container detection and bounded reads are specified; tensor metadata import remains pending.**
