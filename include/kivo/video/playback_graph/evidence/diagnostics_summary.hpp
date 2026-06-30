#pragma once

#include <cstdint>

#include "kivo/video/playback_graph/playback_graph_error.hpp"
#include "kivo/video/playback_graph/playback_graph_state.hpp"

namespace kivo::video::playback_graph {

struct DiagnosticsSummary {
    PlaybackGraphState state{PlaybackGraphState::NotCreated};
    PlaybackGraphError last_error{PlaybackGraphError::None};
    std::uint32_t retained_command_count{0};
    std::uint32_t dropped_critical_event_count{0};
    std::uint32_t late_event_discard_count{0};
    bool closed{false};
    bool valid{false};
};

}  // namespace kivo::video::playback_graph