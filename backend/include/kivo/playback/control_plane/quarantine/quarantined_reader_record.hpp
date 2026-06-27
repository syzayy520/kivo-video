#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/recovery/playback_failure.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::quarantine {

/// QuarantinedReaderRecord holds a quarantined reader for safe lifetime.
/// Rules:
/// - Quarantined reader cannot access PlaybackSession.
/// - All callbacks go to no-op sink.
/// - Quarantined worker does NOT block session TERMINATED if policy allows.
/// - QuarantineRegistry holds lifetime anchor.
/// - Session can terminate while quarantined reader naturally drains.
/// - Quarantine no-op callback must still count diagnostics, but cannot touch session state.
struct QuarantinedReaderRecord
{
    revision::ReaderInstanceId reader_id{};
    revision::SessionId session_id{};
    revision::RevisionStamp invalidated_stamp{};
    void* reader{nullptr};            // shared_ptr<IPlaybackReader> (opaque)
    void* lifetime_anchor{nullptr};   // shared_ptr<ReaderWorkerLifetimeAnchor> (opaque)
    primitives::MonotonicTimestamp quarantined_at{};
    recovery::PlaybackFailure root_failure{};
    bool callbacks_permanently_ignored{false};
};

} // namespace kivo::playback::control_plane::quarantine
