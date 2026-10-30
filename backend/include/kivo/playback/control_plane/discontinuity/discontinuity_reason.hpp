#pragma once

namespace kivo::playback::control_plane::discontinuity {

/// DiscontinuityReason enumerates reasons for a stream discontinuity.
/// P4 does NOT tell P5/P6/P7 how to flush.
/// P4 only emits reason/revision/target/fence.
/// P5/P6/P7 decide their own flush/reset/relock behavior.
enum class DiscontinuityReason
{
    SeekSnapshot,
    Flush,
    Stop,
    RecoveryReopen,
    NetworkSplice,
    AuthorizedFallbackSwitch,
    TrackSwitch,
    SourceRevisionChanged,
    RateChanged,
    LiveWindowJump,
    Unknown
};

} // namespace kivo::playback::control_plane::discontinuity
