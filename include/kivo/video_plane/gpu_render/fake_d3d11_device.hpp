// fake_d3d11_device.hpp -- P5F F1: D3D11 device boundary (fake, no real D3D11)
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_render {

enum class DeviceBoundaryState {
    NotCreated,
    Created,
    DeviceLost,
    Released
};

struct FakeD3D11DeviceBoundary {
    DeviceBoundaryState state_{DeviceBoundaryState::NotCreated};
    uint64_t device_epoch{0};
    uint64_t adapter_luid{0};
    std::string feature_level;

    bool create(uint64_t adapter_luid_) {
        if (state_ != DeviceBoundaryState::NotCreated && state_ != DeviceBoundaryState::Released) return false;
        state_ = DeviceBoundaryState::Created;
        adapter_luid = adapter_luid_;
        device_epoch = ++global_epoch_;
        feature_level = "11_0";
        return true;
    }

    bool is_valid() const { return state_ == DeviceBoundaryState::Created; }
    DeviceBoundaryState state() const { return state_; }

    void simulate_device_lost() {
        if (state_ == DeviceBoundaryState::Created) state_ = DeviceBoundaryState::DeviceLost;
    }

    void release() {
        state_ = DeviceBoundaryState::Released;
    }

    uint64_t epoch() const { return device_epoch; }

private:
    static inline uint64_t global_epoch_{0};
};

}  // namespace kivo::video_plane::gpu_render
