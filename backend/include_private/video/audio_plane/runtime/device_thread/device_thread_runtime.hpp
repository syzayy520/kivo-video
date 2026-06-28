// device_thread_runtime.hpp — P6E: audio device thread runtime
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>

namespace kivo::video::audio_plane::backend::runtime::device_thread {

enum class DeviceThreadState : int32_t {
    NotStarted = 0,
    Running = 1,
    Stopping = 2,
    Stopped = 3,
};

// DeviceThreadRuntime
// Abstracts the AudioDeviceThread: a high-priority thread that fills the
// WASAPI render buffer from an SPSC queue. The real thread spawning is
// deferred; this provides the contract and state machine.
class DeviceThreadRuntime {
public:
    using RenderCallback = std::function<void()>;

    DeviceThreadRuntime() = default;

    // Start the device thread. The callback is invoked on each buffer period.
    bool start(RenderCallback callback);

    // Signal the device thread to stop.
    void stop() noexcept;

    DeviceThreadState state() const noexcept {
        return state_.load(std::memory_order_acquire);
    }

    // Set thread priority (1=normal, 2=high, 3=realtime-proposed).
    void set_priority(int32_t priority) noexcept { priority_ = priority; }
    int32_t priority() const noexcept { return priority_; }

private:
    std::atomic<DeviceThreadState> state_{DeviceThreadState::NotStarted};
    int32_t priority_{1};
};

}  // namespace kivo::video::audio_plane::backend::runtime::device_thread
