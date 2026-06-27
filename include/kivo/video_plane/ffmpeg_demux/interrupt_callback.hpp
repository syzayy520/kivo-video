// interrupt_callback.hpp — P5C C3: Interrupt callback binding contract
#pragma once
#include <cstdint>
#include <functional>
#include <atomic>

namespace kivo::video_plane::ffmpeg_demux {

// Contract: interrupt callback must be bound BEFORE open/probe/find/read
// This prevents FFmpeg from hanging on unresponsive sources

using InterruptCallback = std::function<bool()>;  // returns true to interrupt

enum class CallbackBindPoint {
    BeforeOpen,
    BeforeProbe,
    BeforeFindStreamInfo,
    BeforeReadPacket,
    BeforeSeek
};

struct InterruptCallbackBinding {
    InterruptCallback callback;
    uint64_t timeout_ms{5000};
    bool is_bound{false};

    bool check_timeout(uint64_t elapsed_ms) const {
        return elapsed_ms > timeout_ms;
    }
};

class FakeInterruptCallbackManager {
public:
    // Bind callback before a specific operation
    void bind(CallbackBindPoint point, InterruptCallback cb, uint64_t timeout_ms) {
        binding_.callback = std::move(cb);
        binding_.timeout_ms = timeout_ms;
        binding_.is_bound = true;
        last_bind_point_ = point;
    }

    // Verify callback is bound before operation
    bool verify_bound(CallbackBindPoint point) const {
        if (!binding_.is_bound) return false;
        // Callback must be bound before or at this point
        return static_cast<int>(last_bind_point_) <= static_cast<int>(point);
    }

    // Execute callback (returns true if should interrupt)
    bool execute(uint64_t elapsed_ms) const {
        if (!binding_.is_bound || !binding_.callback) return false;
        if (binding_.check_timeout(elapsed_ms)) return true;
        return binding_.callback();
    }

    void unbind() {
        binding_.is_bound = false;
    }

private:
    InterruptCallbackBinding binding_;
    CallbackBindPoint last_bind_point_{CallbackBindPoint::BeforeOpen};
};

}  // namespace kivo::video_plane::ffmpeg_demux
