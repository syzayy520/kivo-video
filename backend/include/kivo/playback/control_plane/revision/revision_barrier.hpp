#pragma once

#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::revision {

/// RevisionBarrierKind enumerates barrier kinds.
enum class RevisionBarrierKind
{
    Seek,
    Stop,
    Recovery,
    RecoveryFallback,
    Flush,
    TrackSwitch,
    RuntimePolicyUpdate
};

/// RevisionBarrierCommit records barrier commit semantics.
/// T0: barrier intent accepted
/// T1: new revision atomically published
/// T2: queues drained/rejected up to barrier watermark
struct RevisionBarrierCommit
{
    RevisionStamp old_stamp{};
    RevisionStamp new_stamp{};
    primitives::MonotonicTimestamp intent_accepted_at{};
    primitives::MonotonicTimestamp committed_at{};
    primitives::U64 queue_watermark{0};
};

/// RevisionBarrierResult is the result of a barrier operation.
struct RevisionBarrierResult
{
    RevisionBarrierKind kind{};
    RevisionStamp new_stamp{};
    RevisionBarrierCommit commit{};
    bool accepted{false};
};

/// RevisionBarrier guarantees: after new publish point, old stamps cannot be newly accepted.
/// Already-queued old events must be rejected at consumer validation.
/// Barrier does NOT guarantee already-queued old events vanish immediately.
class RevisionBarrier
{
public:
    RevisionBarrierResult begin_seek_barrier() { return begin(RevisionBarrierKind::Seek); }
    RevisionBarrierResult begin_stop_barrier() { return begin(RevisionBarrierKind::Stop); }
    RevisionBarrierResult begin_recovery_barrier() { return begin(RevisionBarrierKind::Recovery); }
    RevisionBarrierResult begin_recovery_fallback_barrier() { return begin(RevisionBarrierKind::RecoveryFallback); }
    RevisionBarrierResult begin_flush_barrier() { return begin(RevisionBarrierKind::Flush); }
    RevisionBarrierResult begin_track_switch_barrier() { return begin(RevisionBarrierKind::TrackSwitch); }
    RevisionBarrierResult begin_runtime_policy_update_barrier() { return begin(RevisionBarrierKind::RuntimePolicyUpdate); }

    bool validate(const RevisionStamp& stamp) const noexcept
    {
        return stamp.session_id.value == current_.session_id.value
            && stamp.generation.value == current_.generation.value;
    }

    RevisionStamp current_stamp() const noexcept { return current_; }

private:
    RevisionBarrierResult begin(RevisionBarrierKind kind)
    {
        RevisionBarrierResult result;
        result.kind = kind;
        result.commit.old_stamp = current_;
        current_.generation = SessionGeneration{current_.generation.value + 1};
        result.new_stamp = current_;
        result.accepted = true;
        return result;
    }

    RevisionStamp current_{};
};

} // namespace kivo::playback::control_plane::revision
