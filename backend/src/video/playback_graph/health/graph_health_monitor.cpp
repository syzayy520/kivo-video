#include "video/playback_graph/health/graph_health_monitor.hpp"

namespace kivo::video::playback_graph::runtime {

void GraphHealthMonitor::sample(GraphHealthSample sample) noexcept {
    latest_ = sample;
}

GraphHealthSample GraphHealthMonitor::latest() const noexcept {
    return latest_;
}

bool GraphHealthMonitor::unhealthy() const noexcept {
    return latest_.stalled || (!latest_.buffering_ready && latest_.queued_audio_ms == 0 &&
                                  latest_.queued_video_frames == 0);
}

}  // namespace kivo::video::playback_graph::runtime
