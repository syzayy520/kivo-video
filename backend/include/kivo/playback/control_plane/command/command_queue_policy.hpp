#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::command {

/// CommandQueuePolicy defines queue depth and delay limits.
struct CommandQueuePolicy
{
    primitives::Count normal_max_depth{};
    primitives::Count critical_reserved_depth{};
    primitives::Milliseconds max_queue_delay_ms{};
    bool reject_when_full{false};
};

} // namespace kivo::playback::control_plane::command
