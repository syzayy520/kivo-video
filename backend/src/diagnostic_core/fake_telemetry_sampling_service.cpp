#include "kivo/cinema_engine/diagnostic_core/fake_telemetry_sampling_service.hpp"

namespace kivo::cinema_engine {

// --- Ring buffer builders ---

TelemetryRingBuffer FakeTelemetrySamplingService::build_frame_timing_buffer() {
    TelemetryRingBuffer b;
    b.metadata.schema_version = "1.1";
    b.buffer_id = "buf-frame-timing-001";
    b.capacity_samples = 10000;
    b.current_samples = 0;
    b.sample_kind = "frame_timing";
    b.is_bounded = true;
    return b;
}

TelemetryRingBuffer FakeTelemetrySamplingService::build_audio_drift_buffer() {
    TelemetryRingBuffer b;
    b.metadata.schema_version = "1.1";
    b.buffer_id = "buf-audio-drift-001";
    b.capacity_samples = 5000;
    b.current_samples = 0;
    b.sample_kind = "audio_drift";
    b.is_bounded = true;
    return b;
}

TelemetryRingBuffer FakeTelemetrySamplingService::build_network_throughput_buffer() {
    TelemetryRingBuffer b;
    b.metadata.schema_version = "1.1";
    b.buffer_id = "buf-network-throughput-001";
    b.capacity_samples = 2000;
    b.current_samples = 0;
    b.sample_kind = "network_throughput";
    b.is_bounded = true;
    return b;
}

TelemetryRingBuffer FakeTelemetrySamplingService::build_full_buffer() {
    TelemetryRingBuffer b;
    b.metadata.schema_version = "1.1";
    b.buffer_id = "buf-full-001";
    b.capacity_samples = 100;
    b.current_samples = 100;
    b.sample_kind = "frame_timing";
    b.is_bounded = true;
    return b;
}

// --- Request builders ---

TelemetrySampleRequest FakeTelemetrySamplingService::build_frame_timing_request() {
    TelemetrySampleRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "req-frame-timing-001";
    r.buffer_id = "buf-frame-timing-001";
    r.sample_kind = "frame_timing";
    r.timestamp_ns = 1000000000;  // 1 second
    r.value = 16.67;  // 16.67ms frame time
    return r;
}

TelemetrySampleRequest FakeTelemetrySamplingService::build_audio_drift_request() {
    TelemetrySampleRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "req-audio-drift-001";
    r.buffer_id = "buf-audio-drift-001";
    r.sample_kind = "audio_drift";
    r.timestamp_ns = 2000000000;
    r.value = 0.5;  // 0.5ms drift
    return r;
}

TelemetrySampleRequest FakeTelemetrySamplingService::build_network_throughput_request() {
    TelemetrySampleRequest r;
    r.metadata.schema_version = "1.1";
    r.request_id = "req-network-throughput-001";
    r.buffer_id = "buf-network-throughput-001";
    r.sample_kind = "network_throughput";
    r.timestamp_ns = 3000000000;
    r.value = 5000000.0;  // 5 MB/s
    return r;
}

// --- Result builders ---

TelemetrySampleResult FakeTelemetrySamplingService::build_written_result() {
    TelemetrySampleResult res;
    res.metadata.schema_version = "1.1";
    res.result_id = "res-written-001";
    res.request_id = "req-frame-timing-001";
    res.was_written = true;
    res.was_overwritten = false;
    res.current_occupancy = 1;
    return res;
}

TelemetrySampleResult FakeTelemetrySamplingService::build_overwritten_result() {
    TelemetrySampleResult res;
    res.metadata.schema_version = "1.1";
    res.result_id = "res-overwritten-001";
    res.request_id = "req-frame-timing-002";
    res.was_written = true;
    res.was_overwritten = true;
    res.current_occupancy = 100;
    return res;
}

TelemetrySampleResult FakeTelemetrySamplingService::build_rejected_result() {
    TelemetrySampleResult res;
    res.metadata.schema_version = "1.1";
    res.result_id = "res-rejected-001";
    res.request_id = "req-frame-timing-003";
    res.was_written = false;
    res.was_overwritten = false;
    res.current_occupancy = 100;
    return res;
}

// --- Policy builders ---

TelemetrySamplingPolicy FakeTelemetrySamplingService::build_hot_path_allowed_policy() {
    TelemetrySamplingPolicy p;
    p.metadata.schema_version = "1.1";
    p.policy_id = "policy-hot-path-allowed";
    p.max_sample_rate_hz = 60;
    p.allow_hot_path = true;
    p.ring_buffer_capacity = 10000;
    return p;
}

TelemetrySamplingPolicy FakeTelemetrySamplingService::build_hot_path_forbidden_policy() {
    TelemetrySamplingPolicy p;
    p.metadata.schema_version = "1.1";
    p.policy_id = "policy-hot-path-forbidden";
    p.max_sample_rate_hz = 10;
    p.allow_hot_path = false;
    p.ring_buffer_capacity = 1000;
    return p;
}

TelemetrySamplingPolicy FakeTelemetrySamplingService::build_low_rate_policy() {
    TelemetrySamplingPolicy p;
    p.metadata.schema_version = "1.1";
    p.policy_id = "policy-low-rate";
    p.max_sample_rate_hz = 1;
    p.allow_hot_path = false;
    p.ring_buffer_capacity = 100;
    return p;
}

}  // namespace kivo::cinema_engine
