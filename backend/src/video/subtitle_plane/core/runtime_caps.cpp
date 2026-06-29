#include "video/subtitle_plane/core/runtime_caps.hpp"

namespace kivo::video::subtitle_plane::runtime {
namespace {

bool invalid_cap(const uint64_t value) noexcept {
    return value == UINT64_MAX;
}

}  // namespace

KivoSubtitlePlaneRuntimeCaps default_runtime_caps() noexcept {
    KivoSubtitlePlaneRuntimeCaps caps{};
    caps.max_packet_queue_bytes = 1024 * 1024;
    caps.max_visible_cues = 16;
    caps.max_text_pool_bytes = 1024 * 1024;
    caps.max_font_blob_bytes = 4 * 1024 * 1024;
    caps.subtitle_time_compare_epsilon_us = 1000;
    caps.frame_lease_capacity = 32;
    caps.snapshot_active_lease_capacity = 32;
    caps.subtitle_frame_lease_close_timeout_ms = 2000;
    caps.snapshot_lease_close_timeout_ms = 2000;
    caps.rcu_hazard_acquire_max_retries = 3;
    return caps;
}

KivoP8ApiStatus merge_runtime_caps(const KivoSubtitlePlaneConfig* const config,
                                   KivoSubtitlePlaneRuntimeCaps* const out_caps) noexcept {
    if (out_caps == nullptr) {
        return KivoP8ApiStatus_NullPointer;
    }
    KivoSubtitlePlaneRuntimeCaps caps = default_runtime_caps();
    if (config == nullptr) {
        *out_caps = caps;
        return KivoP8ApiStatus_Ok;
    }

    const KivoSubtitlePlaneRuntimeCaps requested = config->caps;
    if (invalid_cap(requested.max_packet_queue_bytes) ||
        invalid_cap(requested.max_visible_cues) ||
        invalid_cap(requested.max_text_pool_bytes) ||
        invalid_cap(requested.max_font_blob_bytes) ||
        invalid_cap(requested.subtitle_time_compare_epsilon_us) ||
        invalid_cap(requested.frame_lease_capacity) ||
        invalid_cap(requested.snapshot_active_lease_capacity) ||
        invalid_cap(requested.subtitle_frame_lease_close_timeout_ms) ||
        invalid_cap(requested.snapshot_lease_close_timeout_ms)) {
        return KivoP8ApiStatus_InvalidArgument;
    }

    if (requested.max_packet_queue_bytes != 0) caps.max_packet_queue_bytes = requested.max_packet_queue_bytes;
    if (requested.max_visible_cues != 0) caps.max_visible_cues = requested.max_visible_cues;
    if (requested.max_text_pool_bytes != 0) caps.max_text_pool_bytes = requested.max_text_pool_bytes;
    if (requested.max_font_blob_bytes != 0) caps.max_font_blob_bytes = requested.max_font_blob_bytes;
    if (requested.subtitle_time_compare_epsilon_us != 0) caps.subtitle_time_compare_epsilon_us = requested.subtitle_time_compare_epsilon_us;
    if (requested.frame_lease_capacity != 0) caps.frame_lease_capacity = requested.frame_lease_capacity;
    if (requested.snapshot_active_lease_capacity != 0) caps.snapshot_active_lease_capacity = requested.snapshot_active_lease_capacity;
    if (requested.subtitle_frame_lease_close_timeout_ms != 0) caps.subtitle_frame_lease_close_timeout_ms = requested.subtitle_frame_lease_close_timeout_ms;
    if (requested.snapshot_lease_close_timeout_ms != 0) caps.snapshot_lease_close_timeout_ms = requested.snapshot_lease_close_timeout_ms;
    if (requested.rcu_hazard_acquire_max_retries != 0) caps.rcu_hazard_acquire_max_retries = requested.rcu_hazard_acquire_max_retries;

    *out_caps = caps;
    return KivoP8ApiStatus_Ok;
}

}  // namespace kivo::video::subtitle_plane::runtime
