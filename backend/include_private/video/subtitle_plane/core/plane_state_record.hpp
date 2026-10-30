#pragma once

#include <cstddef>
#include <cstdint>

#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::subtitle_plane::runtime {

struct PlaneStateRecord {
    static constexpr std::size_t kMaxTextCues = 16;
    static constexpr std::size_t kMaxTextCueBytes = 256;
    static constexpr std::size_t kFrameLeaseSlots = 64;

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
    uint64_t active_frame_lease_ids[kFrameLeaseSlots]{};
    uint64_t active_frame_lease_text_cue_slots[kFrameLeaseSlots]{};
    bool active_text_cue = false;
    bool active_bitmap_packet = false;
    KivoSubtitleFormat active_format = KivoSubtitleFormat_Unknown;
    KivoSubtitleTime active_start_time{};
    KivoSubtitleTime active_end_time{};
    uint64_t active_text_bytes = 0;
    uint8_t active_text[kMaxTextCueBytes]{};
    uint64_t text_cue_count = 0;
    uint64_t next_text_cue_slot = 0;
    bool text_cue_valid[kMaxTextCues]{};
    KivoSubtitleTime text_cue_start_times[kMaxTextCues]{};
    KivoSubtitleTime text_cue_end_times[kMaxTextCues]{};
    KivoSubtitleFormat text_cue_formats[kMaxTextCues]{};
    uint64_t text_cue_bytes[kMaxTextCues]{};
    uint8_t text_cue_text[kMaxTextCues][kMaxTextCueBytes]{};
    uint64_t next_snapshot_lease_id = 0;
    uint64_t active_snapshot_lease_ids[kFrameLeaseSlots]{};
    KivoSubtitleObserverCallbacks observer_callbacks{};
    uint64_t observer_subscription_token = 0;
    uint32_t observer_dispatching = 0;
};

}  // namespace kivo::video::subtitle_plane::runtime
