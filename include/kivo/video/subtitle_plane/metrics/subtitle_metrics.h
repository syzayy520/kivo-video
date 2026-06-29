#ifndef KIVO_VIDEO_SUBTITLE_PLANE_METRICS_SUBTITLE_METRICS_H
#define KIVO_VIDEO_SUBTITLE_PLANE_METRICS_SUBTITLE_METRICS_H

#include "kivo/video/subtitle_plane/foundation/p8_abi_header.h"

typedef struct KivoSubtitleMetricsSnapshot {
    uint64_t cue_count;
    uint64_t skipped_cue_count;
    uint64_t repaired_cue_count;
    uint64_t unsupported_feature_count;
    uint64_t packet_queue_depth;
    uint64_t packet_queue_bytes;
    uint64_t peak_packet_queue_bytes;
    uint64_t packet_drop_count;
    uint64_t visible_cue_count;
    uint64_t visible_cue_cap_hit_count;
    uint64_t generation_discard_count;
    uint64_t generation_wrap_risk_count;
    uint64_t frame_pool_exhausted_count;
    uint64_t active_frame_lease_count;
    uint64_t active_snapshot_lease_count;
    uint64_t frame_lease_leak_count;
    uint64_t snapshot_lease_leak_count;
    uint64_t snapshot_count;
    uint64_t retired_snapshot_count;
    uint64_t font_blob_bytes;
    uint64_t text_pool_bytes;
    uint64_t string_pool_bytes;
    uint64_t evidence_overflow_count;
    uint64_t observer_queue_overflow_count;
    uint64_t last_parse_time_ms;
    uint64_t max_parse_time_ms_observed;
} KivoSubtitleMetricsSnapshot;

#endif
