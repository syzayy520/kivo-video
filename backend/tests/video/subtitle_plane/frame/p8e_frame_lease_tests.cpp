#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

#include "video/subtitle_plane/frame/frame_hot_path_guard.hpp"

namespace {

KivoSubtitlePlaneApiV1 api_table() {
    KivoSubtitlePlaneApiV1 api{};
    api.header.struct_size = sizeof(api);
    if (kivo_subtitle_plane_get_api_v1(KIVO_P8_ABI_REVISION, &api) != KivoP8ApiStatus_Ok) {
        api.header.struct_size = 0;
    }
    return api;
}

KivoSubtitleFrameBuildRequest request_at(const int64_t microseconds) {
    KivoSubtitleFrameBuildRequest request{};
    request.playback_time.microseconds = microseconds;
    request.playback_revision.value = 1;
    request.graph_snapshot_revision.value = 1;
    request.seek_epoch.value = 1;
    return request;
}

int make_ready_plane(KivoSubtitlePlaneApiV1& api,
                     const KivoSubtitlePlaneConfig* config,
                     KivoSubtitlePlaneHandle* out_plane) {
    if (api.create_plane(config, out_plane) != KivoP8ApiStatus_Ok) return 100;
    const uint8_t bytes[] = {'1', '\n'};
    KivoSubtitleSourceDescriptor source{};
    source.kind = KivoSubtitleSourceKind_MemoryBytes;
    source.memory_bytes.data = bytes;
    source.memory_bytes.bytes = sizeof(bytes);
    KivoSubtitleCommandToken token{};
    if (api.set_source(*out_plane, &source, KivoSubtitleCorrelationId{1}, &token) != KivoP8ApiStatus_Ok) {
        return 101;
    }
    return 0;
}

}  // namespace

int main() {
    KivoSubtitlePlaneApiV1 api = api_table();

    KivoSubtitlePlaneHandle cold{};
    if (api.create_plane(nullptr, &cold) != KivoP8ApiStatus_Ok) return 1;
    KivoSubtitleFrameBuildRequest request = request_at(0);
    KivoSubtitleFrameLease lease{};
    if (api.build_frame(cold, &request, &lease) != KivoP8ApiStatus_Ok) return 2;
    if (lease.status != KivoSubtitleFrameLeaseStatus_Empty ||
        lease.error != KivoSubtitlePlaneError_SubtitleTimelineNotStarted) {
        return 3;
    }
    api.destroy_plane(cold);

    KivoSubtitlePlaneConfig config{};
    config.caps.frame_lease_capacity = 1;
    config.caps.subtitle_time_compare_epsilon_us = 5000;
    config.caps.rcu_hazard_acquire_max_retries = 3;
    KivoSubtitlePlaneHandle plane{};
    if (make_ready_plane(api, &config, &plane) != 0) return 4;

    if (api.build_frame(plane, &request, &lease) != KivoP8ApiStatus_Ok) return 5;
    if (lease.status != KivoSubtitleFrameLeaseStatus_Ok ||
        lease.lease_id == 0 ||
        lease.frame == nullptr) {
        return 6;
    }
    KivoSubtitleFrameLease exhausted{};
    if (api.build_frame(plane, &request, &exhausted) != KivoP8ApiStatus_Ok) return 7;
    if (exhausted.status != KivoSubtitleFrameLeaseStatus_PoolExhausted ||
        exhausted.error != KivoSubtitlePlaneError_SubtitleFramePoolExhausted) {
        return 8;
    }
    if (api.release_frame(plane, &lease) != KivoP8ApiStatus_Ok) return 9;
    if (api.release_frame(plane, &lease) != KivoP8ApiStatus_InvalidArgument) return 10;
    KivoSubtitleFrameLease zero{};
    if (api.release_frame(plane, &zero) != KivoP8ApiStatus_Ok) return 11;

    request.flags = kivo::video::subtitle_plane::runtime::kP8FrameBuildFlagSimulateHazardContention;
    request.hazard_retry_budget = 0;
    KivoSubtitleFrameLease hazard{};
    if (api.build_frame(plane, &request, &hazard) != KivoP8ApiStatus_NotReady) return 12;
    if (hazard.error != KivoSubtitlePlaneError_SubtitleFrameAcquireContention) return 13;
    request.flags = 0;

    KivoEmbeddedSubtitleStreamEndRequest eos{};
    eos.packet_generation.packet_generation = 1;
    eos.flags = 1u;
    KivoSubtitleCommandToken token{};
    if (api.end_embedded_stream(plane, &eos, &token) != KivoP8ApiStatus_Ok) return 14;
    request = request_at(4000);
    if (api.build_frame(plane, &request, &lease) != KivoP8ApiStatus_Ok) return 15;
    if (lease.status != KivoSubtitleFrameLeaseStatus_Ok) return 16;
    api.release_frame(plane, &lease);
    request = request_at(6000);
    if (api.build_frame(plane, &request, &lease) != KivoP8ApiStatus_Ok) return 17;
    if (lease.status != KivoSubtitleFrameLeaseStatus_Empty ||
        lease.error != KivoSubtitlePlaneError_SubtitleTimelineTemporarilyUnavailable) {
        return 18;
    }

    if (api.close_plane(plane) != KivoP8ApiStatus_Ok) return 19;
    request = request_at(0);
    if (api.build_frame(plane, &request, &lease) != KivoP8ApiStatus_Ok) return 20;
    if (lease.status != KivoSubtitleFrameLeaseStatus_Closed) return 21;

    api.destroy_plane(plane);
    return 0;
}
