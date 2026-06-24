#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/playback_core/playback_session_state_machine.hpp"

using namespace kivo::cinema_engine;

static void test_startup_sequence() {
    auto result = attempt_transition(PlaybackState::Idle, PlaybackState::Opening, "open");
    assert(result.decision == TransitionDecision::Accepted);
    assert(result.new_state == PlaybackState::Opening);

    result = attempt_transition(PlaybackState::Opening, PlaybackState::CreatingSession, "session_created");
    assert(result.decision == TransitionDecision::Accepted);

    result = attempt_transition(PlaybackState::CreatingSession, PlaybackState::ResolvingSource, "source_resolved");
    assert(result.decision == TransitionDecision::Accepted);

    std::cout << "  PASS: startup_sequence\n";
}

static void test_ready_to_playing() {
    auto result = attempt_transition(PlaybackState::Ready, PlaybackState::Playing, "play");
    assert(result.decision == TransitionDecision::Accepted);
    assert(result.new_state == PlaybackState::Playing);
    std::cout << "  PASS: ready_to_playing\n";
}

static void test_play_pause_cycle() {
    auto result = attempt_transition(PlaybackState::Playing, PlaybackState::Paused, "pause");
    assert(result.decision == TransitionDecision::Accepted);
    assert(result.new_state == PlaybackState::Paused);

    result = attempt_transition(PlaybackState::Paused, PlaybackState::Playing, "play");
    assert(result.decision == TransitionDecision::Accepted);
    assert(result.new_state == PlaybackState::Playing);
    std::cout << "  PASS: play_pause_cycle\n";
}

static void test_seek_from_playing() {
    auto result = attempt_transition(PlaybackState::Playing, PlaybackState::Seeking, "seek");
    assert(result.decision == TransitionDecision::Accepted);

    result = attempt_transition(PlaybackState::Seeking, PlaybackState::Playing, "seek_complete");
    assert(result.decision == TransitionDecision::Accepted);
    std::cout << "  PASS: seek_from_playing\n";
}

static void test_seek_from_paused() {
    auto result = attempt_transition(PlaybackState::Paused, PlaybackState::Seeking, "seek");
    assert(result.decision == TransitionDecision::Accepted);

    result = attempt_transition(PlaybackState::Seeking, PlaybackState::Paused, "seek_complete");
    assert(result.decision == TransitionDecision::Accepted);
    std::cout << "  PASS: seek_from_paused\n";
}

static void test_buffering_cycle() {
    auto result = attempt_transition(PlaybackState::Playing, PlaybackState::Buffering, "buffer");
    assert(result.decision == TransitionDecision::Accepted);

    result = attempt_transition(PlaybackState::Buffering, PlaybackState::Playing, "buffer_ready");
    assert(result.decision == TransitionDecision::Accepted);
    std::cout << "  PASS: buffering_cycle\n";
}

static void test_recovery() {
    auto result = attempt_transition(PlaybackState::Playing, PlaybackState::Recovering, "recover");
    assert(result.decision == TransitionDecision::Accepted);

    result = attempt_transition(PlaybackState::Recovering, PlaybackState::Playing, "recovery_complete");
    assert(result.decision == TransitionDecision::Accepted);
    std::cout << "  PASS: recovery\n";
}

static void test_drain_to_ended() {
    auto result = attempt_transition(PlaybackState::Playing, PlaybackState::Draining, "drain");
    assert(result.decision == TransitionDecision::Accepted);

    result = attempt_transition(PlaybackState::Draining, PlaybackState::Ended, "drained");
    assert(result.decision == TransitionDecision::Accepted);
    std::cout << "  PASS: drain_to_ended\n";
}

static void test_close_from_any_state() {
    auto result = attempt_transition(PlaybackState::Playing, PlaybackState::Closed, "close");
    assert(result.decision == TransitionDecision::Accepted);

    result = attempt_transition(PlaybackState::Paused, PlaybackState::Closed, "close");
    assert(result.decision == TransitionDecision::Accepted);

    result = attempt_transition(PlaybackState::Ready, PlaybackState::Closed, "close");
    assert(result.decision == TransitionDecision::Accepted);

    result = attempt_transition(PlaybackState::Idle, PlaybackState::Closed, "close");
    assert(result.decision == TransitionDecision::Accepted);
    std::cout << "  PASS: close_from_any_state\n";
}

static void test_closed_is_terminal() {
    auto result = attempt_transition(PlaybackState::Closed, PlaybackState::Idle, "reopen");
    assert(result.decision == TransitionDecision::RejectedTerminalState);
    assert(result.previous_state == PlaybackState::Closed);
    std::cout << "  PASS: closed_is_terminal\n";
}

static void test_failed_only_to_closed() {
    auto result = attempt_transition(PlaybackState::Failed, PlaybackState::Playing, "play");
    assert(result.decision == TransitionDecision::RejectedTerminalState);

    result = attempt_transition(PlaybackState::Failed, PlaybackState::Closed, "close");
    assert(result.decision == TransitionDecision::Accepted);
    std::cout << "  PASS: failed_only_to_closed\n";
}

static void test_invalid_transition() {
    auto result = attempt_transition(PlaybackState::Idle, PlaybackState::Playing, "play");
    assert(result.decision == TransitionDecision::RejectedInvalidTransition);
    assert(result.rejection_reason.has_value());
    std::cout << "  PASS: invalid_transition\n";
}

static void test_valid_targets_from_playing() {
    auto targets = valid_targets_from(PlaybackState::Playing);
    // Playing can go to: Paused, Seeking, Buffering, SwitchingSource,
    // SwitchingTrack, ReevaluatingPlan, Recovering, Draining, Failed, Closed
    assert(targets.size() >= 9);
    std::cout << "  PASS: valid_targets_from_playing\n";
}

static void test_state_names() {
    assert(std::string(playback_state_name(PlaybackState::Idle)) == "Idle");
    assert(std::string(playback_state_name(PlaybackState::Playing)) == "Playing");
    assert(std::string(playback_state_name(PlaybackState::Closed)) == "Closed");
    std::cout << "  PASS: state_names\n";
}

int main() {
    std::cout << "playback_session_state_machine_test:\n";
    test_startup_sequence();
    test_ready_to_playing();
    test_play_pause_cycle();
    test_seek_from_playing();
    test_seek_from_paused();
    test_buffering_cycle();
    test_recovery();
    test_drain_to_ended();
    test_close_from_any_state();
    test_closed_is_terminal();
    test_failed_only_to_closed();
    test_invalid_transition();
    test_valid_targets_from_playing();
    test_state_names();
    std::cout << "ALL 14 TESTS PASSED\n";
    return 0;
}
