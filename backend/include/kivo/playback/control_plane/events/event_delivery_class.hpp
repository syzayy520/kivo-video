#pragma once

namespace kivo::playback::control_plane::events {

/// EventDeliveryClass enumerates event delivery classes.
/// - Critical (Terminated/ErrorRaised/StopStarted/TrackSwitchFenceTimeout) must NOT be dropped.
/// - Timeline: may be coalesced.
/// - Diagnostics: may be coalesced.
/// - Verbose: may be sampled or dropped.
enum class EventDeliveryClass
{
    Critical,
    State,
    Timeline,
    Diagnostics,
    Verbose
};

} // namespace kivo::playback::control_plane::events
