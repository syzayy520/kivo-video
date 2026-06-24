# P2-012 FFmpeg Probe + StreamInventory

Updated: 2026-06-25

## Goal

Implement FFmpeg probe functionality behind adapter boundary to extract StreamInventory from media files.

## Adapter Boundary Principle

FFmpeg types (AVFormatContext, AVStream, etc.) must NOT cross the adapter boundary. The probe service must:
1. Accept platform-neutral input (file path, byte stream)
2. Return platform-neutral output (StreamInventory)
3. Contain all FFmpeg-specific logic within adapter implementation

## Probe Contract

```cpp
struct ProbeRequest {
    ContractMetadata metadata;
    std::string source_uri;
    std::optional<std::string> format_hint;
    bool fast_probe{true};
};

struct ProbeResult {
    ContractMetadata metadata;
    StreamInventory inventory;
    bool success{false};
    std::string error_message;
};
```

## Fake Probe Service

Provides deterministic probe results for testing without real FFmpeg dependency:
- `build_successful_probe_result()` - returns valid StreamInventory
- `build_failed_probe_result()` - returns error state
- `build_probe_result_with_streams()` - customizable stream inventory

## Real Probe Service (Future)

Will implement actual FFmpeg probe behind adapter boundary:
1. Accept ProbeRequest
2. Use FFmpeg API to probe file
3. Extract stream information
4. Build StreamInventory
5. Return ProbeResult

## Tests

1. Successful probe returns valid StreamInventory
2. Failed probe returns error state
3. Custom stream inventory construction
4. ProbeRequest field validation
5. ProbeResult field validation