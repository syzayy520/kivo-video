#pragma once

#include <cstdint>

#include "kivo/video/playback_graph/playback_graph_error.hpp"

namespace kivo::video::playback_graph::runtime {

struct EosDrainPlan {
    bool audio_selected{true};
    bool video_selected{true};
    bool presentation_active{true};
    bool output_active{true};
};

struct EosDrainOutcome {
    bool complete{false};
    PlaybackGraphError error{PlaybackGraphError::None};
    std::uint32_t required_bits{0};
    std::uint32_t completed_bits{0};
    bool last_frame_timeout{false};
    bool output_drained_timeout{false};
};

class EosDrainCoordinator {
public:
    [[nodiscard]] EosDrainOutcome drain_fake(const EosDrainPlan& plan,
                                             bool inject_last_frame_timeout,
                                             bool inject_output_drained_timeout) noexcept;
};

}  // namespace kivo::video::playback_graph::runtime
