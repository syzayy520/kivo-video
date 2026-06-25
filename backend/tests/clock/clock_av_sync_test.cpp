#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/media_timeline_core/master_clock.hpp"
#include "kivo/cinema_engine/media_timeline_core/av_sync_policy.hpp"

using namespace kivo::cinema_engine;

// AvSyncDecision represents the result of an AV sync scheduling decision.
struct AvSyncDecision {
    std::string action;  // "play_video", "drop_video", "repeat_video", "adjust_speed"
    double drift_ms{0.0};
    bool is_within_threshold{true};
    std::string reason;
};

// Minimal AV sync logic: compare clock drift against policy thresholds.
AvSyncDecision make_av_sync_decision(const MasterClock& clock, const AvSyncPolicy& policy, int64_t video_pts_us, int64_t audio_pts_us) {
    AvSyncDecision decision;

    // Calculate drift: video ahead of audio is positive
    double drift = static_cast<double>(video_pts_us - audio_pts_us) / 1000.0;
    decision.drift_ms = drift;
    double abs_drift = (drift < 0.0) ? -drift : drift;

    if (abs_drift <= policy.correction_threshold_ms) {
        // Within threshold: play normally
        decision.action = "play_video";
        decision.is_within_threshold = true;
        decision.reason = "drift within correction threshold";
    } else if (abs_drift <= policy.max_drift_ms) {
        // Between threshold and max: correct
        if (drift > 0) {
            // Video ahead: drop frame
            decision.action = "drop_video";
            decision.is_within_threshold = false;
            decision.reason = "video ahead of audio, drop frame";
        } else {
            // Video behind: repeat frame
            decision.action = "repeat_video";
            decision.is_within_threshold = false;
            decision.reason = "video behind audio, repeat frame";
        }
    } else {
        // Beyond max drift: major correction
        if (drift > 0) {
            decision.action = "drop_video";
            decision.reason = "major drift: video far ahead";
        } else {
            decision.action = "repeat_video";
            decision.reason = "major drift: video far behind";
        }
        decision.is_within_threshold = false;
    }

    return decision;
}

int main() {
    std::cout << "clock_av_sync_test:\n";

    // Test 1: Initialize master clock
    MasterClock clock;
    clock.clock_id = "test-clock-001";
    clock.clock_source = "audio";
    clock.current_time_us = 0;
    clock.is_running = true;
    clock.is_synced = true;
    assert(clock.is_running);
    assert(clock.is_synced);
    std::cout << "  PASS: Master clock initialized, source=" << clock.clock_source << "\n";

    // Test 2: Initialize AV sync policy
    AvSyncPolicy policy;
    policy.policy_id = "test-sync-policy";
    policy.sync_mode = "audio_master";
    policy.max_drift_ms = 40.0;
    policy.correction_threshold_ms = 20.0;
    policy.correction_strategy = "skip_frame";
    policy.allow_video_drop = true;
    assert(policy.sync_mode == "audio_master");
    std::cout << "  PASS: AV sync policy initialized, mode=" << policy.sync_mode << "\n";

    // Test 3: First clock loop - sync point (zero drift)
    clock.current_time_us = 0;
    AvSyncDecision decision1 = make_av_sync_decision(clock, policy, 0, 0);
    assert(decision1.action == "play_video");
    assert(decision1.is_within_threshold);
    assert(decision1.drift_ms == 0.0);
    std::cout << "  PASS: First loop sync point, action=" << decision1.action
              << ", drift=" << decision1.drift_ms << "ms\n";

    // Test 4: Video slightly ahead (within threshold)
    AvSyncDecision decision2 = make_av_sync_decision(clock, policy, 10000, 0);  // 10ms ahead
    assert(decision2.action == "play_video");
    assert(decision2.is_within_threshold);
    std::cout << "  PASS: Video 10ms ahead, within threshold, action=" << decision2.action << "\n";

    // Test 5: Video ahead beyond threshold (drop frame)
    AvSyncDecision decision3 = make_av_sync_decision(clock, policy, 30000, 0);  // 30ms ahead
    assert(decision3.action == "drop_video");
    assert(!decision3.is_within_threshold);
    assert(decision3.drift_ms == 30.0);
    std::cout << "  PASS: Video 30ms ahead, beyond threshold, action=" << decision3.action << "\n";

    // Test 6: Video behind (repeat frame)
    AvSyncDecision decision4 = make_av_sync_decision(clock, policy, 0, 25000);  // 25ms behind
    assert(decision4.action == "repeat_video");
    assert(!decision4.is_within_threshold);
    std::cout << "  PASS: Video 25ms behind, action=" << decision4.action << "\n";

    // Test 7: Major drift (beyond max)
    AvSyncDecision decision5 = make_av_sync_decision(clock, policy, 50000, 0);  // 50ms ahead
    assert(decision5.action == "drop_video");
    assert(!decision5.is_within_threshold);
    std::cout << "  PASS: Major drift 50ms, action=" << decision5.action << "\n";

    // Test 8: Clock snapshot
    MasterClock snapshot = clock;
    snapshot.current_time_us = 1000000;  // 1 second
    snapshot.drift_ms = 1.5;
    assert(snapshot.current_time_us == 1000000);
    assert(snapshot.drift_ms == 1.5);
    std::cout << "  PASS: Clock snapshot: time=" << snapshot.current_time_us
              << "us, drift=" << snapshot.drift_ms << "ms\n";

    std::cout << "ALL 8 TESTS PASSED\n";
    return 0;
}
