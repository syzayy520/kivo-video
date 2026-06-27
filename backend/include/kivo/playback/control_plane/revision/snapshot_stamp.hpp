#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::revision {

/// SnapshotStamp for immutable cross-thread snapshot reads.
/// publish uses release semantics.
/// load uses acquire semantics.
/// snapshot must be immutable.
/// External threads do not directly read internal mutable state.
struct SnapshotStamp
{
    SessionId session_id{};
    SessionGeneration generation{};
    TimelineRevision timeline_revision{};
    StreamRevision stream_revision{};
    SeekEpoch seek_epoch{};
    RecoveryEpoch recovery_epoch{};
    primitives::MonotonicTimestamp captured_at{};

    constexpr SnapshotStamp() noexcept = default;
};

} // namespace kivo::playback::control_plane::revision
