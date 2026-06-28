// fake_render_packet_pool.hpp — P6B runtime: fake render packet pool
// HEADER-ONLY fake implementation. Platform-neutral: no WASAPI/FFmpeg/Qt/Win32/COM.
#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/render/audio_render_packet.hpp"

namespace kivo::video::audio_plane::runtime::render {

using kivo::video::audio_plane::render::AudioRenderPacket;

// Re-affirm the P6A static_asserts on AudioRenderPacket: the pool relies on
// nothrow move constructibility and nothrow destruction for safe leasing.
static_assert(std::is_nothrow_move_constructible_v<AudioRenderPacket>,
              "AudioRenderPacket must be nothrow move constructible (pool lease)");
static_assert(std::is_nothrow_destructible_v<AudioRenderPacket>,
              "AudioRenderPacket must be nothrow destructible (pool lease)");

// PoolFull: no free slots available for acquire.
enum class PoolFull : int32_t {
    NoFreeSlots = 0,
};

// FakeRenderPacketPool
// Preallocates AudioRenderPacket slots during construction. Leases are tracked
// by uint64_t lease_id (= slot index + 1, so 0 means "invalid lease").
// This is a FAKE ADAPTER: no real device-thread-safe pool, no real WASAPI.
class FakeRenderPacketPool {
public:
    explicit FakeRenderPacketPool(size_t capacity)
        : slots_(capacity), free_flags_(capacity, true) {}

    FakeRenderPacketPool() : FakeRenderPacketPool(16) {}

    // Acquire a free slot. Returns lease_id (= slot_index + 1) or PoolFull.
    kivo::Expected<uint64_t, PoolFull> acquire() {
        for (size_t i = 0; i < free_flags_.size(); ++i) {
            if (free_flags_[i]) {
                free_flags_[i] = false;
                slots_[i] = AudioRenderPacket{};
                const uint64_t lease_id = static_cast<uint64_t>(i + 1);
                ++acquired_count_;
                return lease_id;
            }
        }
        return PoolFull::NoFreeSlots;
    }

    // Release a lease. Returns true if the lease was valid and is now released.
    bool release(uint64_t lease_id) noexcept {
        if (lease_id == 0 || lease_id > slots_.size()) {
            return false;
        }
        const size_t index = static_cast<size_t>(lease_id - 1);
        if (free_flags_[index]) {
            return false;  // already free, double-release
        }
        free_flags_[index] = true;
        slots_[index] = AudioRenderPacket{};
        ++released_count_;
        return true;
    }

    // Number of free slots available for acquire.
    size_t available_count() const noexcept {
        size_t count = 0;
        for (bool f : free_flags_) {
            if (f) ++count;
        }
        return count;
    }

    size_t capacity() const noexcept { return slots_.size(); }
    uint64_t acquired_count() const noexcept { return acquired_count_; }
    uint64_t released_count() const noexcept { return released_count_; }

    // Access the packet bound to a lease (for the producer to fill).
    // Returns nullptr if the lease is invalid or not currently held.
    AudioRenderPacket* access(uint64_t lease_id) noexcept {
        if (lease_id == 0 || lease_id > slots_.size()) return nullptr;
        const size_t index = static_cast<size_t>(lease_id - 1);
        if (free_flags_[index]) return nullptr;
        return &slots_[index];
    }

private:
    std::vector<AudioRenderPacket> slots_;
    std::vector<bool> free_flags_;
    uint64_t acquired_count_{0};
    uint64_t released_count_{0};
};

}  // namespace kivo::video::audio_plane::runtime::render
