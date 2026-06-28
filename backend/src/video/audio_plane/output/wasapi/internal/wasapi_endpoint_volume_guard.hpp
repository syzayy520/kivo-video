// wasapi_endpoint_volume_guard.hpp — P6D internal: RAII guard for IAudioEndpointVolume
#pragma once

#include <endpointvolume.h>
#include <wrl/client.h>

namespace kivo::video::audio_plane::backend::output::wasapi::internal {

// RAII guard for IAudioEndpointVolume.
class EndpointVolumeGuard {
public:
    EndpointVolumeGuard() noexcept = default;

    void take(IAudioEndpointVolume* vol) noexcept { vol_ = vol; }
    IAudioEndpointVolume* get() noexcept { return vol_.Get(); }
    bool is_valid() const noexcept { return vol_ != nullptr; }
    void reset() noexcept { vol_.Reset(); }

private:
    Microsoft::WRL::ComPtr<IAudioEndpointVolume> vol_;
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi::internal
