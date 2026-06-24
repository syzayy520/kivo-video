# P2-019 WASAPI Shared PCM Output

Updated: 2026-06-25

## Goal

Implement WASAPI shared PCM output test or manual gate.

## WASAPI Shared PCM Output

The WASAPI shared PCM output handles audio playback in Windows shared mode. Key components:

### Audio Endpoint Runtime Contract

Already defined in `audio_core/audio_endpoint_contract.hpp`:

```cpp
struct AudioEndpointRuntimeContract {
    ContractMetadata metadata;
    std::string endpoint_id;
    std::string endpoint_name;
    std::string output_mode;  // "shared_pcm", "exclusive_pcm", "passthrough"
    int32_t sample_rate{0};
    int32_t channels{0};
    int32_t bit_depth{0};
    bool is_initialized{false};
    bool is_playing{false};
    int64_t samples_written{0};
    double buffer_duration_ms{0.0};
};
```

### Fake Audio Endpoint Service

Provides deterministic audio endpoint results for testing:

- `build_shared_pcm_endpoint()` - returns shared PCM endpoint with configurable sample rate, channels, bit depth
- `build_uninitialized_endpoint()` - returns uninitialized endpoint
- `build_playing_endpoint()` - returns playing endpoint with samples written
- `build_exclusive_pcm_endpoint()` - returns exclusive PCM endpoint
- `build_passthrough_endpoint()` - returns passthrough endpoint for encoded audio

## Tests

1. Shared PCM endpoint with default parameters
2. Uninitialized endpoint
3. Playing endpoint with sample count
4. Exclusive PCM endpoint
5. Passthrough endpoint

## Evidence Target

Shared-mode PCM output test or manual gate.