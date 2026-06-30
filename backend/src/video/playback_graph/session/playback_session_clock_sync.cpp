#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

void PlaybackSessionRuntime::sync_clock_store() noexcept {
    snapshot_store_.set_clock_estimate(position_ms_, duration_ms_ > 0);
}

void PlaybackSessionRuntime::publish_current_snapshot() noexcept {
    sync_clock_store();
    (void)snapshot_store_.publish(snapshot());
}

}  // namespace kivo::video::playback_graph::runtime