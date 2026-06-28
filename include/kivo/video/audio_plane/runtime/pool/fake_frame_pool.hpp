// fake_frame_pool.hpp — P6B pool: fake preallocated frame pool
// HEADER-ONLY. In-process ring of frame leases with noexcept move guarantees.
#pragma once
#include <cstdint>
#include <type_traits>
#include <vector>
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/frame/audio_frame_pool_contract.hpp"
#include "kivo/video/audio_plane/frame/audio_frame_buffer.hpp"

namespace kivo::video::audio_plane::runtime::pool {

// PoolFull: error when the frame pool ring is exhausted.
struct FramePoolFull {};

// Internal frame slot type. Must satisfy the noexcept move / destruct
// guarantees declared by AudioFramePoolContract.
struct FrameSlot {
    frame::AudioFrameBuffer buffer{};
    bool in_use{false};
};

// Compile-time guarantees required by the P6A frame pool contract.
static_assert(std::is_nothrow_move_constructible_v<FrameSlot>,
              "FrameSlot must be nothrow move constructible");
static_assert(std::is_nothrow_destructible_v<FrameSlot>,
              "FrameSlot must be nothrow destructible");

// FakeFramePool
// Preallocated ring of M frame slots. Acquire returns a lease id; release
// frees the slot. recycle() marks all slots free without deallocating storage.
class FakeFramePool {
public:
    explicit FakeFramePool(uint32_t frame_count = 32)
        : slots_(frame_count) {}

    // Acquire a free frame slot. Returns PoolFull if the ring is exhausted.
    Expected<uint64_t, FramePoolFull> acquire() noexcept {
        for (uint32_t i = 0; i < slots_.size(); ++i) {
            const uint32_t idx = (next_scan_ + i) % static_cast<uint32_t>(slots_.size());
            if (!slots_[idx].in_use) {
                slots_[idx].in_use = true;
                ++next_scan_;
                if (next_scan_ >= slots_.size()) next_scan_ = 0;
                ++total_acquired_;
                ++current_usage_;
                return static_cast<uint64_t>(idx + 1);  // lease ids are 1-based
            }
        }
        return FramePoolFull{};
    }

    // Release a lease. Returns false if the lease was already free or invalid.
    bool release(uint64_t lease_id) noexcept {
        if (lease_id == 0 || lease_id > slots_.size()) {
            return false;
        }
        const std::size_t idx = static_cast<std::size_t>(lease_id - 1);
        if (!slots_[idx].in_use) {
            return false;
        }
        slots_[idx].in_use = false;
        slots_[idx].buffer = frame::AudioFrameBuffer{};  // reset contents
        --current_usage_;
        return true;
    }

    // Mark all slots free without deallocating storage (bulk recycle).
    void recycle() noexcept {
        for (auto& slot : slots_) {
            slot.in_use = false;
            slot.buffer = frame::AudioFrameBuffer{};
        }
        current_usage_ = 0;
        next_scan_ = 0;
    }

    // Access the frame buffer for a lease (read/write).
    frame::AudioFrameBuffer* get(uint64_t lease_id) noexcept {
        if (lease_id == 0 || lease_id > slots_.size()) return nullptr;
        const std::size_t idx = static_cast<std::size_t>(lease_id - 1);
        if (!slots_[idx].in_use) return nullptr;
        return &slots_[idx].buffer;
    }

    uint32_t capacity() const noexcept { return static_cast<uint32_t>(slots_.size()); }
    uint64_t current_usage() const noexcept { return current_usage_; }
    uint64_t total_acquired() const noexcept { return total_acquired_; }

private:
    std::vector<FrameSlot> slots_;
    uint32_t next_scan_{0};
    uint64_t current_usage_{0};
    uint64_t total_acquired_{0};
};

}  // namespace kivo::video::audio_plane::runtime::pool
