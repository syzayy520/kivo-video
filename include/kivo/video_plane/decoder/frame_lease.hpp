// frame_lease.hpp — P5D D3: FrameLease / RetireToken
#pragma once
#include <cstdint>
#include <atomic>

namespace kivo::video_plane::decoder {

struct RetireToken {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(RetireToken const&) const = default;
};

struct FrameLease {
    uint64_t frame_handle{0};
    RetireToken retire_token;
    uint64_t leased_at_ms{0};
    uint64_t max_lease_ms{5000};
    std::atomic<uint32_t> ref_count{0};

    bool is_expired(uint64_t current_ms) const {
        return (current_ms - leased_at_ms) > max_lease_ms;
    }

    void add_ref() { ref_count.fetch_add(1, std::memory_order_relaxed); }
    void release() { ref_count.fetch_sub(1, std::memory_order_relaxed); }
    uint32_t refs() const { return ref_count.load(std::memory_order_relaxed); }
};

}  // namespace kivo::video_plane::decoder
