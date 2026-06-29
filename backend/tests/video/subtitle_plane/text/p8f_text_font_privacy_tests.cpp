#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

#include "video/subtitle_plane/privacy/privacy_redline_policy.hpp"

namespace {

KivoSubtitlePlaneApiV1 api_table() {
    KivoSubtitlePlaneApiV1 api{};
    api.header.struct_size = sizeof(api);
    if (kivo_subtitle_plane_get_api_v1(KIVO_P8_ABI_REVISION, &api) != KivoP8ApiStatus_Ok) {
        api.header.struct_size = 0;
    }
    return api;
}

KivoSubtitleFrameBuildRequest frame_request() {
    KivoSubtitleFrameBuildRequest request{};
    request.playback_revision.value = 1;
    request.graph_snapshot_revision.value = 1;
    request.seek_epoch.value = 1;
    return request;
}

}  // namespace

int main() {
    KivoSubtitlePlaneApiV1 api = api_table();
    KivoSubtitlePlaneHandle plane{};
    if (api.create_plane(nullptr, &plane) != KivoP8ApiStatus_Ok) return 1;
    const uint8_t bytes[] = {'1'};
    KivoSubtitleSourceDescriptor source{};
    source.kind = KivoSubtitleSourceKind_MemoryBytes;
    source.memory_bytes.data = bytes;
    source.memory_bytes.bytes = sizeof(bytes);
    KivoSubtitleCommandToken token{};
    if (api.set_source(plane, &source, KivoSubtitleCorrelationId{1}, &token) != KivoP8ApiStatus_Ok) return 2;

    KivoSubtitleFrameLease frame{};
    KivoSubtitleFrameBuildRequest build_request = frame_request();
    if (api.build_frame(plane, &build_request, &frame) != KivoP8ApiStatus_Ok) return 3;
    KivoSubtitleResourceAccessToken frame_token{};
    frame_token.kind = KivoSubtitleResourceAccessKind_FrameLease;
    frame_token.lease_id = frame.lease_id;
    frame_token.lease_generation = frame.lease_generation;
    frame_token.snapshot_revision = frame.snapshot_revision;

    uint64_t required = 0;
    if (api.copy_text_buffer(plane, frame_token, KivoSubtitleTextBufferId{1}, KivoMutableByteSpan{}, &required) !=
        KivoP8ApiStatus_Ok) {
        return 4;
    }
    if (required != 3) return 5;
    uint8_t small[2]{};
    if (api.copy_text_buffer(plane, frame_token, KivoSubtitleTextBufferId{1},
                             KivoMutableByteSpan{small, sizeof(small)}, &required) !=
        KivoP8ApiStatus_InvalidArgument) {
        return 6;
    }
    uint8_t text[3]{};
    if (api.copy_text_buffer(plane, frame_token, KivoSubtitleTextBufferId{1},
                             KivoMutableByteSpan{text, sizeof(text)}, &required) != KivoP8ApiStatus_Ok) {
        return 7;
    }
    if (text[0] != 'c' || text[1] != 'u' || text[2] != 'e') return 8;
    if (api.copy_text_buffer(plane, frame_token, KivoSubtitleTextBufferId{99},
                             KivoMutableByteSpan{}, &required) != KivoP8ApiStatus_InvalidArgument) {
        return 9;
    }

    KivoSubtitleSnapshotAcquireRequest snapshot_request{};
    KivoSubtitleSnapshotLease snapshot{};
    if (api.acquire_snapshot(plane, &snapshot_request, &snapshot) != KivoP8ApiStatus_Ok) return 10;
    KivoSubtitleResourceAccessToken snapshot_token{};
    snapshot_token.kind = KivoSubtitleResourceAccessKind_SnapshotLease;
    snapshot_token.snapshot_lease_id = snapshot.snapshot_lease_id;
    snapshot_token.snapshot_lease_generation = snapshot.snapshot_lease_generation;
    snapshot_token.snapshot_revision = snapshot.snapshot_revision;
    if (api.copy_font_blob(plane, snapshot_token, KivoSubtitleFontBlobId{1}, KivoMutableByteSpan{}, &required) !=
        KivoP8ApiStatus_Ok) {
        return 11;
    }
    if (required != 2) return 12;
    uint8_t font[2]{};
    if (api.copy_font_blob(plane, snapshot_token, KivoSubtitleFontBlobId{1},
                           KivoMutableByteSpan{font, sizeof(font)}, &required) != KivoP8ApiStatus_Ok) {
        return 13;
    }
    if (font[0] != 0x4b || font[1] != 0x46) return 14;
    if (api.copy_font_blob(plane, frame_token, KivoSubtitleFontBlobId{1}, KivoMutableByteSpan{}, &required) !=
        KivoP8ApiStatus_InvalidArgument) {
        return 15;
    }
    if (api.copy_text_buffer(plane, snapshot_token, KivoSubtitleTextBufferId{1}, KivoMutableByteSpan{}, &required) !=
        KivoP8ApiStatus_InvalidArgument) {
        return 16;
    }

    KivoSubtitleMetricsSnapshot metrics{};
    if (api.get_metrics(plane, &metrics) != KivoP8ApiStatus_Ok) return 17;
    if (metrics.text_pool_bytes != 3 || metrics.font_blob_bytes != 2) return 18;

    using kivo::video::subtitle_plane::runtime::privacy_mode_is_export_safe;
    if (!privacy_mode_is_export_safe(KivoSubtitlePrivacyMode_Strict)) return 19;
    if (!privacy_mode_is_export_safe(KivoSubtitlePrivacyMode_DiagnosticRedacted)) return 20;
    if (privacy_mode_is_export_safe(KivoSubtitlePrivacyMode_DiagnosticVerboseLocalOnly)) return 21;

    api.release_frame(plane, &frame);
    api.release_snapshot(plane, &snapshot);
    api.destroy_plane(plane);
    return 0;
}
