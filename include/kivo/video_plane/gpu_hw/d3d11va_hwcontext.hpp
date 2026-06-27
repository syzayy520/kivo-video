// d3d11va_hwcontext.hpp -- P5G G1: D3D11VA hwcontext bridge (fake, no real D3D11)
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_hw {

enum class HwcontextState {
    NotInitialized,
    Initialized,
    DeviceLost,
    Released
};

struct FakeD3D11vaHwcontext {
    HwcontextState state_{HwcontextState::NotInitialized};
    uint64_t device_epoch_{0};
    uint64_t adapter_luid_{0};

    bool initialize(uint64_t adapter_luid) {
        if (state_ != HwcontextState::NotInitialized && state_ != HwcontextState::Released) return false;
        state_ = HwcontextState::Initialized;
        adapter_luid_ = adapter_luid;
        device_epoch_ = ++global_epoch_;
        return true;
    }

    bool is_valid() const { return state_ == HwcontextState::Initialized; }
    HwcontextState state() const { return state_; }
    uint64_t device_epoch() const { return device_epoch_; }
    uint64_t adapter_luid() const { return adapter_luid_; }

    void simulate_device_lost() { if (state_ == HwcontextState::Initialized) state_ = HwcontextState::DeviceLost; }
    void release() { state_ = HwcontextState::Released; }

private:
    static inline uint64_t global_epoch_{0};
};

}  // namespace kivo::video_plane::gpu_hw
