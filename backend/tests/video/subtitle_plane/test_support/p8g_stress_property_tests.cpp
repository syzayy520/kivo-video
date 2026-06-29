#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

namespace {

KivoSubtitlePlaneApiV1 api_table() {
    KivoSubtitlePlaneApiV1 api{};
    api.header.struct_size = sizeof(api);
    if (kivo_subtitle_plane_get_api_v1(KIVO_P8_ABI_REVISION, &api) != KivoP8ApiStatus_Ok) {
        api.header.struct_size = 0;
    }
    return api;
}

}  // namespace

int main() {
    KivoSubtitlePlaneApiV1 api = api_table();

    KivoSubtitlePlaneConfig bad_config{};
    bad_config.caps.max_packet_queue_bytes = UINT64_MAX;
    KivoSubtitlePlaneHandle plane{};
    if (api.create_plane(&bad_config, &plane) != KivoP8ApiStatus_InvalidArgument) return 1;

    if (api.create_plane(nullptr, &plane) != KivoP8ApiStatus_Ok) return 2;
    KivoSubtitleResourceAccessToken malformed{};
    malformed.kind = KivoSubtitleResourceAccessKind_FrameLease;
    malformed.lease_id = 1;
    malformed.lease_generation = 1;
    uint64_t required = 0;
    if (api.copy_text_buffer(plane, malformed, KivoSubtitleTextBufferId{1}, KivoMutableByteSpan{}, &required) !=
        KivoP8ApiStatus_InvalidArgument) {
        return 3;
    }

    KivoSubtitleSnapshotAcquireRequest snapshot_request{};
    snapshot_request.flags = 0x80000000u;
    KivoSubtitleSnapshotLease snapshot{};
    if (api.acquire_snapshot(plane, &snapshot_request, &snapshot) != KivoP8ApiStatus_NotReady) return 4;
    if (snapshot.error != KivoSubtitlePlaneError_SubtitleFrameAcquireContention) return 5;

    for (uint32_t index = 0; index < 16; ++index) {
        KivoEmbeddedSubtitleStreamEndRequest stream_end{};
        stream_end.packet_generation.packet_generation = index & 1u;
        KivoSubtitleCommandToken token{};
        if (api.end_embedded_stream(plane, &stream_end, &token) != KivoP8ApiStatus_Ok) return 6;
    }
    KivoSubtitleMetricsSnapshot metrics{};
    if (api.get_metrics(plane, &metrics) != KivoP8ApiStatus_Ok) return 7;
    if (metrics.generation_discard_count == 0) return 8;

    api.destroy_plane(plane);
    return 0;
}
