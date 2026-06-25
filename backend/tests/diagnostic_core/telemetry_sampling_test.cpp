#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/diagnostic_core/fake_telemetry_sampling_service.hpp"

using namespace kivo::cinema_engine;

// P2-043: Telemetry Sampling / Hot Path Policy tests

void test_frame_timing_buffer() {
    auto b = FakeTelemetrySamplingService::build_frame_timing_buffer();
    assert(b.metadata.schema_version == "1.1");
    assert(b.capacity_samples == 10000);
    assert(b.current_samples == 0);
    assert(b.sample_kind == "frame_timing");
    assert(b.is_bounded == true);
    std::cout << "PASS: test_frame_timing_buffer" << std::endl;
}

void test_audio_drift_buffer() {
    auto b = FakeTelemetrySamplingService::build_audio_drift_buffer();
    assert(b.sample_kind == "audio_drift");
    assert(b.capacity_samples == 5000);
    assert(b.is_bounded == true);
    std::cout << "PASS: test_audio_drift_buffer" << std::endl;
}

void test_full_buffer() {
    auto b = FakeTelemetrySamplingService::build_full_buffer();
    assert(b.current_samples == b.capacity_samples);
    assert(b.capacity_samples == 100);
    std::cout << "PASS: test_full_buffer" << std::endl;
}

void test_frame_timing_request() {
    auto r = FakeTelemetrySamplingService::build_frame_timing_request();
    assert(r.request_id == "req-frame-timing-001");
    assert(r.sample_kind == "frame_timing");
    assert(r.value == 16.67);
    assert(r.timestamp_ns > 0);
    std::cout << "PASS: test_frame_timing_request" << std::endl;
}

void test_audio_drift_request() {
    auto r = FakeTelemetrySamplingService::build_audio_drift_request();
    assert(r.sample_kind == "audio_drift");
    assert(r.value == 0.5);
    std::cout << "PASS: test_audio_drift_request" << std::endl;
}

void test_written_result() {
    auto res = FakeTelemetrySamplingService::build_written_result();
    assert(res.was_written == true);
    assert(res.was_overwritten == false);
    assert(res.current_occupancy == 1);
    std::cout << "PASS: test_written_result" << std::endl;
}

void test_overwritten_result() {
    auto res = FakeTelemetrySamplingService::build_overwritten_result();
    assert(res.was_written == true);
    assert(res.was_overwritten == true);
    assert(res.current_occupancy == 100);
    std::cout << "PASS: test_overwritten_result" << std::endl;
}

void test_hot_path_allowed_policy() {
    auto p = FakeTelemetrySamplingService::build_hot_path_allowed_policy();
    assert(p.allow_hot_path == true);
    assert(p.max_sample_rate_hz == 60);
    assert(p.ring_buffer_capacity == 10000);
    std::cout << "PASS: test_hot_path_allowed_policy" << std::endl;
}

void test_hot_path_forbidden_policy() {
    auto p = FakeTelemetrySamplingService::build_hot_path_forbidden_policy();
    assert(p.allow_hot_path == false);
    assert(p.max_sample_rate_hz == 10);
    assert(p.ring_buffer_capacity == 1000);
    std::cout << "PASS: test_hot_path_forbidden_policy" << std::endl;
}

void test_low_rate_policy() {
    auto p = FakeTelemetrySamplingService::build_low_rate_policy();
    assert(p.max_sample_rate_hz == 1);
    assert(p.allow_hot_path == false);
    assert(p.ring_buffer_capacity == 100);
    std::cout << "PASS: test_low_rate_policy" << std::endl;
}

int main() {
    // Ring buffer tests
    test_frame_timing_buffer();
    test_audio_drift_buffer();
    test_full_buffer();

    // Request tests
    test_frame_timing_request();
    test_audio_drift_request();

    // Result tests
    test_written_result();
    test_overwritten_result();

    // Policy tests
    test_hot_path_allowed_policy();
    test_hot_path_forbidden_policy();
    test_low_rate_policy();

    std::cout << "All P2-043 Telemetry Sampling / Hot Path Policy tests passed." << std::endl;
    return 0;
}
