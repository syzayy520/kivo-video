#pragma once

#include "kivo/playback/control_plane/events/event_delivery_class.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::events {

/// EventBusPolicy defines event bus capacity and coalescing rules.
struct EventBusPolicy
{
    primitives::Count max_pending_events_per_subscriber{};
    bool coalesce_timeline_events{true};
    bool coalesce_diagnostics_events{true};
    bool drop_noncritical_when_full{true};
};

} // namespace kivo::playback::control_plane::events
