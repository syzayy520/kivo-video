# P2-018 D3D11 Flip Presenter First Frame

Updated: 2026-06-25

## Goal

Implement D3D11 flip presenter first frame integration test or manual gate.

## D3D11 Flip Presenter First Frame

The D3D11 flip presenter first frame handles the initial frame presentation in D3D11 flip mode. Key components:

### First Frame Config

```cpp
struct FlipPresenterFirstFrameConfig {
    ContractMetadata metadata;
    std::string config_id;
    bool enable_hdr{false};
    bool enable_zero_copy{true};
    int32_t width{1920};
    int32_t height{1080};
    std::string pixel_format;  // "rgba8", "rgba16f", "nv12"
    bool vsync_enabled{true};
    int32_t max_latency{2};
};
```

### First Frame Result

```cpp
struct FlipPresenterFirstFrameResult {
    ContractMetadata metadata;
    std::string config_id;
    bool success{false};
    bool first_frame_presented{false};
    int64_t presentation_time_us{0};
    std::string error_message;
    bool hdr_active{false};
    bool zero_copy_active{false};
};
```

## Fake D3D11 Flip Presenter Service

Provides deterministic D3D11 flip presenter results for testing:

- `build_default_d3d11_flip_first_frame_config()` - returns default config
- `build_successful_first_frame_result()` - returns success result
- `build_failed_first_frame_result()` - returns failure result
- `build_hdr_first_frame_result()` - returns HDR result
- `build_zero_copy_first_frame_result()` - returns zero-copy result

## Tests

1. D3D11 flip first frame default config
2. D3D11 flip first frame successful result
3. D3D11 flip first frame failed result
4. D3D11 flip first frame HDR result
5. D3D11 flip first frame zero-copy result
6. D3D11 flip first frame custom config
7. D3D11 flip first frame validation