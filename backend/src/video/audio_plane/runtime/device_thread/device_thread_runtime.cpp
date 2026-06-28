// device_thread_runtime.cpp — P6E: device thread runtime implementation
#include "video/audio_plane/runtime/device_thread/device_thread_runtime.hpp"

namespace kivo::video::audio_plane::backend::runtime::device_thread {

bool DeviceThreadRuntime::start(RenderCallback callback) {
    if (!callback) return false;
    DeviceThreadState expected = DeviceThreadState::NotStarted;
    if (!state_.compare_exchange_strong(expected, DeviceThreadState::Running,
                                         std::memory_order_acq_rel)) {
        return false;  // already running
    }
    // Real implementation would spawn a thread with PROAUDIO priority here.
    // For now, the state machine is correct and the callback contract is defined.
    return true;
}

void DeviceThreadRuntime::stop() noexcept {
    state_.store(DeviceThreadState::Stopping, std::memory_order_release);
    // Real implementation would join the thread here.
    state_.store(DeviceThreadState::Stopped, std::memory_order_release);
}

}  // namespace kivo::video::audio_plane::backend::runtime::device_thread
