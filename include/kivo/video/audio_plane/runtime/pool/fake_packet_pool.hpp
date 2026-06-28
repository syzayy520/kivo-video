// fake_packet_pool.hpp — P6B pool: fake preallocated packet pool
// HEADER-ONLY. In-process ring of packet slots with cap enforcement.
#pragma once
#include <cstdint>
#include <vector>
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/packet/audio_packet_pool_contract.hpp"

namespace kivo::video::audio_plane::runtime::pool {

// PoolFull: error when the packet pool ring is exhausted.
struct PacketPoolFull {};

// PoolStats: diagnostic snapshot of pool usage.
struct PoolStats {
    uint64_t current_usage{0};      // currently acquired slots
    uint64_t total_allocated{0};    // lifetime acquisitions
    bool cap_exceeded{false};       // true if any cap was breached
};

// FakePacketPool
// Preallocated ring of N slots. Acquire returns a slot id; release frees it.
// Caps (max_bytes, max_packets, max_duration_ms) are enforced as hard limits.
class FakePacketPool {
public:
    explicit FakePacketPool(uint32_t slot_count = 64,
                            int64_t max_bytes = 0,
                            int32_t max_packets = 0,
                            int64_t max_duration_ms = 0)
        : slots_(slot_count, false), max_bytes_(max_bytes),
          max_packets_(max_packets), max_duration_ms_(max_duration_ms) {}

    // Acquire a free slot. Returns PoolFull if the ring is exhausted or a cap
    // is breached.
    Expected<uint64_t, PacketPoolFull> acquire() noexcept {
        if (max_packets_ > 0 && static_cast<int32_t>(in_use_) >= max_packets_) {
            stats_.cap_exceeded = true;
            return PacketPoolFull{};
        }
        for (uint32_t i = 0; i < slots_.size(); ++i) {
            const uint32_t idx = (next_scan_ + i) % static_cast<uint32_t>(slots_.size());
            if (!slots_[idx]) {
                slots_[idx] = true;
                ++in_use_;
                ++next_scan_;
                if (next_scan_ >= slots_.size()) next_scan_ = 0;
                ++stats_.total_allocated;
                stats_.current_usage = in_use_;
                return static_cast<uint64_t>(idx + 1);  // slot ids are 1-based
            }
        }
        stats_.cap_exceeded = true;
        return PacketPoolFull{};
    }

    // Release a slot. Returns false if the slot was already free or invalid.
    bool release(uint64_t slot_id) noexcept {
        if (slot_id == 0 || slot_id > slots_.size()) {
            return false;
        }
        const std::size_t idx = static_cast<std::size_t>(slot_id - 1);
        if (!slots_[idx]) {
            return false;
        }
        slots_[idx] = false;
        --in_use_;
        stats_.current_usage = in_use_;
        return true;
    }

    // Account bytes against the byte cap. Returns false if cap exceeded.
    bool account_bytes(int64_t bytes) noexcept {
        if (max_bytes_ > 0 && current_bytes_ + bytes > max_bytes_) {
            stats_.cap_exceeded = true;
            return false;
        }
        current_bytes_ += bytes;
        return true;
    }

    // Account duration against the duration cap. Returns false if exceeded.
    bool account_duration_ms(int64_t ms) noexcept {
        if (max_duration_ms_ > 0 && current_duration_ms_ + ms > max_duration_ms_) {
            stats_.cap_exceeded = true;
            return false;
        }
        current_duration_ms_ += ms;
        return true;
    }

    PoolStats stats() const noexcept { return stats_; }
    uint32_t capacity() const noexcept { return static_cast<uint32_t>(slots_.size()); }
    uint64_t in_use() const noexcept { return in_use_; }

private:
    std::vector<bool> slots_;
    uint32_t next_scan_{0};
    uint64_t in_use_{0};
    int64_t max_bytes_{0};
    int32_t max_packets_{0};
    int64_t max_duration_ms_{0};
    int64_t current_bytes_{0};
    int64_t current_duration_ms_{0};
    PoolStats stats_;
};

}  // namespace kivo::video::audio_plane::runtime::pool
