#pragma once

#include <array>
#include "kivo/playback/control_plane/feedback/downstream_feedback_queue.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::track {

/// RequiredDownstreamAcks holds the ack kind set required by a track switch.
struct RequiredDownstreamAcks
{
    std::array<feedback::DownstreamAckKind, 8> kinds{};
    primitives::U32 count{0};
};

} // namespace kivo::playback::control_plane::track
