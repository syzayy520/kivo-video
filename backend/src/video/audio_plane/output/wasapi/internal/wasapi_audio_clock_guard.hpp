// wasapi_audio_clock_guard.hpp — P6D internal: RAII guard for IAudioClock
#pragma once

#include <audioclient.h>
#include <wrl/client.h>

namespace kivo::video::audio_plane::backend::output::wasapi::internal {

// RAII guard for IAudioClock.
class AudioClockGuard {
public:
    AudioClockGuard() noexcept = default;

    void take(IAudioClock* clock) noexcept { clock_ = clock; }
    IAudioClock* get() noexcept { return clock_.Get(); }
    bool is_valid() const noexcept { return clock_ != nullptr; }
    void reset() noexcept { clock_.Reset(); }

private:
    Microsoft::WRL::ComPtr<IAudioClock> clock_;
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi::internal
