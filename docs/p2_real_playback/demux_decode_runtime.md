# P2-015 FFmpeg Demux Runtime

Updated: 2026-06-25

## Goal

Implement real demux runtime producing KivoPacket using FFmpeg behind adapter boundary.

## Adapter Boundary Principle

FFmpeg types (AVFormatContext, AVPacket, etc.) must NOT cross the adapter boundary. The demux runtime must:
1. Accept platform-neutral input (file path, stream inventory)
2. Return platform-neutral output (KivoPacket)
3. Contain all FFmpeg-specific logic within adapter implementation

## Demux Runtime Contract

```cpp
struct DemuxRequest {
    ContractMetadata metadata;
    std::string source_uri;
    StreamInventory inventory;
    bool fast_seek{true};
};

struct DemuxResult {
    ContractMetadata metadata;
    KivoPacket packet;
    bool success{false};
    bool eof{false};
    std::string error_message;
};
```

## Fake Demux Runtime

Provides deterministic demux results for testing without real FFmpeg dependency:
- `build_successful_demux_result()` - returns valid KivoPacket
- `build_eof_demux_result()` - returns EOF state
- `build_failed_demux_result()` - returns error state
- `build_demux_result_with_packet()` - customizable packet

## Tests

1. Successful demux returns valid KivoPacket
2. EOF detection
3. Failed demux returns error state
4. Custom packet construction
5. DemuxRequest field validation
6. DemuxResult field validation