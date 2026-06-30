#include "video/playback_graph/p8_bridge/subtitle_frame_p8_bridge.hpp"

#include <algorithm>
#include <cstring>

#include "kivo/video/subtitle_plane/foundation/p8_version.h"
#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace kivo::video::playback_graph::runtime {
namespace {

KivoSubtitlePlaneApiV1& api_table() noexcept {
    static KivoSubtitlePlaneApiV1 api{};
    static bool loaded = false;
    if (!loaded) {
        api.header.struct_size = sizeof(api);
        loaded = kivo_subtitle_plane_get_api_v1(KIVO_P8_ABI_REVISION, &api) == KivoP8ApiStatus_Ok;
    }
    return api;
}

[[nodiscard]] KivoSubtitlePlaneHandle plane_handle(const std::uint64_t value) noexcept {
    return KivoSubtitlePlaneHandle{value};
}

[[nodiscard]] bool copy_primary_text(const KivoSubtitlePlaneApiV1& api,
                                     const KivoSubtitlePlaneHandle plane,
                                     const KivoSubtitleFrameLease& lease,
                                     char* const out,
                                     const std::size_t out_size) noexcept {
    if (out == nullptr || out_size == 0) {
        return false;
    }
    out[0] = '\0';

    KivoSubtitleResourceAccessToken token{};
    token.kind = KivoSubtitleResourceAccessKind_FrameLease;
    token.lease_id = lease.lease_id;
    token.lease_generation = lease.lease_generation;
    token.snapshot_revision = lease.snapshot_revision;

    uint64_t required = 0;
    if (api.copy_text_buffer(plane, token, KivoSubtitleTextBufferId{1}, KivoMutableByteSpan{},
                             &required) != KivoP8ApiStatus_Ok ||
        required == 0) {
        return false;
    }

    uint8_t buffer[256]{};
    const std::size_t copy_size = std::min<std::size_t>(required, sizeof(buffer) - 1);
    if (api.copy_text_buffer(plane, token, KivoSubtitleTextBufferId{1},
                             KivoMutableByteSpan{buffer, copy_size}, &required) !=
        KivoP8ApiStatus_Ok) {
        return false;
    }
    std::memcpy(out, buffer, copy_size);
    out[copy_size] = '\0';
    return true;
}

}  // namespace

bool SubtitleFrameP8Bridge::ensure_api() noexcept {
    api_ready_ = api_table().header.struct_size != 0;
    return api_ready_;
}

void SubtitleFrameP8Bridge::attach(const std::uint64_t track_id) noexcept {
    detach();
    if (!ensure_api() || track_id == 0) {
        return;
    }

    auto& api = api_table();
    KivoSubtitlePlaneConfig config{};
    config.caps.frame_lease_capacity = 1;
    config.caps.subtitle_time_compare_epsilon_us = 5000;
    config.caps.rcu_hazard_acquire_max_retries = 3;

    KivoSubtitlePlaneHandle plane{};
    if (api.create_plane(&config, &plane) != KivoP8ApiStatus_Ok) {
        return;
    }

    static const uint8_t kEmbeddedCueSource[] = {'1', '\n'};
    KivoSubtitleSourceDescriptor source{};
    source.kind = KivoSubtitleSourceKind_MemoryBytes;
    source.memory_bytes.data = kEmbeddedCueSource;
    source.memory_bytes.bytes = sizeof(kEmbeddedCueSource);

    KivoSubtitleCommandToken token{};
    if (api.set_source(plane, &source, KivoSubtitleCorrelationId{1}, &token) != KivoP8ApiStatus_Ok) {
        api.destroy_plane(plane);
        return;
    }

    plane_value_ = plane.value;
    track_id_ = track_id;
}

void SubtitleFrameP8Bridge::detach() noexcept {
    if (plane_value_ == 0 || !api_ready_) {
        plane_value_ = 0;
        track_id_ = 0;
        return;
    }

    auto& api = api_table();
    const KivoSubtitlePlaneHandle plane = plane_handle(plane_value_);
    api.close_plane(plane);
    api.destroy_plane(plane);
    plane_value_ = 0;
    track_id_ = 0;
}

void SubtitleFrameP8Bridge::reset() noexcept {
    detach();
    api_ready_ = false;
}

bool SubtitleFrameP8Bridge::active() const noexcept {
    return api_ready_ && plane_value_ != 0 && track_id_ != 0;
}

SubtitleFrameSnapshot SubtitleFrameP8Bridge::query(const std::int64_t media_time_ms,
                                                   const std::int64_t delay_ms,
                                                   const std::uint64_t track_id,
                                                   const bool subtitle_enabled) const noexcept {
    SubtitleFrameSnapshot frame{};
    frame.valid = true;
    frame.media_time_ms = media_time_ms + delay_ms;
    frame.track_id = track_id;

    if (!subtitle_enabled) {
        frame.unavailable_reason = SubtitleFrameUnavailableReason::SubtitleDisabled;
        return frame;
    }

    if (!active()) {
        frame.unavailable_reason = SubtitleFrameUnavailableReason::NoRuntime;
        return frame;
    }

    frame.runtime_connected = true;

    auto& api = api_table();
    const KivoSubtitlePlaneHandle plane = plane_handle(plane_value_);

    KivoSubtitleFrameBuildRequest request{};
    request.playback_time.microseconds = frame.media_time_ms * 1000;
    request.playback_revision.value = 1;
    request.graph_snapshot_revision.value = 1;
    request.seek_epoch.value = 1;

    KivoSubtitleFrameLease lease{};
    if (api.build_frame(plane, &request, &lease) != KivoP8ApiStatus_Ok) {
        frame.unavailable_reason = SubtitleFrameUnavailableReason::NoActiveCue;
        return frame;
    }

    if (lease.status == KivoSubtitleFrameLeaseStatus_Ok) {
        frame.frame_available = true;
        frame.primary.cue_id = 1;
        frame.primary.visible = true;
        (void)copy_primary_text(api, plane, lease, frame.primary.primary_text,
                                sizeof(frame.primary.primary_text));
        api.release_frame(plane, &lease);
        return frame;
    }

    if (lease.status == KivoSubtitleFrameLeaseStatus_Empty &&
        lease.error == KivoSubtitlePlaneError_SubtitleTimelineNotStarted) {
        frame.unavailable_reason = SubtitleFrameUnavailableReason::TimelineNotStarted;
    } else {
        frame.unavailable_reason = SubtitleFrameUnavailableReason::NoActiveCue;
    }
    if (lease.lease_id != 0) {
        api.release_frame(plane, &lease);
    }
    return frame;
}

}  // namespace kivo::video::playback_graph::runtime