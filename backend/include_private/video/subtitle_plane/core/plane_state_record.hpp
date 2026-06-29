#pragma once

#include <cstdint>

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

struct PlaneStateRecord {
    uint64_t handle_value = 0;
    KivoSubtitlePlaneRuntimeCaps caps{};
    KivoSubtitlePlaneState state = KivoSubtitlePlaneState_NotCreated;
    KivoSubtitleTimelineAvailability timeline =
        KivoSubtitleTimelineAvailability_NotYetStarted;
    KivoSubtitleSnapshotRevision snapshot_revision{};
    KivoP8PlaybackRevision latest_playback_revision{};
    KivoP8GraphSnapshotRevision latest_graph_revision{};
    uint64_t next_command_id = 1;
    KivoSubtitleCommandLifecycleSnapshot last_command{};
    KivoSubtitleMetricsSnapshot metrics{};
    KivoSubtitleEvidenceSummary evidence_summary{};
    uint64_t next_frame_id = 0;
    uint64_t next_frame_lease_id = 0;
    uint64_t active_frame_lease_ids[64]{};
    uint64_t next_snapshot_lease_id = 0;
    uint64_t active_snapshot_lease_ids[64]{};
    KivoSubtitleObserverCallbacks observer_callbacks{};
    uint64_t observer_subscription_token = 0;
    uint32_t observer_dispatching = 0;
};

}  // namespace kivo::video::subtitle_plane::runtime
