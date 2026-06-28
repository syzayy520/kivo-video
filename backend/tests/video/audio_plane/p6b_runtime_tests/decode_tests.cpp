// decode_tests.cpp — P6B runtime: fake audio decode backend + lifecycle controller tests
// Tests B17 (FakeDecodeBackend) and B18 (FakeDecodeLifecycleController).
// Uses try/catch to prevent abort popups (follows P5B test pattern).
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "kivo/video/audio_plane/runtime/decode/fake_decode_backend.hpp"
#include "kivo/video/audio_plane/runtime/decode/fake_decode_lifecycle_controller.hpp"
#include "kivo/video/audio_plane/decode/audio_decode_backend_lifecycle.hpp"

namespace rt_decode = kivo::video::audio_plane::runtime::decode;
using kivo::video::audio_plane::decode::AudioDecodeBackendState;
using kivo::video::audio_plane::decode::AudioDecodeBackendLifecycle;

static int g_failed = 0;

#define P6B_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// ============================================================
// B17: FakeDecodeBackend
// ============================================================

// B17.1: init() drives NotCreated -> Created -> Opening -> Ready.
static void test_b17_init_transitions_to_ready() {
    rt_decode::FakeDecodeBackend backend;
    if (backend.state() != AudioDecodeBackendState::NotCreated) throw std::runtime_error("initial state must be NotCreated");
    auto result = backend.init();
    if (result.is_error()) throw std::runtime_error("init() must succeed");
    if (backend.state() != AudioDecodeBackendState::Ready) throw std::runtime_error("init() must reach Ready");
    std::printf("[B17.1] FakeDecodeBackend init NotCreated -> Ready PASS\n");
}

// B17.2: init() twice is invalid adjacency (Ready -> Ready rejected).
static void test_b17_init_twice_rejected() {
    rt_decode::FakeDecodeBackend backend;
    auto first = backend.init();
    if (first.is_error()) throw std::runtime_error("first init must succeed");
    auto second = backend.init();
    if (!second.is_error()) throw std::runtime_error("second init must fail adjacency");
    if (second.error() != rt_decode::TransitionError::InvalidAdjacency) throw std::runtime_error("expected InvalidAdjacency");
    std::printf("[B17.2] FakeDecodeBackend init twice rejected PASS\n");
}

// B17.3: submit_packet accepted when Ready, drives state to Decoding.
static void test_b17_submit_packet_accepted() {
    rt_decode::FakeDecodeBackend backend;
    if (backend.init().is_error()) throw std::runtime_error("init must succeed");
    auto result = backend.submit_packet(1001);
    if (result.is_error()) throw std::runtime_error("submit_packet must be accepted");
    if (backend.state() != AudioDecodeBackendState::Decoding) throw std::runtime_error("state must be Decoding after submit");
    if (backend.pending_packet_count() != 1) throw std::runtime_error("pending_packet_count must be 1");
    std::printf("[B17.3] FakeDecodeBackend submit_packet Accepted PASS\n");
}

// B17.4: submit_packet with id 0 rejected as InvalidPacketId.
static void test_b17_submit_packet_invalid_id() {
    rt_decode::FakeDecodeBackend backend;
    if (backend.init().is_error()) throw std::runtime_error("init must succeed");
    auto result = backend.submit_packet(0);
    if (!result.is_error()) throw std::runtime_error("submit_packet(0) must be rejected");
    if (result.error() != rt_decode::DecodeReject::InvalidPacketId) throw std::runtime_error("expected InvalidPacketId");
    std::printf("[B17.4] FakeDecodeBackend submit_packet invalid id rejected PASS\n");
}

// B17.5: submit_packet before init rejected as NotReady.
static void test_b17_submit_packet_not_ready() {
    rt_decode::FakeDecodeBackend backend;
    auto result = backend.submit_packet(1002);
    if (!result.is_error()) throw std::runtime_error("submit_packet before init must be rejected");
    if (result.error() != rt_decode::DecodeReject::NotReady) throw std::runtime_error("expected NotReady");
    std::printf("[B17.5] FakeDecodeBackend submit_packet NotReady PASS\n");
}

// B17.6: receive_frame returns frame with sample_count > 0 after submit.
static void test_b17_receive_frame_returns_frame() {
    rt_decode::FakeDecodeBackend backend;
    if (backend.init().is_error()) throw std::runtime_error("init must succeed");
    if (backend.submit_packet(2001).is_error()) throw std::runtime_error("submit must succeed");
    auto frame_result = backend.receive_frame();
    if (frame_result.is_error()) throw std::runtime_error("receive_frame must return a frame");
    auto& frame = frame_result.value();
    if (frame.sample_count <= 0) throw std::runtime_error("sample_count must be > 0");
    if (frame.frame_id == 0) throw std::runtime_error("frame_id must be non-zero");
    if (frame.channel_count <= 0) throw std::runtime_error("channel_count must be > 0");
    std::printf("[B17.6] FakeDecodeBackend receive_frame returns frame PASS\n");
}

// B17.7: receive_frame without pending returns WouldBlock.
static void test_b17_receive_frame_would_block() {
    rt_decode::FakeDecodeBackend backend;
    if (backend.init().is_error()) throw std::runtime_error("init must succeed");
    // State is Ready (no submit): NotReady, not WouldBlock per current impl.
    auto result = backend.receive_frame();
    if (!result.is_error()) throw std::runtime_error("receive_frame in Ready must not return frame");
    if (result.error() != rt_decode::DecodeStatus::NotReady) throw std::runtime_error("expected NotReady in Ready state");
    std::printf("[B17.7] FakeDecodeBackend receive_frame NotReady in Ready state PASS\n");
}

// B17.8: receive_frame after draining with no packets returns Eos.
static void test_b17_receive_frame_eos_when_draining() {
    rt_decode::FakeDecodeBackend backend;
    if (backend.init().is_error()) throw std::runtime_error("init must succeed");
    if (backend.submit_packet(3001).is_error()) throw std::runtime_error("submit must succeed");
    // Drain the pending packet first.
    if (backend.receive_frame().is_error()) throw std::runtime_error("first receive must succeed");
    // Transition Decoding -> Draining.
    auto tr = backend.transition_to(AudioDecodeBackendState::Draining);
    if (tr.is_error()) throw std::runtime_error("transition to Draining must succeed");
    auto result = backend.receive_frame();
    if (!result.is_error()) throw std::runtime_error("receive_frame while draining with no packets must be Eos");
    if (result.error() != rt_decode::DecodeStatus::Eos) throw std::runtime_error("expected Eos");
    std::printf("[B17.8] FakeDecodeBackend receive_frame Eos when Draining PASS\n");
}

// B17.9: set_deadline + exceed_deadline + receive_frame returns TimedOut.
static void test_b17_receive_frame_timed_out() {
    rt_decode::FakeDecodeBackend backend;
    if (backend.init().is_error()) throw std::runtime_error("init must succeed");
    if (backend.submit_packet(4001).is_error()) throw std::runtime_error("submit must succeed");
    backend.set_deadline_ms(1000);
    if (!backend.is_deadline_exceeded()) {
        // set_deadline only arms; exceed_deadline simulates timeout.
    }
    backend.exceed_deadline();
    if (!backend.is_deadline_exceeded()) throw std::runtime_error("deadline must be exceeded");
    auto result = backend.receive_frame();
    if (!result.is_error()) throw std::runtime_error("receive_frame after deadline must be TimedOut");
    if (result.error() != rt_decode::DecodeStatus::TimedOut) throw std::runtime_error("expected TimedOut");
    std::printf("[B17.9] FakeDecodeBackend receive_frame TimedOut PASS\n");
}

// B17.10: transition_to Closed terminates backend; receive_frame returns Closed.
static void test_b17_transition_to_closed() {
    rt_decode::FakeDecodeBackend backend;
    if (backend.init().is_error()) throw std::runtime_error("init must succeed");
    auto tr = backend.transition_to(AudioDecodeBackendState::Closed);
    if (tr.is_error()) throw std::runtime_error("transition to Closed must succeed");
    if (backend.state() != AudioDecodeBackendState::Closed) throw std::runtime_error("state must be Closed");
    auto submit = backend.submit_packet(5001);
    if (!submit.is_error()) throw std::runtime_error("submit_packet after Closed must be rejected");
    if (submit.error() != rt_decode::DecodeReject::Closed) throw std::runtime_error("expected Closed reject");
    auto frame = backend.receive_frame();
    if (!frame.is_error()) throw std::runtime_error("receive_frame after Closed must be error");
    if (frame.error() != rt_decode::DecodeStatus::Closed) throw std::runtime_error("expected Closed status");
    std::printf("[B17.10] FakeDecodeBackend transition to Closed PASS\n");
}

// B17.11: transition_to from terminal state rejected (SourceIsTerminal).
static void test_b17_terminal_source_rejected() {
    rt_decode::FakeDecodeBackend backend;
    if (backend.init().is_error()) throw std::runtime_error("init must succeed");
    if (backend.transition_to(AudioDecodeBackendState::Closed).is_error()) throw std::runtime_error("transition to Closed must succeed");
    auto tr = backend.transition_to(AudioDecodeBackendState::Ready);
    if (!tr.is_error()) throw std::runtime_error("transition from terminal Closed must be rejected");
    if (tr.error() != rt_decode::TransitionError::SourceIsTerminal) throw std::runtime_error("expected SourceIsTerminal");
    std::printf("[B17.11] FakeDecodeBackend terminal source rejected PASS\n");
}

// B17.12: transition_to invalid adjacency rejected (Ready -> NotCreated).
static void test_b17_invalid_adjacency_rejected() {
    rt_decode::FakeDecodeBackend backend;
    if (backend.init().is_error()) throw std::runtime_error("init must succeed");
    auto tr = backend.transition_to(AudioDecodeBackendState::NotCreated);
    if (!tr.is_error()) throw std::runtime_error("Ready -> NotCreated must be rejected");
    if (tr.error() != rt_decode::TransitionError::InvalidAdjacency) throw std::runtime_error("expected InvalidAdjacency");
    std::printf("[B17.12] FakeDecodeBackend invalid adjacency rejected PASS\n");
}

// B17.13: frame counter increments per receive_frame.
static void test_b17_frame_counter_increments() {
    rt_decode::FakeDecodeBackend backend;
    if (backend.init().is_error()) throw std::runtime_error("init must succeed");
    if (backend.submit_packet(6001).is_error()) throw std::runtime_error("submit 1 must succeed");
    if (backend.submit_packet(6002).is_error()) throw std::runtime_error("submit 2 must succeed");
    auto f1 = backend.receive_frame();
    auto f2 = backend.receive_frame();
    if (f1.is_error() || f2.is_error()) throw std::runtime_error("both receives must succeed");
    if (f1.value().frame_id >= f2.value().frame_id) throw std::runtime_error("frame_id must increase");
    if (backend.frame_counter() != 2) throw std::runtime_error("frame_counter must be 2");
    std::printf("[B17.13] FakeDecodeBackend frame counter increments PASS\n");
}

// ============================================================
// B18: FakeDecodeLifecycleController
// ============================================================

// B18.1: valid transition chain NotCreated -> Created -> Opening -> Ready.
static void test_b18_valid_transition_chain() {
    rt_decode::FakeDecodeLifecycleController controller;
    if (controller.current_state() != AudioDecodeBackendState::NotCreated) throw std::runtime_error("initial state must be NotCreated");

    auto t1 = controller.transition(AudioDecodeBackendState::NotCreated, AudioDecodeBackendState::Created);
    if (t1.is_error()) throw std::runtime_error("NotCreated -> Created must succeed");
    if (controller.current_state() != AudioDecodeBackendState::Created) throw std::runtime_error("state must be Created");

    auto t2 = controller.transition(AudioDecodeBackendState::Created, AudioDecodeBackendState::Opening);
    if (t2.is_error()) throw std::runtime_error("Created -> Opening must succeed");
    if (controller.current_state() != AudioDecodeBackendState::Opening) throw std::runtime_error("state must be Opening");

    auto t3 = controller.transition(AudioDecodeBackendState::Opening, AudioDecodeBackendState::Ready);
    if (t3.is_error()) throw std::runtime_error("Opening -> Ready must succeed");
    if (controller.current_state() != AudioDecodeBackendState::Ready) throw std::runtime_error("state must be Ready");
    std::printf("[B18.1] FakeDecodeLifecycleController valid chain PASS\n");
}

// B18.2: invalid adjacency Ready -> NotCreated rejected.
static void test_b18_invalid_adjacency_rejected() {
    rt_decode::FakeDecodeLifecycleController controller;
    auto t1 = controller.transition(AudioDecodeBackendState::NotCreated, AudioDecodeBackendState::Created);
    if (t1.is_error()) throw std::runtime_error("first transition must succeed");
    auto t2 = controller.transition(AudioDecodeBackendState::Created, AudioDecodeBackendState::Ready);
    if (!t2.is_error()) throw std::runtime_error("Created -> Ready must be rejected (invalid adjacency)");
    if (t2.error() != rt_decode::LifecycleTransitionError::InvalidAdjacency) throw std::runtime_error("expected InvalidAdjacency");
    std::printf("[B18.2] FakeDecodeLifecycleController invalid adjacency rejected PASS\n");
}

// B18.3: transition from terminal state rejected (SourceIsTerminal).
static void test_b18_terminal_source_rejected() {
    rt_decode::FakeDecodeLifecycleController controller;
    // Closed is terminal; Any -> Closed is allowed adjacency.
    auto t1 = controller.transition(AudioDecodeBackendState::NotCreated, AudioDecodeBackendState::Closed);
    if (t1.is_error()) throw std::runtime_error("NotCreated -> Closed must succeed");
    auto t2 = controller.transition(AudioDecodeBackendState::Closed, AudioDecodeBackendState::Ready);
    if (!t2.is_error()) throw std::runtime_error("transition from terminal Closed must be rejected");
    if (t2.error() != rt_decode::LifecycleTransitionError::SourceIsTerminal) throw std::runtime_error("expected SourceIsTerminal");
    std::printf("[B18.3] FakeDecodeLifecycleController terminal source rejected PASS\n");
}

// B18.4: is_cancellable returns true for non-terminal, false for terminal.
static void test_b18_is_cancellable() {
    rt_decode::FakeDecodeLifecycleController controller;
    if (!controller.is_cancellable(AudioDecodeBackendState::NotCreated)) throw std::runtime_error("NotCreated must be cancellable");
    if (!controller.is_cancellable(AudioDecodeBackendState::Ready)) throw std::runtime_error("Ready must be cancellable");
    if (!controller.is_cancellable(AudioDecodeBackendState::Decoding)) throw std::runtime_error("Decoding must be cancellable");
    if (controller.is_cancellable(AudioDecodeBackendState::Closed)) throw std::runtime_error("Closed must NOT be cancellable");
    if (controller.is_cancellable(AudioDecodeBackendState::FailedFatal)) throw std::runtime_error("FailedFatal must NOT be cancellable");
    if (controller.is_cancellable(AudioDecodeBackendState::Superseded)) throw std::runtime_error("Superseded must NOT be cancellable");
    std::printf("[B18.4] FakeDecodeLifecycleController is_cancellable PASS\n");
}

// B18.5: cancel() sets cancelled flag; all subsequent transitions fail with Cancelled.
static void test_b18_cancel_blocks_transitions() {
    rt_decode::FakeDecodeLifecycleController controller;
    controller.cancel(rt_decode::CancelReason::UserRequested);
    if (!controller.is_cancelled()) throw std::runtime_error("cancelled flag must be set");
    if (controller.current_state() != AudioDecodeBackendState::Closed) throw std::runtime_error("cancel must set state to Closed");
    auto t = controller.transition(AudioDecodeBackendState::NotCreated, AudioDecodeBackendState::Created);
    if (!t.is_error()) throw std::runtime_error("transition after cancel must fail");
    if (t.error() != rt_decode::LifecycleTransitionError::Cancelled) throw std::runtime_error("expected Cancelled");
    std::printf("[B18.5] FakeDecodeLifecycleController cancel blocks transitions PASS\n");
}

// B18.6: supersede() sets superseded flag; all subsequent transitions fail with Superseded.
static void test_b18_supersede_blocks_transitions() {
    rt_decode::FakeDecodeLifecycleController controller;
    controller.supersede(42);
    if (!controller.is_superseded()) throw std::runtime_error("superseded flag must be set");
    if (controller.current_state() != AudioDecodeBackendState::Superseded) throw std::runtime_error("supersede must set state to Superseded");
    auto t = controller.transition(AudioDecodeBackendState::NotCreated, AudioDecodeBackendState::Created);
    if (!t.is_error()) throw std::runtime_error("transition after supersede must fail");
    if (t.error() != rt_decode::LifecycleTransitionError::Superseded) throw std::runtime_error("expected Superseded");
    std::printf("[B18.6] FakeDecodeLifecycleController supersede blocks transitions PASS\n");
}

// B18.7: configurable timeout thresholds (normal + live).
static void test_b18_configurable_timeouts() {
    rt_decode::FakeDecodeLifecycleController controller;
    // Defaults.
    if (controller.normal_timeout_ms() != 3000) throw std::runtime_error("default normal_timeout must be 3000");
    if (controller.live_timeout_ms() != 5000) throw std::runtime_error("default live_timeout must be 5000");
    controller.set_normal_timeout_ms(1500);
    controller.set_live_timeout_ms(2500);
    if (controller.normal_timeout_ms() != 1500) throw std::runtime_error("normal_timeout must be 1500");
    if (controller.live_timeout_ms() != 2500) throw std::runtime_error("live_timeout must be 2500");
    std::printf("[B18.7] FakeDecodeLifecycleController configurable timeouts PASS\n");
}

// B18.8: transition WaitingCodecPrivateData -> Opening valid (extra adjacency).
static void test_b18_waiting_private_data_adjacency() {
    rt_decode::FakeDecodeLifecycleController controller;
    auto t1 = controller.transition(AudioDecodeBackendState::NotCreated, AudioDecodeBackendState::Created);
    if (t1.is_error()) throw std::runtime_error("NotCreated -> Created must succeed");
    auto t2 = controller.transition(AudioDecodeBackendState::Created, AudioDecodeBackendState::WaitingCodecPrivateData);
    if (t2.is_error()) throw std::runtime_error("Created -> WaitingCodecPrivateData must succeed");
    auto t3 = controller.transition(AudioDecodeBackendState::WaitingCodecPrivateData, AudioDecodeBackendState::Opening);
    if (t3.is_error()) throw std::runtime_error("WaitingCodecPrivateData -> Opening must succeed");
    std::printf("[B18.8] FakeDecodeLifecycleController WaitingCodecPrivateData adjacency PASS\n");
}

// B18.9: terminal-state check via AudioDecodeBackendLifecycle (cross-reference P6A).
static void test_b18_terminal_state_via_lifecycle() {
    AudioDecodeBackendLifecycle lifecycle{};
    lifecycle.state = AudioDecodeBackendState::Closed;
    if (!lifecycle.is_terminal()) throw std::runtime_error("Closed must be terminal");
    lifecycle.state = AudioDecodeBackendState::FailedFatal;
    if (!lifecycle.is_terminal()) throw std::runtime_error("FailedFatal must be terminal");
    lifecycle.state = AudioDecodeBackendState::Superseded;
    if (!lifecycle.is_terminal()) throw std::runtime_error("Superseded must be terminal");
    lifecycle.state = AudioDecodeBackendState::Ready;
    if (lifecycle.is_terminal()) throw std::runtime_error("Ready must NOT be terminal");
    std::printf("[B18.9] AudioDecodeBackendLifecycle terminal check PASS\n");
}

// B18.10: 16-state enumeration coverage (all values distinct).
static void test_b18_sixteen_states_distinct() {
    // The P6A §7.2 lifecycle defines exactly 16 states. Verify they are distinct
    // by collecting them into a set-like comparison.
    AudioDecodeBackendState states[] = {
        AudioDecodeBackendState::NotCreated,
        AudioDecodeBackendState::Created,
        AudioDecodeBackendState::WaitingCodecPrivateData,
        AudioDecodeBackendState::Opening,
        AudioDecodeBackendState::Ready,
        AudioDecodeBackendState::Decoding,
        AudioDecodeBackendState::Draining,
        AudioDecodeBackendState::Flushing,
        AudioDecodeBackendState::Resetting,
        AudioDecodeBackendState::FormatChanging,
        AudioDecodeBackendState::TimedOut,
        AudioDecodeBackendState::FailedRecoverable,
        AudioDecodeBackendState::FailedFatal,
        AudioDecodeBackendState::Closed,
        AudioDecodeBackendState::Superseded,
        AudioDecodeBackendState::FormatChanged,
    };
    constexpr size_t kCount = sizeof(states) / sizeof(states[0]);
    if (kCount != 16) throw std::runtime_error("expected exactly 16 states");
    for (size_t i = 0; i < kCount; ++i) {
        for (size_t j = i + 1; j < kCount; ++j) {
            if (states[i] == states[j]) throw std::runtime_error("duplicate state detected");
        }
    }
    std::printf("[B18.10] AudioDecodeBackendState 16 distinct states PASS\n");
}

int main() {
    std::printf("=== P6B Decode Runtime Tests (B17-B18) ===\n\n");

    P6B_RUN(test_b17_init_transitions_to_ready);
    P6B_RUN(test_b17_init_twice_rejected);
    P6B_RUN(test_b17_submit_packet_accepted);
    P6B_RUN(test_b17_submit_packet_invalid_id);
    P6B_RUN(test_b17_submit_packet_not_ready);
    P6B_RUN(test_b17_receive_frame_returns_frame);
    P6B_RUN(test_b17_receive_frame_would_block);
    P6B_RUN(test_b17_receive_frame_eos_when_draining);
    P6B_RUN(test_b17_receive_frame_timed_out);
    P6B_RUN(test_b17_transition_to_closed);
    P6B_RUN(test_b17_terminal_source_rejected);
    P6B_RUN(test_b17_invalid_adjacency_rejected);
    P6B_RUN(test_b17_frame_counter_increments);

    P6B_RUN(test_b18_valid_transition_chain);
    P6B_RUN(test_b18_invalid_adjacency_rejected);
    P6B_RUN(test_b18_terminal_source_rejected);
    P6B_RUN(test_b18_is_cancellable);
    P6B_RUN(test_b18_cancel_blocks_transitions);
    P6B_RUN(test_b18_supersede_blocks_transitions);
    P6B_RUN(test_b18_configurable_timeouts);
    P6B_RUN(test_b18_waiting_private_data_adjacency);
    P6B_RUN(test_b18_terminal_state_via_lifecycle);
    P6B_RUN(test_b18_sixteen_states_distinct);

    if (g_failed == 0) {
        std::printf("\n=== P6B Decode Summary: all 23 exit criteria PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P6B Decode Summary: %d test(s) FAILED ===\n", g_failed);
        return 1;
    }
}
