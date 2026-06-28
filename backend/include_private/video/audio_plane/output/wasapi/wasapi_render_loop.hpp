// wasapi_render_loop.hpp — P6D backend: event-driven render loop
#pragma once

#include <cstdint>
#include <functional>

namespace kivo::video::audio_plane::backend::output::wasapi {

// RenderLoopState mirrors the render loop's internal state (separate from backend lifecycle).
enum class RenderLoopState : int32_t {
    Idle = 0,
    Running = 1,
    Stopping = 2,
    Stopped = 3,
};

// WasapiRenderLoop
// Event-driven render loop: waits on buffer event, fills buffer from callback.
// The callback provides PCM bytes to write. Returns frames written.
class WasapiRenderLoop {
public:
    using FillCallback = std::function<int(uint8_t* dest, int frame_count)>;

    WasapiRenderLoop() = default;

    // Start the loop. The callback is called on the render thread.
    bool start(FillCallback callback);

    // Signal the loop to stop (non-blocking).
    void stop();

    RenderLoopState state() const noexcept { return state_; }

private:
    RenderLoopState state_{RenderLoopState::Idle};
};

}  // namespace kivo::video::audio_plane::backend::output::wasapi
