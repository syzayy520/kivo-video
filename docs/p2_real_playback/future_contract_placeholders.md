# P2-001C Future Contract Placeholders

Updated: 2026-06-25

## Goal

Define contract placeholders for P2 features that are not yet implemented but need header stubs to prevent future reorganization.

## Placeholder Families

### 1. Source Adapter Core (P2.2)

```cpp
// source_core/source_adapter_contract.hpp
struct SourceAdapterContract {
    ContractMetadata metadata;
    std::string adapter_id;
    std::string adapter_kind; // "local", "webdav", "alist", "emby"
    std::vector<std::string> supported_operations;
    bool boundary_valid{true};
};
```

### 2. Source Health Core (P2.2)

```cpp
// source_core/source_health_snapshot.hpp
struct SourceHealthSnapshot {
    ContractMetadata metadata;
    std::string source_id;
    std::string health_status; // "healthy", "degraded", "unreachable"
    double throughput_estimate{0.0};
    double latency_estimate{0.0};
    std::string last_error;
    int64_t last_check_timestamp{0};
};
```

### 3. Network Security Policy (P2.2)

```cpp
// security_core/redirect_policy.hpp
struct RedirectPolicy {
    ContractMetadata metadata;
    int max_redirects{5};
    bool follow_cross_origin{false};
    bool strip_credentials_on_redirect{true};
    std::vector<std::string> allowed_schemes;
};
```

### 4. Segment Cache Runtime (P2.2)

```cpp
// network_cache_core/segment_cache_runtime.hpp
struct SegmentCacheRuntime {
    ContractMetadata metadata;
    std::string cache_id;
    std::string bound_remote_object_identity_id;
    int64_t total_bytes{0};
    int64_t cached_bytes{0};
    double hit_rate{0.0};
};
```

### 5. HDR Detection (P2.3)

```cpp
// color_science_core/hdr_detection.hpp
struct HdrDetectionResult {
    ContractMetadata metadata;
    std::string input_signal; // "SDR", "HDR10", "HLG", "DV_Profile5", etc.
    bool hdr_metadata_present{false};
    bool dv_rpu_present{false};
    std::string dv_profile;
    std::string detection_confidence;
};
```

### 6. Subtitle Overlay (P2.3)

```cpp
// subtitle_core/subtitle_overlay.hpp
struct SubtitleOverlayContract {
    ContractMetadata metadata;
    std::string overlay_backend; // "d3d11_texture", "software_blit"
    bool supports_hdr_luminance{false};
    bool supports_ass_advanced{false};
    std::string font_render_backend;
};
```

### 7. Inspector Runtime (P2.3)

```cpp
// playback_inspector_core/inspector_runtime.hpp
struct InspectorRuntimeContract {
    ContractMetadata metadata;
    std::string inspector_id;
    bool reads_decision_ledger{true};
    bool reads_telemetry_timeline{true};
    bool reads_capability_join{true};
    bool supports_redaction{true};
};
```

### 8. Telemetry Ring Buffer (P2.3)

```cpp
// diagnostic_core/telemetry_ring_buffer.hpp
struct TelemetryRingBuffer {
    ContractMetadata metadata;
    std::string buffer_id;
    int64_t capacity_samples{10000};
    int64_t current_samples{0};
    std::string sample_kind; // "frame_timing", "audio_drift", "network_throughput"
    bool is_bounded{true};
};
```

### 9. Schema Migration Policy (P2.3)

```cpp
// governance_core/schema_migration_policy.hpp
struct SchemaMigrationPolicy {
    ContractMetadata metadata;
    std::string schema_name;
    int from_version{0};
    int to_version{0};
    std::string migration_strategy; // "forward_only", "backward_compatible", "destructive"
    bool requires_backup{true};
};
```

### 10. Claim Vocabulary Gate (P2.3)

```cpp
// governance_core/claim_vocabulary_gate.hpp
struct ClaimVocabularyGate {
    ContractMetadata metadata;
    std::vector<std::string> allowed_hdr_claims;
    std::vector<std::string> allowed_audio_claims;
    std::vector<std::string> forbidden_claim_patterns;
    std::string enforcement_level; // "strict", "warn", "audit"
};
```

## Implementation Plan

1. Create header-only stubs in `include/kivo/cinema_engine/`
2. No .cpp files needed (header-only contracts)
3. No tests needed yet (placeholders only)
4. Governance gate verifies headers compile

## Verification

P2-001C is verified when:
1. All placeholder headers compile with MSVC C++20
2. Governance gate passes with new headers
3. No implementation logic in placeholder headers
4. `cmake --build --preset ninja-debug` succeeds
