// wasapi_session_control_guard.hpp — P6D internal: RAII guard for IAudioSessionControl
#pragma once

#include <audiopolicy.h>
#include <wrl/client.h>

namespace kivo::video::audio_plane::backend::output::wasapi::internal {

// RAII guard for IAudioSessionControl.
class SessionControlGuard {
public:
    SessionControlGuard() noexcept = default;

    void take(IAudioSessionControl* ctrl) noexcept { ctrl_ = ctrl; }
    IAudioSessionControl* get() noexcept { return ctrl_.Get(); }
    bool is_valid() const noexcept { return ctrl_ != nullptr; }
    void reset() noexcept { ctrl_.Reset(); }

private:
    Microsoft::WRL::ComPtr<IAudioSessionControl> ctrl_;
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi::internal
