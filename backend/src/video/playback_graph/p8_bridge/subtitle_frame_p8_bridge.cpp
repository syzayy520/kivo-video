#include "video/playback_graph/p8_bridge/subtitle_frame_p8_bridge.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

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

[[nodiscard]] KivoSubtitleFormat format_from_codec_name(std::string codec_name) noexcept {
    std::transform(codec_name.begin(), codec_name.end(), codec_name.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (codec_name == "subrip" || codec_name == "srt") {
        return KivoSubtitleFormat_Srt;
    }
    if (codec_name == "ass") {
        return KivoSubtitleFormat_Ass;
    }
    if (codec_name == "ssa") {
        return KivoSubtitleFormat_Ssa;
    }
    if (codec_name == "webvtt") {
        return KivoSubtitleFormat_WebVtt;
    }
    if (codec_name == "mov_text" || codec_name == "text") {
        return KivoSubtitleFormat_WebVtt;
    }
    if (codec_name == "hdmv_pgs_subtitle" || codec_name == "pgs") {
        return KivoSubtitleFormat_Pgs;
    }
    if (codec_name == "dvd_subtitle" || codec_name == "vobsub") {
        return KivoSubtitleFormat_VobSub;
    }
    if (codec_name == "dvb_subtitle") {
        return KivoSubtitleFormat_DvbSub;
    }
    return KivoSubtitleFormat_Unsupported;
}

[[nodiscard]] bool create_plane(KivoSubtitlePlaneApiV1& api,
                                KivoSubtitlePlaneHandle& plane) noexcept {
    KivoSubtitlePlaneConfig config{};
    config.caps.frame_lease_capacity = 1;
    config.caps.subtitle_time_compare_epsilon_us = 5000;
    config.caps.rcu_hazard_acquire_max_retries = 3;
    return api.create_plane(&config, &plane) == KivoP8ApiStatus_Ok;
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
    const std::size_t copy_size =
        std::min<std::size_t>(std::min<std::size_t>(required, sizeof(buffer) - 1),
                              out_size - 1);
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

void SubtitleFrameP8Bridge::attach(const std::uint64_t track_id,
                                   const bool allow_contract_fixture_source) noexcept {
    detach();
    stats_ = {};
    if (!ensure_api() || track_id == 0) {
        return;
    }
    track_id_ = track_id;

    auto& api = api_table();
    KivoSubtitlePlaneHandle plane{};
    if (!create_plane(api, plane)) {
        return;
    }

    KivoSubtitleSourceDescriptor source{};
    if (allow_contract_fixture_source) {
        static const uint8_t kEmbeddedCueSource[] = {'c', 'u', 'e'};
        source.kind = KivoSubtitleSourceKind_MemoryBytes;
        source.memory_bytes.data = kEmbeddedCueSource;
        source.memory_bytes.bytes = sizeof(kEmbeddedCueSource);
    } else {
        source.kind = KivoSubtitleSourceKind_EmbeddedTrack;
        source.source_id = KivoSubtitleSourceId{track_id};
    }

    KivoSubtitleCommandToken token{};
    if (api.set_source(plane, &source, KivoSubtitleCorrelationId{1}, &token) != KivoP8ApiStatus_Ok) {
        api.destroy_plane(plane);
        return;
    }

    plane_value_ = plane.value;
    track_id_ = track_id;
    stats_.embedded_track_attached = !allow_contract_fixture_source;
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
    stats_ = {};
}

void SubtitleFrameP8Bridge::reset() noexcept {
    detach();
    api_ready_ = false;
}

bool SubtitleFrameP8Bridge::active() const noexcept {
    return api_ready_ && plane_value_ != 0 && track_id_ != 0;
}

SubtitleFrameP8BridgeStats SubtitleFrameP8Bridge::stats() const noexcept {
    SubtitleFrameP8BridgeStats current = stats_;
    if (!active()) {
        return current;
    }

    KivoSubtitleMetricsSnapshot metrics{};
    if (api_table().get_metrics(plane_handle(plane_value_), &metrics) == KivoP8ApiStatus_Ok) {
        current.unsupported_feature_count = metrics.unsupported_feature_count;
    }
    return current;
}

bool SubtitleFrameP8Bridge::submit_embedded_packet(
    const EmbeddedSubtitlePacketSubmission& packet) noexcept {
    if (!active() || packet.data.empty() || packet.end_time_us <= packet.start_time_us) {
        ++stats_.rejected_packet_count;
        return false;
    }

    const KivoSubtitleFormat format = format_from_codec_name(packet.codec_name);
    KivoEmbeddedSubtitlePacketSubmitRequest request{};
    request.header.struct_size = sizeof(request);
    request.correlation_id = KivoSubtitleCorrelationId{stats_.submitted_packet_count + 2u};
    request.source_id = KivoSubtitleSourceId{track_id_};
    request.packet_generation.source_generation = 1;
    request.packet_generation.track_generation = track_id_;
    request.packet_generation.seek_epoch.value = 1;
    request.packet_generation.packet_generation = stats_.submitted_packet_count + 1u;
    request.format = format;
    request.flags = packet.key_packet ? KivoEmbeddedSubtitlePacketFlag_KeyPacket
                                      : KivoEmbeddedSubtitlePacketFlag_None;
    request.start_time.microseconds = packet.start_time_us;
    request.end_time.microseconds = packet.end_time_us;
    request.packet_bytes.data = packet.data.data();
    request.packet_bytes.bytes = packet.data.size();

    KivoSubtitleCommandToken token{};
    const KivoP8ApiStatus status = kivo_subtitle_plane_submit_embedded_packet_v1(
        plane_handle(plane_value_), &request, &token);
    if (status != KivoP8ApiStatus_Ok) {
        ++stats_.rejected_packet_count;
        return false;
    }
    stats_.embedded_packet_ingest_connected = true;
    ++stats_.submitted_packet_count;
    return true;
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