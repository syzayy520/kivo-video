// wasapi_render_loop.cpp — P6D backend: render loop implementation
#include "video/audio_plane/output/wasapi/wasapi_render_loop.hpp"

namespace kivo::video::audio_plane::backend::output::wasapi {

bool WasapiRenderLoop::start(FillCallback callback) {
    if (state_ == RenderLoopState::Running) return false;
    if (!callback) return false;
    // Real implementation would spawn a thread that waits on the buffer event
    // and calls the callback to fill data. For now, mark as running.
    state_ = RenderLoopState::Running;
    return true;
}

void WasapiRenderLoop::stop() {
    if (state_ != RenderLoopState::Running) return;
    state_ = RenderLoopState::Stopping;
    // Real implementation would signal the thread to stop and join.
    state_ = RenderLoopState::Stopped;
}

}  // namespace kivo::video::audio_plane::backend::output::wasapi
