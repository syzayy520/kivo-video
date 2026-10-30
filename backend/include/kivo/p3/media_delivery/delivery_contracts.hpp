#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include "kivo/p3/source_model/source_kinds.hpp"

namespace kivo::p3::media_delivery {

/// ProgressiveDelivery: support progressive file candidates.
struct ProgressiveDelivery
{
    std::string schema_version{"p3.delivery.progressive.v1"};
    kivo::p3::source_model::MediaDeliveryKind kind{kivo::p3::source_model::MediaDeliveryKind::progressive_file};
    bool supports_range_read{false};
    bool supports_seek{false};
    int64_t content_length{0};
};

/// HLSBoundary: model HLS without implementing segment playback queue.
struct HlsBoundary
{
    std::string schema_version{"p3.delivery.hls.v1"};
    std::string manifest_identity{};
    int32_t variant_count{0};
    bool is_live{false};
    bool is_vod{false};
    bool has_encryption{false};
    bool has_ll_hls_hints{false};
    std::string subtitle_rendition_ref{};
    std::string audio_rendition_ref{};

    bool has_segment_scheduler() const noexcept { return false; }
};

/// DashBoundary: model DASH without DASH playback engine.
struct DashBoundary
{
    std::string schema_version{"p3.delivery.dash.v1"};
    std::string mpd_identity{};
    int32_t period_count{0};
    int32_t adaptation_set_count{0};
    int32_t representation_count{0};
    bool is_live{false};
    bool has_content_protection{false};
    bool has_low_latency_hints{false};

    bool has_abr_execution() const noexcept { return false; }
};

/// DrmMixedClear: handle mixed DRM/clear tracks correctly.
struct DrmMixedClear
{
    std::string schema_version{"p3.delivery.drm_mixed.v1"};
    bool has_drm_track{false};
    bool has_clear_fallback{false};
    bool partial_drm_blocked{false};
    std::string drm_metadata_ref{};
    std::string clear_candidate_ref{};

    bool is_playable() const noexcept { return has_clear_fallback || !has_drm_track; }
};

} // namespace kivo::p3::media_delivery
