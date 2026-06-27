// out_of_band_backpressure.hpp — P5B B5: Out-of-band backpressure atomic publish
#pragma once
#include <cstdint>
#include <atomic>

namespace kivo::video_plane::input_bridge {

enum class BackpressureLevel : uint32_t {
    None = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4
};

// OutOfBandBackpressure — atomic backpressure publish for cross-thread communication
class OutOfBandBackpressure {
public:
    OutOfBandBackpressure() = default;

    // Publish backpressure level (atomic, lock-free)
    void publish(BackpressureLevel level) {
        level_.store(static_cast<uint32_t>(level), std::memory_order_release);
        ++revision_;
    }

    // Read current backpressure level (atomic, lock-free)
    BackpressureLevel read() const {
        return static_cast<BackpressureLevel>(level_.load(std::memory_order_acquire));
    }

    // Read revision counter (to detect changes)
    uint64_t revision() const {
        return revision_.load(std::memory_order_acquire);
    }

    // Check if backpressure changed since last revision
    bool changed_since(uint64_t last_seen_revision) const {
        return revision() != last_seen_revision;
    }

private:
    std::atomic<uint32_t> level_{0};
    std::atomic<uint64_t> revision_{0};
};

}  // namespace kivo::video_plane::input_bridge
