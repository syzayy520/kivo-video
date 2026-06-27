// queue_drain_escape.hpp — P5D D6: Queue drain escape hatch
#pragma once
#include <cstdint>
#include <atomic>

namespace kivo::video_plane::decoder {

struct DrainEscapeConfig {
    uint64_t max_drain_ms{5000};
    uint64_t check_interval_ms{100};
    bool force_terminate_on_timeout{true};
    bool operator==(DrainEscapeConfig const&) const = default;
};

enum class DrainState {
    NotStarted,
    Draining,
    Completed,
    TimedOut,
    ForceTerminated
};

class FakeQueueDrainEscape {
public:
    explicit FakeQueueDrainEscape(DrainEscapeConfig const& config)
        : config_(config) {}

    void start_drain() {
        state_ = DrainState::Draining;
        start_ms_ = 0;
    }

    DrainState check(uint64_t elapsed_ms) {
        if (state_ != DrainState::Draining) return state_;
        if (elapsed_ms > config_.max_drain_ms) {
            state_ = DrainState::TimedOut;
            if (config_.force_terminate_on_timeout) {
                state_ = DrainState::ForceTerminated;
            }
        }
        return state_;
    }

    void complete() { state_ = DrainState::Completed; }
    DrainState state() const { return state_; }

private:
    DrainEscapeConfig config_;
    DrainState state_{DrainState::NotStarted};
    uint64_t start_ms_{0};
};

}  // namespace kivo::video_plane::decoder
