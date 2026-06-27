// context_lock_bridge.hpp -- P5G G3: D3D11 context lock bridge
// Only GpuDeviceThread or maintenance context can acquire lock
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_hw {

enum class ContextLockState {
    Unlocked,
    LockedByGpuDeviceThread,
    LockedByMaintenanceThread,
    LockFailed
};

class FakeContextLockBridge {
public:
    ContextLockState try_lock(bool is_gpu_device_thread, bool is_maintenance_context) {
        if (state_ != ContextLockState::Unlocked) {
            return ContextLockState::LockFailed;
        }
        if (is_gpu_device_thread) {
            state_ = ContextLockState::LockedByGpuDeviceThread;
            lock_count_++;
        } else if (is_maintenance_context) {
            state_ = ContextLockState::LockedByMaintenanceThread;
            lock_count_++;
        } else {
            return ContextLockState::LockFailed;
        }
        return state_;
    }

    void unlock() {
        state_ = ContextLockState::Unlocked;
    }

    ContextLockState state() const { return state_; }
    uint32_t lock_count() const { return lock_count_; }
    bool is_locked() const { return state_ != ContextLockState::Unlocked; }

private:
    ContextLockState state_{ContextLockState::Unlocked};
    uint32_t lock_count_{0};
};

}  // namespace kivo::video_plane::gpu_hw
