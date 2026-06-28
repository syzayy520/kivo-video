// pool_tests.cpp — P6B pool runtime tests (B8-B10)
// Tests FakePacketPool, FakeFramePool, and PoolPressurePolicy.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <type_traits>

#include "kivo/video/audio_plane/runtime/pool/fake_packet_pool.hpp"
#include "kivo/video/audio_plane/runtime/pool/fake_frame_pool.hpp"
#include "kivo/video/audio_plane/runtime/pool/pool_pressure_policy.hpp"

namespace rt = kivo::video::audio_plane::runtime::pool;

static int g_failed = 0;

#define P6B_RUN(name) do { try { name(); printf(" PASS: %s\n", #name); } catch(...) { ++g_failed; printf(" FAIL: %s\n", #name); } } while(0)

// ---------- B8: FakePacketPool ----------

static void test_b78_packet_pool_acquire_returns_slot() {
    rt::FakePacketPool pool(8);
    auto r = pool.acquire();
    assert(r.has_value());
    assert(r.value() >= 1);  // 1-based slot id
    assert(pool.in_use() == 1);
}

static void test_b79_packet_pool_acquire_multiple_slots() {
    rt::FakePacketPool pool(8);
    for (uint32_t i = 0; i < 4; ++i) {
        auto r = pool.acquire();
        assert(r.has_value());
    }
    assert(pool.in_use() == 4);
}

static void test_b80_packet_pool_acquire_until_full_returns_pool_full() {
    rt::FakePacketPool pool(4);
    for (uint32_t i = 0; i < 4; ++i) {
        auto r = pool.acquire();
        assert(r.has_value());
    }
    auto r = pool.acquire();
    assert(r.is_error());
    assert(pool.in_use() == 4);
}

static void test_b81_packet_pool_release_frees_slot() {
    rt::FakePacketPool pool(4);
    auto r1 = pool.acquire();
    auto r2 = pool.acquire();
    assert(pool.in_use() == 2);
    assert(pool.release(r1.value()));
    assert(pool.in_use() == 1);
    assert(pool.release(r2.value()));
    assert(pool.in_use() == 0);
}

static void test_b82_packet_pool_release_invalid_slot_returns_false() {
    rt::FakePacketPool pool(4);
    assert(!pool.release(0));       // zero is invalid
    assert(!pool.release(999));     // out of range
}

static void test_b83_packet_pool_release_already_free_returns_false() {
    rt::FakePacketPool pool(4);
    auto r1 = pool.acquire();
    assert(pool.release(r1.value()));
    // Releasing an already-free slot must return false.
    assert(!pool.release(r1.value()));
}

static void test_b84_packet_pool_reacquire_after_release() {
    rt::FakePacketPool pool(2);
    auto r1 = pool.acquire();
    auto r2 = pool.acquire();
    assert(pool.release(r1.value()));
    auto r3 = pool.acquire();
    assert(r3.has_value());
    assert(pool.in_use() == 2);
}

static void test_b85_packet_pool_capacity() {
    rt::FakePacketPool pool(16);
    assert(pool.capacity() == 16);
}

static void test_b86_packet_pool_stats_track_usage_and_allocated() {
    rt::FakePacketPool pool(8);
    auto r1 = pool.acquire();
    auto r2 = pool.acquire();
    const auto stats = pool.stats();
    assert(stats.current_usage == 2);
    assert(stats.total_allocated == 2);
    assert(!stats.cap_exceeded);
}

static void test_b87_packet_pool_max_packets_cap_enforced() {
    rt::FakePacketPool pool(8, /*max_bytes=*/0, /*max_packets=*/2, /*max_duration_ms=*/0);
    auto r1 = pool.acquire();
    auto r2 = pool.acquire();
    assert(r1.has_value());
    assert(r2.has_value());
    auto r3 = pool.acquire();
    assert(r3.is_error());
    assert(pool.stats().cap_exceeded);
}

static void test_b88_packet_pool_max_bytes_cap_enforced() {
    rt::FakePacketPool pool(8, /*max_bytes=*/100, /*max_packets=*/0, /*max_duration_ms=*/0);
    assert(pool.account_bytes(60));
    assert(pool.account_bytes(40));
    assert(!pool.account_bytes(1));  // would exceed 100
    assert(pool.stats().cap_exceeded);
}

static void test_b89_packet_pool_max_duration_cap_enforced() {
    rt::FakePacketPool pool(8, 0, 0, /*max_duration_ms=*/1000);
    assert(pool.account_duration_ms(600));
    assert(pool.account_duration_ms(400));
    assert(!pool.account_duration_ms(1));  // would exceed 1000
    assert(pool.stats().cap_exceeded);
}

static void test_b90_packet_pool_slot_ids_one_based_and_within_range() {
    rt::FakePacketPool pool(4);
    for (uint32_t i = 0; i < 4; ++i) {
        auto r = pool.acquire();
        assert(r.has_value());
        assert(r.value() >= 1);
        assert(r.value() <= 4);
    }
}

// ---------- B9: FakeFramePool ----------

static void test_b91_frame_pool_acquire_returns_lease() {
    rt::FakeFramePool pool(8);
    auto r = pool.acquire();
    assert(r.has_value());
    assert(r.value() >= 1);
    assert(pool.current_usage() == 1);
    assert(pool.total_acquired() == 1);
}

static void test_b92_frame_pool_acquire_multiple_frames() {
    rt::FakeFramePool pool(8);
    for (uint32_t i = 0; i < 4; ++i) {
        auto r = pool.acquire();
        assert(r.has_value());
    }
    assert(pool.current_usage() == 4);
    assert(pool.total_acquired() == 4);
}

static void test_b93_frame_pool_acquire_until_full_returns_pool_full() {
    rt::FakeFramePool pool(4);
    for (uint32_t i = 0; i < 4; ++i) {
        auto r = pool.acquire();
        assert(r.has_value());
    }
    auto r = pool.acquire();
    assert(r.is_error());
}

static void test_b94_frame_pool_release_frees_lease() {
    rt::FakeFramePool pool(4);
    auto r1 = pool.acquire();
    auto r2 = pool.acquire();
    assert(pool.current_usage() == 2);
    assert(pool.release(r1.value()));
    assert(pool.current_usage() == 1);
    assert(pool.release(r2.value()));
    assert(pool.current_usage() == 0);
}

static void test_b95_frame_pool_release_invalid_lease_returns_false() {
    rt::FakeFramePool pool(4);
    assert(!pool.release(0));
    assert(!pool.release(999));
}

static void test_b96_frame_pool_release_already_free_returns_false() {
    rt::FakeFramePool pool(4);
    auto r1 = pool.acquire();
    assert(pool.release(r1.value()));
    assert(!pool.release(r1.value()));
}

static void test_b97_frame_pool_recycle_marks_all_slots_free() {
    rt::FakeFramePool pool(8);
    for (uint32_t i = 0; i < 4; ++i) {
        (void)pool.acquire();
    }
    assert(pool.current_usage() == 4);
    pool.recycle();
    assert(pool.current_usage() == 0);
    // total_acquired is lifetime; recycle does not reset it.
    assert(pool.total_acquired() == 4);
}

static void test_b98_frame_pool_recycle_allows_reacquire() {
    rt::FakeFramePool pool(2);
    (void)pool.acquire();
    (void)pool.acquire();
    auto full = pool.acquire();
    assert(full.is_error());
    pool.recycle();
    auto r = pool.acquire();
    assert(r.has_value());
}

static void test_b99_frame_pool_get_returns_buffer_for_active_lease() {
    rt::FakeFramePool pool(4);
    auto r = pool.acquire();
    auto* buf = pool.get(r.value());
    assert(buf != nullptr);
}

static void test_b100_frame_pool_get_returns_null_for_invalid_lease() {
    rt::FakeFramePool pool(4);
    assert(pool.get(0) == nullptr);
    assert(pool.get(999) == nullptr);
}

static void test_b101_frame_pool_get_returns_null_for_released_lease() {
    rt::FakeFramePool pool(4);
    auto r = pool.acquire();
    assert(pool.release(r.value()));
    assert(pool.get(r.value()) == nullptr);
}

static void test_b102_frame_pool_capacity() {
    rt::FakeFramePool pool(16);
    assert(pool.capacity() == 16);
}

static void test_b103_frame_slot_nothrow_move_constructible_static_assert() {
    // The header already contains static_asserts for FrameSlot. Here we
    // re-assert at the test site to demonstrate the contract is upheld.
    static_assert(std::is_nothrow_move_constructible_v<rt::FrameSlot>,
                  "FrameSlot must be nothrow move constructible");
    static_assert(std::is_nothrow_destructible_v<rt::FrameSlot>,
                  "FrameSlot must be nothrow destructible");
}

static void test_b104_frame_slot_default_in_use_false() {
    rt::FrameSlot slot{};
    assert(!slot.in_use);
}

// ---------- B10: PoolPressurePolicy ----------

static void test_b105_pressure_policy_normal_high_pressure_backpressure() {
    rt::PoolPressurePolicy policy;
    auto action = policy.decide(rt::PlaybackMode::Normal, 0.9);
    assert(action == rt::PressureAction::Backpressure);
}

static void test_b106_pressure_policy_normal_low_pressure_backpressure_signal() {
    rt::PoolPressurePolicy policy;
    auto action = policy.decide(rt::PlaybackMode::Normal, 0.1);
    // Normal mode at low pressure still returns Backpressure (gentle signal).
    assert(action == rt::PressureAction::Backpressure);
}

static void test_b107_pressure_policy_normal_threshold_boundary() {
    rt::PoolPressurePolicy policy;
    // At exactly threshold (0.8) the rule is "pressure > 0.8", so 0.8 itself
    // is NOT above threshold. Both branches still return Backpressure in
    // Normal mode, so we just assert the action class.
    auto at = policy.decide(rt::PlaybackMode::Normal, 0.8);
    auto above = policy.decide(rt::PlaybackMode::Normal, 0.81);
    assert(at == rt::PressureAction::Backpressure);
    assert(above == rt::PressureAction::Backpressure);
}

static void test_b108_pressure_policy_live_high_pressure_drop() {
    rt::PoolPressurePolicy policy;
    auto action = policy.decide(rt::PlaybackMode::Live, 0.95);
    assert(action == rt::PressureAction::Drop);
}

static void test_b109_pressure_policy_live_low_pressure_backpressure() {
    rt::PoolPressurePolicy policy;
    auto action = policy.decide(rt::PlaybackMode::Live, 0.5);
    assert(action == rt::PressureAction::Backpressure);
}

static void test_b110_pressure_policy_live_threshold_just_below_drop() {
    rt::PoolPressurePolicy policy;
    auto action = policy.decide(rt::PlaybackMode::Live, 0.9);
    // Live threshold is "pressure > 0.9"; 0.9 itself is not above -> Backpressure.
    assert(action == rt::PressureAction::Backpressure);
}

static void test_b111_pressure_policy_bitperfect_high_pressure_rebuffer() {
    rt::PoolPressurePolicy policy;
    auto action = policy.decide(rt::PlaybackMode::Bitperfect, 0.9);
    assert(action == rt::PressureAction::Rebuffer);
}

static void test_b112_pressure_policy_bitperfect_never_drops() {
    rt::PoolPressurePolicy policy;
    // Bitperfect mode must NEVER return Drop, even at extreme pressure.
    auto extreme = policy.decide(rt::PlaybackMode::Bitperfect, 1.0);
    assert(extreme != rt::PressureAction::Drop);
    assert(extreme == rt::PressureAction::Rebuffer);
    auto mid = policy.decide(rt::PlaybackMode::Bitperfect, 0.85);
    assert(mid != rt::PressureAction::Drop);
}

static void test_b113_pressure_policy_bitperfect_low_pressure_backpressure() {
    rt::PoolPressurePolicy policy;
    auto action = policy.decide(rt::PlaybackMode::Bitperfect, 0.3);
    assert(action == rt::PressureAction::Backpressure);
}

static void test_b114_pressure_policy_passthrough_extreme_pressure_drop() {
    rt::PoolPressurePolicy policy;
    auto action = policy.decide(rt::PlaybackMode::Passthrough, 0.99);
    assert(action == rt::PressureAction::Drop);
}

static void test_b115_pressure_policy_passthrough_below_threshold_backpressure() {
    rt::PoolPressurePolicy policy;
    auto action = policy.decide(rt::PlaybackMode::Passthrough, 0.9);
    assert(action == rt::PressureAction::Backpressure);
}

static void test_b116_pressure_policy_passthrough_threshold_boundary() {
    rt::PoolPressurePolicy policy;
    // Passthrough threshold is "pressure > 0.95"; 0.95 itself is not above.
    auto at = policy.decide(rt::PlaybackMode::Passthrough, 0.95);
    auto above = policy.decide(rt::PlaybackMode::Passthrough, 0.96);
    assert(at == rt::PressureAction::Backpressure);
    assert(above == rt::PressureAction::Drop);
}

static void test_b117_pressure_policy_threshold_values() {
    // Verify the documented thresholds per mode.
    assert(rt::PoolPressurePolicy::threshold(rt::PlaybackMode::Normal) == 0.8);
    assert(rt::PoolPressurePolicy::threshold(rt::PlaybackMode::Live) == 0.9);
    assert(rt::PoolPressurePolicy::threshold(rt::PlaybackMode::Bitperfect) == 0.8);
    assert(rt::PoolPressurePolicy::threshold(rt::PlaybackMode::Passthrough) == 0.95);
}

static void test_b118_pressure_policy_no_silent_drop_in_bitperfect() {
    // The defining contract of Bitperfect mode: there must never be a silent
    // drop. Rebuffer is the only high-pressure action.
    rt::PoolPressurePolicy policy;
    for (double p = 0.0; p <= 1.0; p += 0.05) {
        auto action = policy.decide(rt::PlaybackMode::Bitperfect, p);
        assert(action != rt::PressureAction::Drop);
    }
}

static void test_b119_pressure_policy_passthrough_drops_only_complete_stale() {
    // Passthrough must NOT drop below its high threshold; only complete stale
    // units (pressure > 0.95) are dropped.
    rt::PoolPressurePolicy policy;
    for (double p = 0.0; p <= 0.95; p += 0.05) {
        auto action = policy.decide(rt::PlaybackMode::Passthrough, p);
        assert(action == rt::PressureAction::Backpressure);
    }
    auto drop = policy.decide(rt::PlaybackMode::Passthrough, 0.96);
    assert(drop == rt::PressureAction::Drop);
}

int main() {
    printf("=== P6B pool_tests ===\n");

    // B8: FakePacketPool
    P6B_RUN(test_b78_packet_pool_acquire_returns_slot);
    P6B_RUN(test_b79_packet_pool_acquire_multiple_slots);
    P6B_RUN(test_b80_packet_pool_acquire_until_full_returns_pool_full);
    P6B_RUN(test_b81_packet_pool_release_frees_slot);
    P6B_RUN(test_b82_packet_pool_release_invalid_slot_returns_false);
    P6B_RUN(test_b83_packet_pool_release_already_free_returns_false);
    P6B_RUN(test_b84_packet_pool_reacquire_after_release);
    P6B_RUN(test_b85_packet_pool_capacity);
    P6B_RUN(test_b86_packet_pool_stats_track_usage_and_allocated);
    P6B_RUN(test_b87_packet_pool_max_packets_cap_enforced);
    P6B_RUN(test_b88_packet_pool_max_bytes_cap_enforced);
    P6B_RUN(test_b89_packet_pool_max_duration_cap_enforced);
    P6B_RUN(test_b90_packet_pool_slot_ids_one_based_and_within_range);

    // B9: FakeFramePool
    P6B_RUN(test_b91_frame_pool_acquire_returns_lease);
    P6B_RUN(test_b92_frame_pool_acquire_multiple_frames);
    P6B_RUN(test_b93_frame_pool_acquire_until_full_returns_pool_full);
    P6B_RUN(test_b94_frame_pool_release_frees_lease);
    P6B_RUN(test_b95_frame_pool_release_invalid_lease_returns_false);
    P6B_RUN(test_b96_frame_pool_release_already_free_returns_false);
    P6B_RUN(test_b97_frame_pool_recycle_marks_all_slots_free);
    P6B_RUN(test_b98_frame_pool_recycle_allows_reacquire);
    P6B_RUN(test_b99_frame_pool_get_returns_buffer_for_active_lease);
    P6B_RUN(test_b100_frame_pool_get_returns_null_for_invalid_lease);
    P6B_RUN(test_b101_frame_pool_get_returns_null_for_released_lease);
    P6B_RUN(test_b102_frame_pool_capacity);
    P6B_RUN(test_b103_frame_slot_nothrow_move_constructible_static_assert);
    P6B_RUN(test_b104_frame_slot_default_in_use_false);

    // B10: PoolPressurePolicy
    P6B_RUN(test_b105_pressure_policy_normal_high_pressure_backpressure);
    P6B_RUN(test_b106_pressure_policy_normal_low_pressure_backpressure_signal);
    P6B_RUN(test_b107_pressure_policy_normal_threshold_boundary);
    P6B_RUN(test_b108_pressure_policy_live_high_pressure_drop);
    P6B_RUN(test_b109_pressure_policy_live_low_pressure_backpressure);
    P6B_RUN(test_b110_pressure_policy_live_threshold_just_below_drop);
    P6B_RUN(test_b111_pressure_policy_bitperfect_high_pressure_rebuffer);
    P6B_RUN(test_b112_pressure_policy_bitperfect_never_drops);
    P6B_RUN(test_b113_pressure_policy_bitperfect_low_pressure_backpressure);
    P6B_RUN(test_b114_pressure_policy_passthrough_extreme_pressure_drop);
    P6B_RUN(test_b115_pressure_policy_passthrough_below_threshold_backpressure);
    P6B_RUN(test_b116_pressure_policy_passthrough_threshold_boundary);
    P6B_RUN(test_b117_pressure_policy_threshold_values);
    P6B_RUN(test_b118_pressure_policy_no_silent_drop_in_bitperfect);
    P6B_RUN(test_b119_pressure_policy_passthrough_drops_only_complete_stale);

    printf("=== P6B pool_tests: %d failed ===\n", g_failed);
    return g_failed == 0 ? 0 : 1;
}
