// wasapi_audio_client_guard.hpp — P6D internal: RAII guard for IAudioClient
#pragma once

#include <audioclient.h>
#include <wrl/client.h>

namespace kivo::video::audio_plane::backend::output::wasapi::internal {

// RAII guard for IAudioClient.
class AudioClientGuard {
public:
    AudioClientGuard() noexcept = default;

    void take(IAudioClient* client) noexcept { client_ = client; }
    IAudioClient* get() noexcept { return client_.Get(); }
    const IAudioClient* get() const noexcept { return client_.Get(); }
    bool is_valid() const noexcept { return client_ != nullptr; }
    void reset() noexcept { client_.Reset(); }

private:
    Microsoft::WRL::ComPtr<IAudioClient> client_;
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi::internal
