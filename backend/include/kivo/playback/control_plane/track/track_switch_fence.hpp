#pragma once

#include <optional>
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/track/required_downstream_acks.hpp"
#include "kivo/playback/control_plane/feedback/downstream_feedback_queue.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::track {

/// FallbackAuthorizationKind enumerates track-switch fallback authorization sources.
enum class FallbackAuthorizationKind
{
    SameCredential,
    RefreshedCredential,
    DifferentCandidate,
    LicenseRefreshed,
    ProviderAuthorized
};

/// AuthorizedTrackSwitchPlan is an authorized track-switch plan.
struct AuthorizedTrackSwitchPlan
{
    primitives::U64 plan_id{0};
    revision::RevisionStamp authorized_revision{};
    primitives::U64 authorization_token{0};
    feedback::StreamType stream_type{feedback::StreamType::Unknown};
    RequiredDownstreamAcks required_acks{};
    primitives::U32 stable_reason_code{0};
};

/// TrackSwitchPolicy defines track-switch authorization and fence rules.
struct TrackSwitchPolicy
{
    bool allow_midstream_track_switch{false};
    bool requires_p3_authorization{true};
    bool requires_stream_revision_increment{true};
    bool requires_downstream_ack{true};
    primitives::Milliseconds fence_ack_timeout_ms{};
};

/// TrackSwitchFenceState enumerates fence lifecycle states.
enum class TrackSwitchFenceState
{
    NotArmed,
    Armed,
    AllAckReceived,
    Expired,
    Failed,
    Released
};

/// TrackSwitchFence tracks required downstream acks for a track switch.
/// Timeout authority is TrackSwitchPolicy.fence_ack_timeout_ms.
/// Required ack set comes from AuthorizedTrackSwitchPlan.
/// all_required_ack_received compares ack kind + fence id + stream revision.
/// Partial ack timeout must emit TrackSwitchFenceTimeout.
struct TrackSwitchFence
{
    revision::TrackSwitchFenceId id{};
    revision::StreamRevision new_revision{};
    RequiredDownstreamAcks required_acks{};
    primitives::MonotonicTimestamp armed_at{};
    TrackSwitchFenceState state{TrackSwitchFenceState::NotArmed};

    void arm(revision::TrackSwitchFenceId fence_id,
             revision::StreamRevision new_rev,
             const RequiredDownstreamAcks& acks,
             primitives::MonotonicTimestamp now)
    {
        id = fence_id;
        new_revision = new_rev;
        required_acks = acks;
        armed_at = now;
        state = TrackSwitchFenceState::Armed;
    }

    void submit_ack(const feedback::DownstreamAck& ack)
    {
        if (state != TrackSwitchFenceState::Armed) return;
        if (ack.stream_revision.value != new_revision.value) return;
        for (primitives::U32 i = 0; i < required_acks.count; ++i)
        {
            if (required_acks.kinds[i] == ack.kind)
            {
                required_acks.kinds[i] = required_acks.kinds[--required_acks.count];
                break;
            }
        }
        if (required_acks.count == 0)
            state = TrackSwitchFenceState::AllAckReceived;
    }

    bool all_required_ack_received() const noexcept
    {
        return state == TrackSwitchFenceState::AllAckReceived;
    }

    bool expired(primitives::MonotonicTimestamp now, const TrackSwitchPolicy& policy) const noexcept
    {
        if (state != TrackSwitchFenceState::Armed) return false;
        return now.ticks_ns >= armed_at.ticks_ns + policy.fence_ack_timeout_ms.value;
    }

    void release() { state = TrackSwitchFenceState::Released; }
    void fail() { state = TrackSwitchFenceState::Failed; }
};

} // namespace kivo::playback::control_plane::track
