// wasapi_render_client_guard.hpp — P6D internal: RAII guard for IAudioRenderClient
#pragma once

#include <audioclient.h>
#include <wrl/client.h>

namespace kivo::video::audio_plane::backend::output::wasapi::internal {

// RAII guard for IAudioRenderClient.
class RenderClientGuard {
public:
    RenderClientGuard() noexcept = default;

    void take(IAudioRenderClient* client) noexcept { client_ = client; }
    IAudioRenderClient* get() noexcept { return client_.Get(); }
    const IAudioRenderClient* get() const noexcept { return client_.Get(); }
    bool is_valid() const noexcept { return client_ != nullptr; }
    void reset() noexcept { client_.Reset(); }

private:
    Microsoft::WRL::ComPtr<IAudioRenderClient> client_;
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi::internal
