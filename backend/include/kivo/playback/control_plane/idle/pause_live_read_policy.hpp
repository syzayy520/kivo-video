#pragma once

#include "kivo/playback/control_plane/live/live_stream_policy.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::idle {

/// ConsumerActivityKind enumerates consumer activity kinds.
enum class ConsumerActivityKind
{
    DataPulled,
    WouldBlockPoll,
    DemandHint,
    ClockObservation,
    DownstreamAck
};

/// PauseBufferPolicy defines pause buffer requirements.
struct PauseBufferPolicy
{
    bool enable_pause_buffer{true};
    primitives::U64 required_bytes{0};
    primitives::U64 required_ms{0};
};

/// IdleTimeoutPolicy defines idle release rules.
struct IdleTimeoutPolicy
{
    bool enable_idle_release{true};
    primitives::U64 idle_timeout_ms{0};
    primitives::U64 would_block_poll_rate_limit_ms{0};
};

/// PauseLiveReadDecision is the result of pause/live/idle policy resolution.
struct PauseLiveReadDecision
{
    bool continue_reading{false};
    bool chase_live_edge{false};
    bool allow_idle_release{false};
    primitives::U32 reason_code{0}; // StableReasonCode
};

/// resolve_pause_live_read_policy resolves pause/live/idle read decision.
/// Rules:
/// - WouldBlockPoll only refreshes idle when state=Playing/Buffering or under
///   rate-limited threshold.
/// - High-frequency WouldBlock polling must NOT prevent idle release forever.
/// - Idle release must wake all pipeline waiters.
inline PauseLiveReadDecision resolve_pause_live_read_policy(
    const PauseBufferPolicy& pause_policy,
    const live::LiveStreamPolicy& live_policy,
    const IdleTimeoutPolicy& idle_policy,
    ConsumerActivityKind activity,
    bool is_playing_or_buffering) noexcept
{
    PauseLiveReadDecision decision{};
    if (live_policy.is_live_like)
    {
        decision.chase_live_edge = true;
        decision.continue_reading = true;
        decision.reason_code = 1; // LiveChase
    }
    else if (pause_policy.enable_pause_buffer)
    {
        decision.continue_reading = true;
        decision.reason_code = 2; // PauseBuffer
    }
    else
    {
        decision.continue_reading = is_playing_or_buffering;
        decision.reason_code = 3; // Normal
    }

    // Idle release: WouldBlockPoll only refreshes idle when playing/buffering
    // or under rate-limited threshold.
    if (activity == ConsumerActivityKind::WouldBlockPoll)
    {
        if (!is_playing_or_buffering)
        {
            decision.allow_idle_release = idle_policy.enable_idle_release;
        }
    }
    else
    {
        decision.allow_idle_release = false;
    }

    return decision;
}

} // namespace kivo::playback::control_plane::idle
