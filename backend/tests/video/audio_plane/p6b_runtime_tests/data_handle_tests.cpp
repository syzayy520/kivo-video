// data_handle_tests.cpp — P6B data runtime tests (B5-B7)
// Tests FakeDataHandleManager, OwnedBlockAllocator, and SyncByteViewGuard.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include "kivo/video/audio_plane/runtime/data/fake_data_handle_manager.hpp"
#include "kivo/video/audio_plane/runtime/data/owned_block_allocator.hpp"
#include "kivo/video/audio_plane/runtime/data/sync_byte_view_guard.hpp"

namespace rt = kivo::video::audio_plane::runtime::data;
using namespace kivo::video::audio_plane;

static int g_failed = 0;

#define P6B_RUN(name) do { try { name(); printf(" PASS: %s\n", #name); } catch(...) { ++g_failed; printf(" FAIL: %s\n", #name); } } while(0)

// ---------- B5: FakeDataHandleManager ----------

static void test_b56_handle_manager_acquire_sync_view() {
    rt::FakeDataHandleManager mgr;
    packet::AudioPacketId pid{1};
    auto view = mgr.acquire_sync_view(pid);
    assert(view.handle_id.value != 0);
    assert(view.owner_stage == data::AudioDataOwnerStage::P6);
    assert(view.packet_id.value == 1);
    assert(view.bytes.size() == 64);
    assert(view.bytes[0] == std::byte{0xAB});
    assert(mgr.active_count() == 1);
}

static void test_b57_handle_manager_acquire_owned_block() {
    rt::FakeDataHandleManager mgr;
    packet::AudioPacketId pid{2};
    auto handle = mgr.acquire_owned_block(pid, 128);
    assert(handle.handle_id.value != 0);
    assert(handle.packet_id.value == 2);
    assert(handle.flags.is_noexcept_release);
    assert(!handle.flags.is_heavy_release);
    assert(mgr.active_count() == 1);
}

static void test_b58_handle_manager_acquire_pool_slot() {
    rt::FakeDataHandleManager mgr;
    packet::AudioPacketId pid{3};
    auto handle = mgr.acquire_pool_slot(pid);
    assert(handle.slot_id.value != 0);
    assert(handle.pool_id.value == 1);
    assert(handle.packet_id.value == 3);
    assert(mgr.active_count() == 1);
}

static void test_b59_handle_manager_release_sync_view_released() {
    rt::FakeDataHandleManager mgr;
    packet::AudioPacketId pid{1};
    auto view = mgr.acquire_sync_view(pid);
    auto r = mgr.release(view.handle_id);
    assert(r == rt::ReleaseResult::Released);
    assert(mgr.active_count() == 0);
}

static void test_b60_handle_manager_release_owned_block_released() {
    rt::FakeDataHandleManager mgr;
    packet::AudioPacketId pid{2};
    auto handle = mgr.acquire_owned_block(pid, 128);
    auto r = mgr.release(handle.handle_id);
    assert(r == rt::ReleaseResult::Released);
    assert(mgr.active_count() == 0);
}

static void test_b61_handle_manager_release_pool_slot_released() {
    rt::FakeDataHandleManager mgr;
    packet::AudioPacketId pid{3};
    auto handle = mgr.acquire_pool_slot(pid);
    data::AudioDataHandleId hid{};
    hid.value = handle.slot_id.value;
    auto r = mgr.release(hid);
    assert(r == rt::ReleaseResult::Released);
    assert(mgr.active_count() == 0);
}

static void test_b62_handle_manager_release_already_released() {
    rt::FakeDataHandleManager mgr;
    packet::AudioPacketId pid{1};
    auto view = mgr.acquire_sync_view(pid);
    (void)mgr.release(view.handle_id);
    auto r = mgr.release(view.handle_id);
    assert(r == rt::ReleaseResult::AlreadyReleased);
}

static void test_b63_handle_manager_release_not_found() {
    rt::FakeDataHandleManager mgr;
    data::AudioDataHandleId unknown{};
    unknown.value = 9999;
    auto r = mgr.release(unknown);
    assert(r == rt::ReleaseResult::NotFound);
}

static void test_b64_handle_manager_multiple_active_handles() {
    rt::FakeDataHandleManager mgr;
    packet::AudioPacketId pid{1};
    auto v1 = mgr.acquire_sync_view(pid);
    auto v2 = mgr.acquire_sync_view(pid);
    auto b1 = mgr.acquire_owned_block(pid, 32);
    auto s1 = mgr.acquire_pool_slot(pid);
    assert(mgr.active_count() == 4);
    assert(v1.handle_id.value != v2.handle_id.value);
    assert(v2.handle_id.value != b1.handle_id.value);
    assert(b1.handle_id.value != s1.slot_id.value);
    (void)mgr.release(v1.handle_id);
    assert(mgr.active_count() == 3);
    (void)mgr.release(b1.handle_id);
    assert(mgr.active_count() == 2);
    {
        data::AudioDataHandleId hid{};
        hid.value = s1.slot_id.value;
        (void)mgr.release(hid);
    }
    assert(mgr.active_count() == 1);
    (void)mgr.release(v2.handle_id);
    assert(mgr.active_count() == 0);
}

static void test_b65_handle_manager_distinct_handle_ids() {
    rt::FakeDataHandleManager mgr;
    packet::AudioPacketId pid{1};
    auto v = mgr.acquire_sync_view(pid);
    auto b = mgr.acquire_owned_block(pid, 16);
    auto s = mgr.acquire_pool_slot(pid);
    // All ids should be unique and monotonically increasing.
    assert(v.handle_id.value < b.handle_id.value);
    assert(b.handle_id.value < s.slot_id.value);
}

// ---------- B6: OwnedBlockAllocator ----------

static void test_b66_block_allocator_allocate_returns_valid_handle() {
    rt::OwnedBlockAllocator alloc;
    packet::AudioPacketId pid{1};
    auto handle = alloc.allocate(pid, 64);
    assert(handle.handle_id.value != 0);
    assert(handle.packet_id.value == 1);
    assert(handle.release_fn != nullptr);
    assert(handle.release_ctx != nullptr);
    assert(handle.flags.is_noexcept_release);
    assert(!handle.flags.is_heavy_release);
    assert(alloc.total_allocated() == 1);
    assert(alloc.live_count() == 1);
}

static void test_b67_block_allocator_allocate_multiple() {
    rt::OwnedBlockAllocator alloc;
    packet::AudioPacketId pid{1};
    auto h1 = alloc.allocate(pid, 16);
    auto h2 = alloc.allocate(pid, 32);
    auto h3 = alloc.allocate(pid, 64);
    assert(h1.handle_id.value != h2.handle_id.value);
    assert(h2.handle_id.value != h3.handle_id.value);
    assert(alloc.total_allocated() == 3);
    assert(alloc.live_count() == 3);
}

static void test_b68_block_allocator_mark_released_reduces_live_count() {
    rt::OwnedBlockAllocator alloc;
    packet::AudioPacketId pid{1};
    auto h1 = alloc.allocate(pid, 16);
    auto h2 = alloc.allocate(pid, 32);
    assert(alloc.live_count() == 2);
    alloc.mark_released(h1.handle_id.value);
    assert(alloc.live_count() == 1);
    assert(alloc.total_allocated() == 2);
}

static void test_b69_block_allocator_mark_released_invalid_handle_noop() {
    rt::OwnedBlockAllocator alloc;
    packet::AudioPacketId pid{1};
    (void)alloc.allocate(pid, 16);
    // Invalid handle id (0 or out of range): must not crash.
    alloc.mark_released(0);
    alloc.mark_released(999);
    assert(alloc.live_count() == 1);
}

static void test_b70_block_allocator_release_fn_is_noexcept_callable() {
    rt::OwnedBlockAllocator alloc;
    packet::AudioPacketId pid{1};
    auto handle = alloc.allocate(pid, 16);
    // The release_fn must be noexcept and callable with the stored ctx.
    assert(handle.release_fn != nullptr);
    handle.release_fn(handle.release_ctx);  // must not throw
    assert(alloc.live_count() == 1);  // mark not called; release_fn is a stub
}

static void test_b71_block_allocator_handle_ids_one_based() {
    rt::OwnedBlockAllocator alloc;
    packet::AudioPacketId pid{1};
    auto h1 = alloc.allocate(pid, 16);
    auto h2 = alloc.allocate(pid, 16);
    assert(h1.handle_id.value == 1);
    assert(h2.handle_id.value == 2);
}

// ---------- B7: SyncByteViewGuard ----------

static void test_b72_sync_view_guard_acquire_returns_view() {
    data::AudioSyncByteView view{};
    view.handle_id.value = 42;
    std::vector<std::byte> buf{std::byte{0x11}, std::byte{0x22}};
    view.bytes = std::span<const std::byte>{buf.data(), buf.size()};
    {
        rt::SyncByteViewGuard guard(view);
        assert(guard.is_valid());
        auto acquired = guard.acquire();
        assert(acquired.handle_id.value == 42);
        assert(acquired.bytes.size() == 2);
        assert(acquired.bytes[0] == std::byte{0x11});
    }
}

static void test_b72b_sync_view_guard_valid_inside_scope() {
    data::AudioSyncByteView view{};
    view.handle_id.value = 7;
    std::vector<std::byte> buf{std::byte{0x33}};
    view.bytes = std::span<const std::byte>{buf.data(), buf.size()};
    {
        rt::SyncByteViewGuard guard(view);
        assert(guard.is_valid());
        // View still has bytes while guard is alive.
        assert(view.bytes.size() == 1);
    }
}

static void test_b73_sync_view_guard_scope_exit_invalidates_view() {
    data::AudioSyncByteView view{};
    view.handle_id.value = 7;
    std::vector<std::byte> buf{std::byte{0x33}};
    view.bytes = std::span<const std::byte>{buf.data(), buf.size()};
    assert(view.bytes.size() == 1);
    {
        rt::SyncByteViewGuard guard(view);
        assert(view.bytes.size() == 1);
    }
    // After scope exit, the guard must have invalidated the borrowed view.
    assert(view.bytes.size() == 0);
}

static void test_b74_sync_view_guard_move_construct_transfers_validity() {
    data::AudioSyncByteView view{};
    view.handle_id.value = 9;
    std::vector<std::byte> buf{std::byte{0x44}};
    view.bytes = std::span<const std::byte>{buf.data(), buf.size()};
    {
        rt::SyncByteViewGuard g1(view);
        assert(g1.is_valid());
        rt::SyncByteViewGuard g2(std::move(g1));
        assert(g2.is_valid());
        assert(!g1.is_valid());  // moved-from
        // g2 still holds the view; on g2 destruction the view is invalidated.
    }
    assert(view.bytes.size() == 0);
}

static void test_b75_sync_view_guard_move_assign_transfers_validity() {
    data::AudioSyncByteView viewA{};
    viewA.handle_id.value = 10;
    std::vector<std::byte> bufA{std::byte{0x55}};
    viewA.bytes = std::span<const std::byte>{bufA.data(), bufA.size()};

    data::AudioSyncByteView viewB{};
    viewB.handle_id.value = 11;
    std::vector<std::byte> bufB{std::byte{0x66}};
    viewB.bytes = std::span<const std::byte>{bufB.data(), bufB.size()};

    {
        rt::SyncByteViewGuard gA(viewA);
        rt::SyncByteViewGuard gB(viewB);
        gA = std::move(gB);
        // After move-assign, gA should be valid (took over gB's state) and
        // gB should be invalid.
        assert(gA.is_valid());
        assert(!gB.is_valid());
    }
    // viewA was invalidated when gA was move-assigned over.
    assert(viewA.bytes.size() == 0);
    // viewB is NOT invalidated (guard uses reference, cannot rebind).
    assert(viewB.bytes.size() != 0);
}

static void test_b76_sync_view_guard_self_assign_safe() {
    data::AudioSyncByteView view{};
    view.handle_id.value = 12;
    std::vector<std::byte> buf{std::byte{0x77}};
    view.bytes = std::span<const std::byte>{buf.data(), buf.size()};
    {
        rt::SyncByteViewGuard guard(view);
        // Self-move-assign must be safe (no-op per implementation guard).
        guard = std::move(guard);
        // Behavior is implementation-defined but must not crash.
    }
}

static void test_b77_sync_view_guard_nested_scope_invalidation() {
    data::AudioSyncByteView view{};
    view.handle_id.value = 13;
    std::vector<std::byte> buf{std::byte{0x88}, std::byte{0x99}};
    view.bytes = std::span<const std::byte>{buf.data(), buf.size()};
    assert(view.bytes.size() == 2);
    {
        rt::SyncByteViewGuard outer(view);
        assert(outer.is_valid());
        assert(view.bytes.size() == 2);
        {
            // Nested scope: the inner guard wraps the same view reference.
            rt::SyncByteViewGuard inner(view);
            assert(inner.is_valid());
        }
        // After inner scope exit, the view was invalidated by inner guard.
        assert(view.bytes.size() == 0);
    }
    // Outer destruction is a no-op on already-invalidated view.
    assert(view.bytes.size() == 0);
}

int main() {
    printf("=== P6B data_handle_tests ===\n");

    // B5: FakeDataHandleManager
    P6B_RUN(test_b56_handle_manager_acquire_sync_view);
    P6B_RUN(test_b57_handle_manager_acquire_owned_block);
    P6B_RUN(test_b58_handle_manager_acquire_pool_slot);
    P6B_RUN(test_b59_handle_manager_release_sync_view_released);
    P6B_RUN(test_b60_handle_manager_release_owned_block_released);
    P6B_RUN(test_b61_handle_manager_release_pool_slot_released);
    P6B_RUN(test_b62_handle_manager_release_already_released);
    P6B_RUN(test_b63_handle_manager_release_not_found);
    P6B_RUN(test_b64_handle_manager_multiple_active_handles);
    P6B_RUN(test_b65_handle_manager_distinct_handle_ids);

    // B6: OwnedBlockAllocator
    P6B_RUN(test_b66_block_allocator_allocate_returns_valid_handle);
    P6B_RUN(test_b67_block_allocator_allocate_multiple);
    P6B_RUN(test_b68_block_allocator_mark_released_reduces_live_count);
    P6B_RUN(test_b69_block_allocator_mark_released_invalid_handle_noop);
    P6B_RUN(test_b70_block_allocator_release_fn_is_noexcept_callable);
    P6B_RUN(test_b71_block_allocator_handle_ids_one_based);

    // B7: SyncByteViewGuard
    P6B_RUN(test_b72_sync_view_guard_acquire_returns_view);
    P6B_RUN(test_b72b_sync_view_guard_valid_inside_scope);
    P6B_RUN(test_b73_sync_view_guard_scope_exit_invalidates_view);
    P6B_RUN(test_b74_sync_view_guard_move_construct_transfers_validity);
    P6B_RUN(test_b75_sync_view_guard_move_assign_transfers_validity);
    P6B_RUN(test_b76_sync_view_guard_self_assign_safe);
    P6B_RUN(test_b77_sync_view_guard_nested_scope_invalidation);

    printf("=== P6B data_handle_tests: %d failed ===\n", g_failed);
    return g_failed == 0 ? 0 : 1;
}
