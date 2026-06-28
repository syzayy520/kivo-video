// render_tests.cpp — P6B runtime: fake render queue / packet pool / remainder slot tests
// Tests B19 (FakeSpscRenderQueue), B20 (FakeRenderPacketPool), B21 (FakeDeviceRemainderSlot).
// Uses try/catch to prevent abort popups (follows P5B test pattern).
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <type_traits>

#include "kivo/video/audio_plane/runtime/render/fake_spsc_render_queue.hpp"
#include "kivo/video/audio_plane/runtime/render/fake_render_packet_pool.hpp"
#include "kivo/video/audio_plane/runtime/render/fake_device_remainder_slot.hpp"
#include "kivo/video/audio_plane/render/audio_render_packet.hpp"
#include "kivo/video/audio_plane/render/audio_spsc_result.hpp"

namespace rt_render = kivo::video::audio_plane::runtime::render;
using kivo::video::audio_plane::render::AudioRenderPacket;
using kivo::video::audio_plane::render::AudioSpscPushResult;
using kivo::video::audio_plane::render::AudioSpscPopResult;
using kivo::video::audio_plane::render::AudioSpscDepthSnapshot;

static int g_failed = 0;

#define P6B_RUN(name) \
    do { \
        try { name(); } catch (...) { \
            ++g_failed; \
            std::printf("  >>> FAIL: %s\n", #name); \
        } \
    } while(0)

// Helper: build a valid packet (pool_slot_handle_id != 0) for queue push.
static AudioRenderPacket make_valid_packet(uint64_t slot_id = 1) {
    AudioRenderPacket p{};
    p.pool_slot_handle_id = slot_id;
    p.sample_count = 1024;
    p.channel_count = 2;
    p.pts = 0;
    p.duration = 1024;
    // pipeline_generation.value defaults to 0; queue with generation 0 treats
    // this as "no generation check configured", so the packet is accepted.
    return p;
}

// ============================================================
// B19: FakeSpscRenderQueue
// ============================================================

// B19.1: try_push on empty queue returns Pushed.
static void test_b19_try_push_pushed() {
    rt_render::FakeSpscRenderQueue<AudioRenderPacket, 4> queue;
    auto packet = make_valid_packet();
    auto result = queue.try_push(std::move(packet));
    if (result != AudioSpscPushResult::Pushed) throw std::runtime_error("first push must be Pushed");
    std::printf("[B19.1] FakeSpscRenderQueue try_push Pushed PASS\n");
}

// B19.2: try_push until Full (capacity = Capacity - 1).
static void test_b19_try_push_until_full() {
    rt_render::FakeSpscRenderQueue<AudioRenderPacket, 4> queue;
    // Capacity usable = 3 (ring buffer wastes one slot).
    for (int i = 0; i < 3; ++i) {
        auto p = make_valid_packet(static_cast<uint64_t>(i + 1));
        if (queue.try_push(std::move(p)) != AudioSpscPushResult::Pushed) throw std::runtime_error("push must succeed until full");
    }
    auto p4 = make_valid_packet(4);
    auto result = queue.try_push(std::move(p4));
    if (result != AudioSpscPushResult::Full) throw std::runtime_error("push when full must return Full");
    std::printf("[B19.2] FakeSpscRenderQueue try_push Full PASS\n");
}

// B19.3: try_pop returns Popped after push.
static void test_b19_try_pop_popped() {
    rt_render::FakeSpscRenderQueue<AudioRenderPacket, 4> queue;
    auto p = make_valid_packet(10);
    if (queue.try_push(std::move(p)) != AudioSpscPushResult::Pushed) throw std::runtime_error("push must succeed");
    AudioRenderPacket out{};
    auto result = queue.try_pop(out);
    if (result != AudioSpscPopResult::Popped) throw std::runtime_error("pop must return Popped");
    if (out.pool_slot_handle_id != 10) throw std::runtime_error("popped packet must match pushed packet");
    std::printf("[B19.3] FakeSpscRenderQueue try_pop Popped PASS\n");
}

// B19.4: try_pop on empty queue returns Empty.
static void test_b19_try_pop_empty() {
    rt_render::FakeSpscRenderQueue<AudioRenderPacket, 4> queue;
    AudioRenderPacket out{};
    auto result = queue.try_pop(out);
    if (result != AudioSpscPopResult::Empty) throw std::runtime_error("pop on empty must return Empty");
    std::printf("[B19.4] FakeSpscRenderQueue try_pop Empty PASS\n");
}

// B19.5: is_lock_free returns FALSE (honest fake).
static void test_b19_is_lock_free_false() {
    rt_render::FakeSpscRenderQueue<AudioRenderPacket, 4> queue;
    if (queue.is_lock_free()) throw std::runtime_error("is_lock_free must return FALSE for fake adapter");
    std::printf("[B19.5] FakeSpscRenderQueue is_lock_free FALSE PASS\n");
}

// B19.6: close() rejects subsequent pushes with Closed.
static void test_b19_close_rejects_push() {
    rt_render::FakeSpscRenderQueue<AudioRenderPacket, 4> queue;
    queue.close();
    if (!queue.is_closed()) throw std::runtime_error("is_closed must be true after close");
    auto p = make_valid_packet(20);
    auto result = queue.try_push(std::move(p));
    if (result != AudioSpscPushResult::Closed) throw std::runtime_error("push after close must return Closed");
    std::printf("[B19.6] FakeSpscRenderQueue close rejects push PASS\n");
}

// B19.7: close() still allows draining remaining packets via try_pop.
static void test_b19_close_drains_remaining() {
    rt_render::FakeSpscRenderQueue<AudioRenderPacket, 4> queue;
    auto p = make_valid_packet(30);
    if (queue.try_push(std::move(p)) != AudioSpscPushResult::Pushed) throw std::runtime_error("push before close must succeed");
    queue.close();
    AudioRenderPacket out{};
    auto result = queue.try_pop(out);
    if (result != AudioSpscPopResult::Popped) throw std::runtime_error("pop after close must drain remaining");
    if (out.pool_slot_handle_id != 30) throw std::runtime_error("drained packet must match");
    // Now empty + closed -> Closed.
    AudioRenderPacket out2{};
    auto result2 = queue.try_pop(out2);
    if (result2 != AudioSpscPopResult::Closed) throw std::runtime_error("pop on empty closed queue must return Closed");
    std::printf("[B19.7] FakeSpscRenderQueue close drains remaining PASS\n");
}

// B19.8: depth_snapshot reports exact (non-approximate) depth for single-threaded fake.
static void test_b19_depth_snapshot_exact() {
    rt_render::FakeSpscRenderQueue<AudioRenderPacket, 4> queue;
    auto snap0 = queue.depth_snapshot();
    if (snap0.approximate_depth != 0) throw std::runtime_error("initial depth must be 0");
    if (snap0.capacity != 3) throw std::runtime_error("capacity must be 3");
    if (snap0.is_approximate) throw std::runtime_error("fake depth must NOT be approximate");
    queue.try_push(make_valid_packet(1));
    queue.try_push(make_valid_packet(2));
    auto snap2 = queue.depth_snapshot();
    if (snap2.approximate_depth != 2) throw std::runtime_error("depth must be 2 after 2 pushes");
    std::printf("[B19.8] FakeSpscRenderQueue depth_snapshot exact PASS\n");
}

// B19.9: invalid packet (pool_slot_handle_id == 0) rejected with InvalidPacket.
static void test_b19_invalid_packet_rejected() {
    rt_render::FakeSpscRenderQueue<AudioRenderPacket, 4> queue;
    AudioRenderPacket p{};  // pool_slot_handle_id == 0 (invalid)
    auto result = queue.try_push(std::move(p));
    if (result != AudioSpscPushResult::InvalidPacket) throw std::runtime_error("invalid packet must be rejected");
    std::printf("[B19.9] FakeSpscRenderQueue invalid packet rejected PASS\n");
}

// B19.10: generation mismatch rejected when generation configured.
static void test_b19_generation_mismatch_rejected() {
    rt_render::FakeSpscRenderQueue<AudioRenderPacket, 4> queue;
    queue.set_generation(7);
    if (queue.generation() != 7) throw std::runtime_error("generation must be 7");
    auto p = make_valid_packet(1);
    p.pipeline_generation.value = 99;  // mismatch
    auto result = queue.try_push(std::move(p));
    if (result != AudioSpscPushResult::GenerationMismatch) throw std::runtime_error("generation mismatch must be rejected");
    std::printf("[B19.10] FakeSpscRenderQueue generation mismatch rejected PASS\n");
}

// B19.11: generation match accepted when generation configured.
static void test_b19_generation_match_accepted() {
    rt_render::FakeSpscRenderQueue<AudioRenderPacket, 4> queue;
    queue.set_generation(7);
    auto p = make_valid_packet(1);
    p.pipeline_generation.value = 7;  // match
    auto result = queue.try_push(std::move(p));
    if (result != AudioSpscPushResult::Pushed) throw std::runtime_error("matching generation must be accepted");
    std::printf("[B19.11] FakeSpscRenderQueue generation match accepted PASS\n");
}

// ============================================================
// B20: FakeRenderPacketPool
// ============================================================

// B20.1: static_assert noexcept on AudioRenderPacket (compile-time check).
// This is a compile-time test; if the file compiles, the static_asserts in
// fake_render_packet_pool.hpp (re-affirmed from P6A) have passed.
static void test_b20_audio_render_packet_noexcept_static_asserts() {
    static_assert(std::is_nothrow_move_constructible_v<AudioRenderPacket>,
                  "AudioRenderPacket must be nothrow move constructible");
    static_assert(std::is_nothrow_destructible_v<AudioRenderPacket>,
                  "AudioRenderPacket must be nothrow destructible");
    std::printf("[B20.1] AudioRenderPacket noexcept static_asserts PASS\n");
}

// B20.2: acquire returns lease_id (non-zero).
static void test_b20_acquire_returns_lease_id() {
    rt_render::FakeRenderPacketPool pool(8);
    auto result = pool.acquire();
    if (result.is_error()) throw std::runtime_error("acquire must succeed on empty pool");
    if (result.value() == 0) throw std::runtime_error("lease_id must be non-zero");
    std::printf("[B20.2] FakeRenderPacketPool acquire returns lease_id PASS\n");
}

// B20.3: acquire returns sequential lease_ids (1, 2, 3, ...).
static void test_b20_acquire_sequential_lease_ids() {
    rt_render::FakeRenderPacketPool pool(8);
    auto r1 = pool.acquire();
    auto r2 = pool.acquire();
    auto r3 = pool.acquire();
    if (r1.is_error() || r2.is_error() || r3.is_error()) throw std::runtime_error("all acquires must succeed");
    if (r1.value() != 1) throw std::runtime_error("first lease_id must be 1");
    if (r2.value() != 2) throw std::runtime_error("second lease_id must be 2");
    if (r3.value() != 3) throw std::runtime_error("third lease_id must be 3");
    std::printf("[B20.3] FakeRenderPacketPool acquire sequential lease_ids PASS\n");
}

// B20.4: acquire until full returns PoolFull.
static void test_b20_acquire_until_full() {
    rt_render::FakeRenderPacketPool pool(4);
    for (size_t i = 0; i < 4; ++i) {
        if (pool.acquire().is_error()) throw std::runtime_error("acquire must succeed until full");
    }
    auto result = pool.acquire();
    if (!result.is_error()) throw std::runtime_error("acquire on full pool must fail");
    if (result.error() != rt_render::PoolFull::NoFreeSlots) throw std::runtime_error("expected NoFreeSlots");
    std::printf("[B20.4] FakeRenderPacketPool acquire until full PASS\n");
}

// B20.5: release frees a slot; subsequent acquire reuses it.
static void test_b20_release_frees_slot() {
    rt_render::FakeRenderPacketPool pool(4);
    auto r1 = pool.acquire();
    if (r1.is_error()) throw std::runtime_error("acquire must succeed");
    if (!pool.release(r1.value())) throw std::runtime_error("release must succeed for valid lease");
    if (pool.available_count() != 4) throw std::runtime_error("available_count must be 4 after release");
    // Re-acquire should reuse slot 1 (lease_id = 1).
    auto r2 = pool.acquire();
    if (r2.is_error()) throw std::runtime_error("re-acquire must succeed after release");
    if (r2.value() != 1) throw std::runtime_error("re-acquired lease_id must be 1");
    std::printf("[B20.5] FakeRenderPacketPool release frees slot PASS\n");
}

// B20.6: release invalid lease_id (0) returns false.
static void test_b20_release_invalid_lease_zero() {
    rt_render::FakeRenderPacketPool pool(4);
    if (pool.release(0)) throw std::runtime_error("release(0) must fail");
    std::printf("[B20.6] FakeRenderPacketPool release invalid lease 0 PASS\n");
}

// B20.7: release out-of-range lease_id returns false.
static void test_b20_release_out_of_range() {
    rt_render::FakeRenderPacketPool pool(4);
    if (pool.release(100)) throw std::runtime_error("release(100) must fail for pool of size 4");
    std::printf("[B20.7] FakeRenderPacketPool release out-of-range PASS\n");
}

// B20.8: double-release returns false.
static void test_b20_double_release_fails() {
    rt_render::FakeRenderPacketPool pool(4);
    auto r = pool.acquire();
    if (r.is_error()) throw std::runtime_error("acquire must succeed");
    if (!pool.release(r.value())) throw std::runtime_error("first release must succeed");
    if (pool.release(r.value())) throw std::runtime_error("second release must fail (double-release)");
    std::printf("[B20.8] FakeRenderPacketPool double-release fails PASS\n");
}

// B20.9: available_count tracks free slots correctly.
static void test_b20_available_count_tracks() {
    rt_render::FakeRenderPacketPool pool(8);
    if (pool.available_count() != 8) throw std::runtime_error("initial available_count must be 8");
    pool.acquire();
    pool.acquire();
    pool.acquire();
    if (pool.available_count() != 5) throw std::runtime_error("available_count must be 5 after 3 acquires");
    std::printf("[B20.9] FakeRenderPacketPool available_count tracks PASS\n");
}

// B20.10: access returns pointer to packet for valid held lease.
static void test_b20_access_valid_lease() {
    rt_render::FakeRenderPacketPool pool(4);
    auto r = pool.acquire();
    if (r.is_error()) throw std::runtime_error("acquire must succeed");
    AudioRenderPacket* pkt = pool.access(r.value());
    if (pkt == nullptr) throw std::runtime_error("access must return non-null for held lease");
    pkt->sample_count = 2048;
    pkt->channel_count = 6;
    if (pkt->sample_count != 2048) throw std::runtime_error("sample_count write must persist");
    std::printf("[B20.10] FakeRenderPacketPool access valid lease PASS\n");
}

// B20.11: access returns nullptr for invalid or free lease.
static void test_b20_access_invalid_lease() {
    rt_render::FakeRenderPacketPool pool(4);
    if (pool.access(0) != nullptr) throw std::runtime_error("access(0) must return nullptr");
    if (pool.access(100) != nullptr) throw std::runtime_error("access(out-of-range) must return nullptr");
    auto r = pool.acquire();
    pool.release(r.value());
    if (pool.access(r.value()) != nullptr) throw std::runtime_error("access on released lease must return nullptr");
    std::printf("[B20.11] FakeRenderPacketPool access invalid lease PASS\n");
}

// B20.12: acquired_count / released_count counters track lifetime.
static void test_b20_counters_track() {
    rt_render::FakeRenderPacketPool pool(4);
    if (pool.acquired_count() != 0) throw std::runtime_error("initial acquired_count must be 0");
    if (pool.released_count() != 0) throw std::runtime_error("initial released_count must be 0");
    auto r1 = pool.acquire();
    auto r2 = pool.acquire();
    if (pool.acquired_count() != 2) throw std::runtime_error("acquired_count must be 2");
    pool.release(r1.value());
    if (pool.released_count() != 1) throw std::runtime_error("released_count must be 1");
    std::printf("[B20.12] FakeRenderPacketPool counters track PASS\n");
}

// B20.13: capacity() reports pool size.
static void test_b20_capacity_reports() {
    rt_render::FakeRenderPacketPool pool(16);
    if (pool.capacity() != 16) throw std::runtime_error("capacity must be 16");
    std::printf("[B20.13] FakeRenderPacketPool capacity reports PASS\n");
}

// ============================================================
// B21: FakeDeviceRemainderSlot
// ============================================================

// B21.1: set + consume round-trip.
static void test_b21_set_consume_roundtrip() {
    rt_render::FakeDeviceRemainderSlot slot;
    AudioRenderPacket p = make_valid_packet(42);
    auto set_result = slot.set(std::move(p));
    if (set_result.is_error()) throw std::runtime_error("set must succeed on empty slot");
    if (!slot.has_value()) throw std::runtime_error("has_value must be true after set");
    auto consume_result = slot.consume();
    if (consume_result.is_error()) throw std::runtime_error("consume must succeed when has_value");
    if (consume_result.value().pool_slot_handle_id != 42) throw std::runtime_error("consumed packet must match");
    if (slot.has_value()) throw std::runtime_error("has_value must be false after consume");
    std::printf("[B21.1] FakeDeviceRemainderSlot set + consume PASS\n");
}

// B21.2: set when has_value returns OverwriteViolation.
static void test_b21_set_overwrite_violation() {
    rt_render::FakeDeviceRemainderSlot slot;
    AudioRenderPacket p1 = make_valid_packet(1);
    if (slot.set(std::move(p1)).is_error()) throw std::runtime_error("first set must succeed");
    AudioRenderPacket p2 = make_valid_packet(2);
    auto result = slot.set(std::move(p2));
    if (!result.is_error()) throw std::runtime_error("second set must fail (OverwriteViolation)");
    if (result.error() != rt_render::OverwriteViolation::SlotNotEmpty) throw std::runtime_error("expected SlotNotEmpty");
    std::printf("[B21.2] FakeDeviceRemainderSlot set OverwriteViolation PASS\n");
}

// B21.3: overwrite_count increments on each violation.
static void test_b21_overwrite_count_increments() {
    rt_render::FakeDeviceRemainderSlot slot;
    if (slot.overwrite_count() != 0) throw std::runtime_error("initial overwrite_count must be 0");
    AudioRenderPacket p1 = make_valid_packet(1);
    slot.set(std::move(p1));
    // Attempt 3 overwrites.
    for (int i = 0; i < 3; ++i) {
        AudioRenderPacket p = make_valid_packet(static_cast<uint64_t>(i + 10));
        slot.set(std::move(p));
    }
    if (slot.overwrite_count() != 3) throw std::runtime_error("overwrite_count must be 3");
    std::printf("[B21.3] FakeDeviceRemainderSlot overwrite_count increments PASS\n");
}

// B21.4: consume when empty returns SlotEmpty.
static void test_b21_consume_empty() {
    rt_render::FakeDeviceRemainderSlot slot;
    if (slot.has_value()) throw std::runtime_error("initial slot must be empty");
    auto result = slot.consume();
    if (!result.is_error()) throw std::runtime_error("consume on empty must fail");
    if (result.error() != rt_render::SlotEmpty::NoValue) throw std::runtime_error("expected NoValue");
    std::printf("[B21.4] FakeDeviceRemainderSlot consume empty SlotEmpty PASS\n");
}

// B21.5: clear() resets the slot to empty.
static void test_b21_clear_resets() {
    rt_render::FakeDeviceRemainderSlot slot;
    AudioRenderPacket p = make_valid_packet(1);
    slot.set(std::move(p));
    if (!slot.has_value()) throw std::runtime_error("slot must have value after set");
    slot.clear();
    if (slot.has_value()) throw std::runtime_error("slot must be empty after clear");
    // consume after clear must fail.
    if (!slot.consume().is_error()) throw std::runtime_error("consume after clear must fail");
    std::printf("[B21.5] FakeDeviceRemainderSlot clear resets PASS\n");
}

// B21.6: set after consume succeeds (slot is reusable).
static void test_b21_set_after_consume() {
    rt_render::FakeDeviceRemainderSlot slot;
    AudioRenderPacket p1 = make_valid_packet(1);
    slot.set(std::move(p1));
    slot.consume();
    // Slot is now empty; set must succeed again.
    AudioRenderPacket p2 = make_valid_packet(2);
    auto result = slot.set(std::move(p2));
    if (result.is_error()) throw std::runtime_error("set after consume must succeed");
    if (!slot.has_value()) throw std::runtime_error("has_value must be true after re-set");
    std::printf("[B21.6] FakeDeviceRemainderSlot set after consume PASS\n");
}

// B21.7: set after clear succeeds (clear makes slot reusable).
static void test_b21_set_after_clear() {
    rt_render::FakeDeviceRemainderSlot slot;
    AudioRenderPacket p1 = make_valid_packet(1);
    slot.set(std::move(p1));
    slot.clear();
    AudioRenderPacket p2 = make_valid_packet(2);
    auto result = slot.set(std::move(p2));
    if (result.is_error()) throw std::runtime_error("set after clear must succeed");
    std::printf("[B21.7] FakeDeviceRemainderSlot set after clear PASS\n");
}

// B21.8: overwrite_count does NOT increment on successful set (only on violations).
static void test_b21_overwrite_count_only_on_violation() {
    rt_render::FakeDeviceRemainderSlot slot;
    AudioRenderPacket p = make_valid_packet(1);
    slot.set(std::move(p));
    if (slot.overwrite_count() != 0) throw std::runtime_error("overwrite_count must remain 0 on successful set");
    slot.consume();
    // Re-set after consume: still no violation.
    AudioRenderPacket p2 = make_valid_packet(2);
    slot.set(std::move(p2));
    if (slot.overwrite_count() != 0) throw std::runtime_error("overwrite_count must remain 0 after legitimate re-set");
    std::printf("[B21.8] FakeDeviceRemainderSlot overwrite_count only on violation PASS\n");
}

int main() {
    std::printf("=== P6B Render Runtime Tests (B19-B21) ===\n\n");

    P6B_RUN(test_b19_try_push_pushed);
    P6B_RUN(test_b19_try_push_until_full);
    P6B_RUN(test_b19_try_pop_popped);
    P6B_RUN(test_b19_try_pop_empty);
    P6B_RUN(test_b19_is_lock_free_false);
    P6B_RUN(test_b19_close_rejects_push);
    P6B_RUN(test_b19_close_drains_remaining);
    P6B_RUN(test_b19_depth_snapshot_exact);
    P6B_RUN(test_b19_invalid_packet_rejected);
    P6B_RUN(test_b19_generation_mismatch_rejected);
    P6B_RUN(test_b19_generation_match_accepted);

    P6B_RUN(test_b20_audio_render_packet_noexcept_static_asserts);
    P6B_RUN(test_b20_acquire_returns_lease_id);
    P6B_RUN(test_b20_acquire_sequential_lease_ids);
    P6B_RUN(test_b20_acquire_until_full);
    P6B_RUN(test_b20_release_frees_slot);
    P6B_RUN(test_b20_release_invalid_lease_zero);
    P6B_RUN(test_b20_release_out_of_range);
    P6B_RUN(test_b20_double_release_fails);
    P6B_RUN(test_b20_available_count_tracks);
    P6B_RUN(test_b20_access_valid_lease);
    P6B_RUN(test_b20_access_invalid_lease);
    P6B_RUN(test_b20_counters_track);
    P6B_RUN(test_b20_capacity_reports);

    P6B_RUN(test_b21_set_consume_roundtrip);
    P6B_RUN(test_b21_set_overwrite_violation);
    P6B_RUN(test_b21_overwrite_count_increments);
    P6B_RUN(test_b21_consume_empty);
    P6B_RUN(test_b21_clear_resets);
    P6B_RUN(test_b21_set_after_consume);
    P6B_RUN(test_b21_set_after_clear);
    P6B_RUN(test_b21_overwrite_count_only_on_violation);

    if (g_failed == 0) {
        std::printf("\n=== P6B Render Summary: all 32 exit criteria PASS ===\n");
        return 0;
    } else {
        std::printf("\n=== P6B Render Summary: %d test(s) FAILED ===\n", g_failed);
        return 1;
    }
}
