# P2-017 SDR Video Upload + Visual Quality Baseline

Updated: 2026-06-25

## Goal

Implement SDR video upload route and visual quality baseline for local playback.

## SDR Video Upload

The SDR video upload route handles non-HDR video content. Key components:

### SDR Upload Config

```cpp
struct SdrUploadConfig {
    ContractMetadata metadata;
    std::string upload_id;
    bool enable_color_correction{true};
    bool enable_sharpening{false};
    float brightness_adjustment{0.0f};
    float contrast_adjustment{1.0f};
    float saturation_adjustment{1.0f};
};
```

### SDR Upload Result

```cpp
struct SdrUploadResult {
    ContractMetadata metadata;
    std::string upload_id;
    bool success{false};
    std::string error_message;
    int32_t frames_processed{0};
    int32_t frames_skipped{0};
};
```

## Visual Quality Baseline

The visual quality baseline provides metrics for video quality assessment:

### Visual Quality Metrics

```cpp
struct VisualQualityMetrics {
    ContractMetadata metadata;
    std::string metrics_id;
    float psnr{0.0f};  // Peak Signal-to-Noise Ratio
    float ssim{0.0f};  // Structural Similarity Index
    float vmaf{0.0f};  // Video Multimethod Assessment Fusion
    float bitrate_efficiency{0.0f};
    int32_t frame_count{0};
    bool is_hdr{false};
};
```

### Visual Quality Result

```cpp
struct VisualQualityResult {
    ContractMetadata metadata;
    VisualQualityMetrics metrics;
    bool meets_baseline{false};
    std::string quality_tier;  // "excellent", "good", "acceptable", "poor"
    std::vector<std::string> warnings;
};
```

## Fake SDR Upload Service

Provides deterministic SDR upload results for testing:

- `build_default_sdr_upload_config()` - returns default config
- `build_successful_sdr_upload_result()` - returns success result
- `build_failed_sdr_upload_result()` - returns failure result
- `build_custom_sdr_upload_result()` - customizable result

## Fake Visual Quality Service

Provides deterministic visual quality results for testing:

- `build_excellent_visual_quality_result()` - returns excellent quality
- `build_good_visual_quality_result()` - returns good quality
- `build_acceptable_visual_quality_result()` - returns acceptable quality
- `build_poor_visual_quality_result()` - returns poor quality
- `build_custom_visual_quality_result()` - customizable result

## Tests

1. SDR upload default config
2. SDR upload successful result
3. SDR upload failed result
4. SDR upload custom result
5. Visual quality excellent tier
6. Visual quality good tier
7. Visual quality acceptable tier
8. Visual quality poor tier
9. Visual quality custom result
10. Visual quality baseline check