#ifndef KIVO_VIDEO_SUBTITLE_PLANE_CONFIG_SUBTITLE_PLANE_CONFIG_H
#define KIVO_VIDEO_SUBTITLE_PLANE_CONFIG_SUBTITLE_PLANE_CONFIG_H

#include "kivo/video/subtitle_plane/privacy/subtitle_privacy.h"

typedef struct KivoSubtitlePlaneRuntimeCaps {
    uint64_t max_packet_queue_bytes;
    uint64_t max_visible_cues;
    uint64_t max_text_pool_bytes;
    uint64_t max_font_blob_bytes;
    uint64_t subtitle_time_compare_epsilon_us;
    uint64_t frame_lease_capacity;
    uint64_t snapshot_active_lease_capacity;
    uint64_t subtitle_frame_lease_close_timeout_ms;
    uint64_t snapshot_lease_close_timeout_ms;
    uint32_t rcu_hazard_acquire_max_retries;
    uint32_t reserved;
} KivoSubtitlePlaneRuntimeCaps;

typedef struct KivoSubtitlePlaneConfig {
    KivoP8AbiHeader header;
    KivoSubtitlePlaneRuntimeCaps caps;
    KivoSubtitlePrivacyPolicy privacy_policy;
} KivoSubtitlePlaneConfig;

#endif
