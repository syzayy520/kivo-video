#pragma once

#include <cstdint>

namespace kivo::video::playback_graph::runtime {

struct GraphHealthSample {
    bool buffering_ready{false};
    bool stalled{false};
    std::uint32_t queued_audio_ms{0};
    std::uint32_t queued_video_frames{0};
};

class GraphHealthMonitor {
public:
    void sample(GraphHealthSample sample) noexcept;
    [[nodiscard]] GraphHealthSample latest() const noexcept;
    [[nodiscard]] bool unhealthy() const noexcept;

private:
    GraphHealthSample latest_{};
};

}  // namespace kivo::video::playback_graph::runtime
