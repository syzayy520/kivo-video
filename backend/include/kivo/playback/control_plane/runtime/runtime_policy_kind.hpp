#pragma once

namespace kivo::playback::control_plane::runtime {

/// RuntimePolicyKind sole authority: runtime/runtime_policy_kind.hpp
enum class RuntimePolicyKind
{
    BufferRequirement,
    ReadThrottle,
    LowSpeed,
    ReaderStall,
    PauseBuffer,
    PlaybackRate,
    LiveStream,
    TrackSwitch,
    IdleTimeout,
    AdaptiveBackpressure,
    ResourceBudget,
    RecoveryPlan
};

} // namespace kivo::playback::control_plane::runtime
