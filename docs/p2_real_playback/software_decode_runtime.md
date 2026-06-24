# P2-016 Software Decode Runtime

Updated: 2026-06-25

## Goal

Implement software decode runtime producing decoded audio/video frames using FFmpeg behind adapter boundary.

## Adapter Boundary Principle

FFmpeg types (AVFrame, AVCodecContext, etc.) must NOT cross the adapter boundary. The decode runtime must:
1. Accept platform-neutral input (KivoPacket)
2. Return platform-neutral output (DecodedFrame)
3. Contain all FFmpeg-specific logic within adapter implementation

## Decode Runtime Contract

```cpp
struct DecodeRequest {
    ContractMetadata metadata;
    KivoPacket packet;
    bool force_software{true};
};

struct DecodeResult {
    ContractMetadata metadata;
    DecodedFrame frame;
    bool success{false};
    bool eof{false};
    bool needs_more_input{false};
    std::string error_message;
};
```

## Fake Decode Runtime

Provides deterministic decode results for testing without real FFmpeg dependency:
- `build_successful_decode_result()` - returns valid DecodedFrame
- `build_eof_decode_result()` - returns EOF state
- `build_needs_more_input_result()` - returns needs more input state
- `build_failed_decode_result()` - returns error state
- `build_decode_result_with_frame()` - customizable frame

## Tests

1. Successful decode returns valid DecodedFrame
2. EOF detection
3. Needs more input detection
4. Failed decode returns error state
5. Custom frame construction
6. DecodeRequest field validation
7. DecodeResult field validation