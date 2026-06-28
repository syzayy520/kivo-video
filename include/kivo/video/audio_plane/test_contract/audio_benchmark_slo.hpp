// audio_benchmark_slo.hpp — P6A test_contract: benchmark SLO (§test_contract)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::test_contract {

// AudioBenchmarkSlo
// Service level objectives for benchmarks. Compile-only skeleton.
struct AudioBenchmarkSlo {
    kivo::video::audio_plane::ContractMetadata metadata{};
    int64_t event_render_p99_us{1000};
    int64_t low_latency_p99_us{500};
    int32_t hot_path_alloc_count{0};
    int64_t spsc_p99_us{10};
    int64_t seek_to_first_audio_ms{250};
    int64_t same_format_track_switch_ms{150};
    int64_t format_change_track_switch_ms{500};
    int64_t device_recovery_ms{2000};
    int64_t evidence_hot_submission_p99_us{10};

    bool operator==(const AudioBenchmarkSlo&) const = default;
};

}  // namespace kivo::video::audio_plane::test_contract
