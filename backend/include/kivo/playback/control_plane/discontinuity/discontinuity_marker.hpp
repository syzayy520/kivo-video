#pragma once

#include <optional>
#include "kivo/playback/control_plane/discontinuity/discontinuity_reason.hpp"
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"

namespace kivo::playback::control_plane::discontinuity {

/// DiscontinuityMarker marks a stream discontinuity with reason/revision/target/fence.
/// P4 does NOT tell P5/P6/P7 how to flush.
/// P4 only emits reason/revision/target/fence.
struct DiscontinuityMarker
{
    DiscontinuityReason reason{DiscontinuityReason::Unknown};
    revision::SessionGeneration generation{};
    revision::SeekEpoch seek_epoch{};
    revision::RecoveryEpoch recovery_epoch{};
    revision::StreamRevision old_revision{};
    revision::StreamRevision new_revision{};
    std::optional<primitives::MediaTimeMs> target_position_ms{};
    std::optional<primitives::SourceOffset> target_source_offset{};
    bool requires_downstream_ack{false};
    std::optional<revision::TrackSwitchFenceId> fence_id{};
    primitives::MonotonicTimestamp emitted_at{};
};

} // namespace kivo::playback::control_plane::discontinuity
