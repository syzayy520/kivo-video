// wasapi_device_guard.hpp — P6D internal: RAII guard for IMMDevice
#pragma once

#include <mmdeviceapi.h>
#include <wrl/client.h>

namespace kivo::video::audio_plane::backend::output::wasapi::internal {

// RAII guard for IMMDevice.
class DeviceGuard {
public:
    DeviceGuard() noexcept = default;

    void take(IMMDevice* device) noexcept { device_ = device; }
    IMMDevice* get() noexcept { return device_.Get(); }
    const IMMDevice* get() const noexcept { return device_.Get(); }
    bool is_valid() const noexcept { return device_ != nullptr; }
    void reset() noexcept { device_.Reset(); }

private:
    Microsoft::WRL::ComPtr<IMMDevice> device_;
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi::internal
