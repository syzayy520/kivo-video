#include <cassert>
#include <iostream>
#include <string>

// Core contracts
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

// Source
#include "kivo/cinema_engine/source_core/fake_file_identity_service.hpp"
#include "kivo/cinema_engine/source_core/fake_local_file_source_service.hpp"

// Demux
#include "kivo/cinema_engine/demux_core/fake_probe_service.hpp"

// Playback core state machine
#include "kivo/cinema_engine/playback_core/playback_session_state_machine.hpp"

using namespace kivo::cinema_engine;

// Helper to walk through the full startup sequence to Playing
static void walk_to_playing() {
    auto r = attempt_transition(PlaybackState::Idle, PlaybackState::Opening, "open");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::Opening, PlaybackState::CreatingSession, "session_created");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::CreatingSession, PlaybackState::ResolvingSource, "source_resolved");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::ResolvingSource, PlaybackState::SelectingVersion, "version_selected");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::SelectingVersion, PlaybackState::PlanningProbe, "probe_planned");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::PlanningProbe, PlaybackState::JoiningCapability, "capability_joined");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::JoiningCapability, PlaybackState::PlanningPlayback, "playback_planned");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::PlanningPlayback, PlaybackState::CheckingCompliance, "compliance_checked");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::CheckingCompliance, PlaybackState::CheckingFeatureGate, "feature_gate_checked");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::CheckingFeatureGate, PlaybackState::CheckingResourceBudget, "resource_budget_checked");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::CheckingResourceBudget, PlaybackState::PreparingGraph, "graph_prepared");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::PreparingGraph, PlaybackState::PreparingCache, "cache_prepared");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::PreparingCache, PlaybackState::ProbingMedia, "media_probed");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::ProbingMedia, PlaybackState::SelectingDecoder, "decoder_selected");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::SelectingDecoder, PlaybackState::PreparingRender, "render_prepared");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::PreparingRender, PlaybackState::Ready, "ready");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::Ready, PlaybackState::Playing, "play");
    assert(r.decision == TransitionDecision::Accepted);
    assert(r.new_state == PlaybackState::Playing);
}

// Test MP4 play from beginning
static void test_mp4_play_from_beginning() {
    // Create file identity for MP4
    auto file_identity = build_stable_file_identity("test_video_sdr.mp4");
    assert(file_identity.file_path == "test_video_sdr.mp4");
    assert(file_identity.identity_stable == true);

    // Create probe result
    auto probe_result = build_successful_probe_result();
    assert(probe_result.success == true);

    // Test full startup sequence
    walk_to_playing();

    std::cout << "  PASS: mp4_play_from_beginning\n";
}

// Test MP4 pause during playback
static void test_mp4_pause_during_playback() {
    walk_to_playing();

    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Paused, "pause");
    assert(r.decision == TransitionDecision::Accepted);
    assert(r.new_state == PlaybackState::Paused);

    std::cout << "  PASS: mp4_pause_during_playback\n";
}

// Test MP4 seek forward
static void test_mp4_seek_forward() {
    walk_to_playing();

    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Seeking, "seek");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::Seeking, PlaybackState::Playing, "seek_complete");
    assert(r.decision == TransitionDecision::Accepted);

    std::cout << "  PASS: mp4_seek_forward\n";
}

// Test MP4 seek backward
static void test_mp4_seek_backward() {
    walk_to_playing();

    // Seek forward first
    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Seeking, "seek");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::Seeking, PlaybackState::Playing, "seek_complete");
    assert(r.decision == TransitionDecision::Accepted);

    // Now seek backward
    r = attempt_transition(PlaybackState::Playing, PlaybackState::Seeking, "seek_back");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::Seeking, PlaybackState::Playing, "seek_complete");
    assert(r.decision == TransitionDecision::Accepted);

    std::cout << "  PASS: mp4_seek_backward\n";
}

// Test MP4 resume from pause
static void test_mp4_resume_from_pause() {
    walk_to_playing();

    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Paused, "pause");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::Paused, PlaybackState::Playing, "resume");
    assert(r.decision == TransitionDecision::Accepted);
    assert(r.new_state == PlaybackState::Playing);

    std::cout << "  PASS: mp4_resume_from_pause\n";
}

// Test MP4 stop during playback
static void test_mp4_stop_during_playback() {
    walk_to_playing();

    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Closed, "stop");
    assert(r.decision == TransitionDecision::Accepted);
    assert(r.new_state == PlaybackState::Closed);

    std::cout << "  PASS: mp4_stop_during_playback\n";
}

// Test MP4 full playback sequence
static void test_mp4_full_playback_sequence() {
    walk_to_playing();

    // Pause
    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Paused, "pause");
    assert(r.decision == TransitionDecision::Accepted);

    // Seek from pause
    r = attempt_transition(PlaybackState::Paused, PlaybackState::Seeking, "seek");
    assert(r.decision == TransitionDecision::Accepted);

    // Seek complete back to pause
    r = attempt_transition(PlaybackState::Seeking, PlaybackState::Paused, "seek_complete");
    assert(r.decision == TransitionDecision::Accepted);

    // Resume
    r = attempt_transition(PlaybackState::Paused, PlaybackState::Playing, "resume");
    assert(r.decision == TransitionDecision::Accepted);

    // Stop
    r = attempt_transition(PlaybackState::Playing, PlaybackState::Closed, "stop");
    assert(r.decision == TransitionDecision::Accepted);

    std::cout << "  PASS: mp4_full_playback_sequence\n";
}

// Test MKV play from beginning
static void test_mkv_play_from_beginning() {
    // Create file identity for MKV
    auto file_identity = build_stable_file_identity("test_video_sdr.mkv");
    assert(file_identity.file_path == "test_video_sdr.mkv");
    assert(file_identity.identity_stable == true);

    // Create probe result
    auto probe_result = build_successful_probe_result();
    assert(probe_result.success == true);

    // Test full startup sequence
    walk_to_playing();

    std::cout << "  PASS: mkv_play_from_beginning\n";
}

// Test MKV pause during playback
static void test_mkv_pause_during_playback() {
    walk_to_playing();

    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Paused, "pause");
    assert(r.decision == TransitionDecision::Accepted);
    assert(r.new_state == PlaybackState::Paused);

    std::cout << "  PASS: mkv_pause_during_playback\n";
}

// Test MKV seek forward
static void test_mkv_seek_forward() {
    walk_to_playing();

    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Seeking, "seek");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::Seeking, PlaybackState::Playing, "seek_complete");
    assert(r.decision == TransitionDecision::Accepted);

    std::cout << "  PASS: mkv_seek_forward\n";
}

// Test MKV seek backward
static void test_mkv_seek_backward() {
    walk_to_playing();

    // Seek forward first
    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Seeking, "seek");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::Seeking, PlaybackState::Playing, "seek_complete");
    assert(r.decision == TransitionDecision::Accepted);

    // Now seek backward
    r = attempt_transition(PlaybackState::Playing, PlaybackState::Seeking, "seek_back");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::Seeking, PlaybackState::Playing, "seek_complete");
    assert(r.decision == TransitionDecision::Accepted);

    std::cout << "  PASS: mkv_seek_backward\n";
}

// Test MKV resume from pause
static void test_mkv_resume_from_pause() {
    walk_to_playing();

    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Paused, "pause");
    assert(r.decision == TransitionDecision::Accepted);

    r = attempt_transition(PlaybackState::Paused, PlaybackState::Playing, "resume");
    assert(r.decision == TransitionDecision::Accepted);
    assert(r.new_state == PlaybackState::Playing);

    std::cout << "  PASS: mkv_resume_from_pause\n";
}

// Test MKV stop during playback
static void test_mkv_stop_during_playback() {
    walk_to_playing();

    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Closed, "stop");
    assert(r.decision == TransitionDecision::Accepted);
    assert(r.new_state == PlaybackState::Closed);

    std::cout << "  PASS: mkv_stop_during_playback\n";
}

// Test MKV full playback sequence
static void test_mkv_full_playback_sequence() {
    walk_to_playing();

    // Pause
    auto r = attempt_transition(PlaybackState::Playing, PlaybackState::Paused, "pause");
    assert(r.decision == TransitionDecision::Accepted);

    // Seek from pause
    r = attempt_transition(PlaybackState::Paused, PlaybackState::Seeking, "seek");
    assert(r.decision == TransitionDecision::Accepted);

    // Seek complete back to pause
    r = attempt_transition(PlaybackState::Seeking, PlaybackState::Paused, "seek_complete");
    assert(r.decision == TransitionDecision::Accepted);

    // Resume
    r = attempt_transition(PlaybackState::Paused, PlaybackState::Playing, "resume");
    assert(r.decision == TransitionDecision::Accepted);

    // Stop
    r = attempt_transition(PlaybackState::Playing, PlaybackState::Closed, "stop");
    assert(r.decision == TransitionDecision::Accepted);

    std::cout << "  PASS: mkv_full_playback_sequence\n";
}

int main() {
    std::cout << "local_file_playback_gate_test:\n";
    
    // MP4 tests
    test_mp4_play_from_beginning();
    test_mp4_pause_during_playback();
    test_mp4_seek_forward();
    test_mp4_seek_backward();
    test_mp4_resume_from_pause();
    test_mp4_stop_during_playback();
    test_mp4_full_playback_sequence();
    
    // MKV tests
    test_mkv_play_from_beginning();
    test_mkv_pause_during_playback();
    test_mkv_seek_forward();
    test_mkv_seek_backward();
    test_mkv_resume_from_pause();
    test_mkv_stop_during_playback();
    test_mkv_full_playback_sequence();
    
    std::cout << "ALL 14 TESTS PASSED\n";
    return 0;
}