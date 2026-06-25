#pragma once

#include "kivo/cinema_engine/diagnostic_core/telemetry_ring_buffer.hpp"
#include "kivo/cinema_engine/diagnostic_core/telemetry_sample_request.hpp"
#include "kivo/cinema_engine/diagnostic_core/telemetry_sample_result.hpp"
#include "kivo/cinema_engine/diagnostic_core/telemetry_sampling_policy.hpp"

namespace kivo::cinema_engine {

// Fake telemetry sampling service for P2-043 testing.
class FakeTelemetrySamplingService {
public:
    // Ring buffer builders
    static TelemetryRingBuffer build_frame_timing_buffer();
    static TelemetryRingBuffer build_audio_drift_buffer();
    static TelemetryRingBuffer build_network_throughput_buffer();
    static TelemetryRingBuffer build_full_buffer();

    // Request builders
    static TelemetrySampleRequest build_frame_timing_request();
    static TelemetrySampleRequest build_audio_drift_request();
    static TelemetrySampleRequest build_network_throughput_request();

    // Result builders
    static TelemetrySampleResult build_written_result();
    static TelemetrySampleResult build_overwritten_result();
    static TelemetrySampleResult build_rejected_result();

    // Policy builders
    static TelemetrySamplingPolicy build_hot_path_allowed_policy();
    static TelemetrySamplingPolicy build_hot_path_forbidden_policy();
    static TelemetrySamplingPolicy build_low_rate_policy();
};

}  // namespace kivo::cinema_engine
