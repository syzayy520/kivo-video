#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/media_timeline_core/fake_master_clock_service.hpp"

using namespace kivo::cinema_engine;

static void test_audio_master_clock() {
    auto clock = build_audio_master_clock();
    assert(clock.clock_source == "audio");
    assert(clock.is_running);
    assert(clock.is_synced);
    assert(clock.current_time_us == 0);
    assert(clock.drift_ms == 0.0);
    std::cout << "  PASS: audio_master_clock\n";
}

static void test_stopped_clock() {
    auto clock = build_stopped_clock();
    assert(!clock.is_running);
    assert(!clock.is_synced);
    std::cout << "  PASS: stopped_clock\n";
}

static void test_default_av_sync_policy() {
    auto policy = build_default_av_sync_policy();
    assert(policy.sync_mode == "audio_master");
    assert(policy.max_drift_ms == 40.0);
    assert(policy.correction_threshold_ms == 20.0);
    assert(policy.correction_strategy == "skip_frame");
    assert(policy.allow_video_drop);
    assert(!policy.allow_audio_resample);
    std::cout << "  PASS: default_av_sync_policy\n";
}

static void test_custom_drift_policy() {
    auto policy = build_av_sync_policy_with_drift(100.0);
    assert(policy.max_drift_ms == 100.0);
    assert(policy.correction_threshold_ms == 50.0);
    std::cout << "  PASS: custom_drift_policy\n";
}

int main() {
    std::cout << "master_clock_av_sync_test:\n";
    test_audio_master_clock();
    test_stopped_clock();
    test_default_av_sync_policy();
    test_custom_drift_policy();
    std::cout << "ALL 4 TESTS PASSED\n";
    return 0;
}
