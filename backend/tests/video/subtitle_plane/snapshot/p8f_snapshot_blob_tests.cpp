#include "kivo/video/subtitle_plane/kivo_subtitle_plane.h"

#include <bit>
#include <cstdint>

namespace {

KivoSubtitlePlaneApiV1 api_table() {
    KivoSubtitlePlaneApiV1 api{};
    api.header.struct_size = sizeof(api);
    if (kivo_subtitle_plane_get_api_v1(KIVO_P8_ABI_REVISION, &api) != KivoP8ApiStatus_Ok) {
        api.header.struct_size = 0;
    }
    return api;
}

int make_ready_plane(KivoSubtitlePlaneApiV1& api, KivoSubtitlePlaneHandle* out_plane) {
    KivoSubtitlePlaneConfig config{};
    config.caps.snapshot_active_lease_capacity = 1;
    config.caps.frame_lease_capacity = 2;
    if (api.create_plane(&config, out_plane) != KivoP8ApiStatus_Ok) return 100;
    const uint8_t bytes[] = {'1', '\n', '2'};
    KivoSubtitleSourceDescriptor source{};
    source.kind = KivoSubtitleSourceKind_MemoryBytes;
    source.memory_bytes.data = bytes;
    source.memory_bytes.bytes = sizeof(bytes);
    KivoSubtitleCommandToken token{};
    return api.set_source(*out_plane, &source, KivoSubtitleCorrelationId{1}, &token) == KivoP8ApiStatus_Ok
        ? 0
        : 101;
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
    static_assert(std::endian::native == std::endian::little);

    KivoSubtitlePlaneApiV1 api = api_table();
    KivoSubtitlePlaneHandle plane{};
    if (make_ready_plane(api, &plane) != 0) return 1;

    KivoSubtitleSnapshotAcquireRequest request{};
    KivoSubtitleSnapshotLease lease{};
    if (api.acquire_snapshot(plane, &request, &lease) != KivoP8ApiStatus_Ok) return 2;
    if (lease.status != KivoSubtitleSnapshotLeaseStatus_Ok || lease.snapshot_lease_id.value == 0) return 3;

    KivoSubtitleSnapshotLease exhausted{};
    if (api.acquire_snapshot(plane, &request, &exhausted) != KivoP8ApiStatus_Ok) return 4;
    if (exhausted.status != KivoSubtitleSnapshotLeaseStatus_Error) return 5;

    uint64_t required = 0;
    if (api.copy_snapshot(plane, &lease, KivoMutableByteSpan{}, &required) != KivoP8ApiStatus_Ok) return 6;
    if (required <= sizeof(KivoSubtitleSnapshotBlobHeader)) return 7;
    uint8_t small[8]{};
    if (api.copy_snapshot(plane, &lease, KivoMutableByteSpan{small, sizeof(small)}, &required) !=
        KivoP8ApiStatus_InvalidArgument) {
        return 8;
    }
    uint8_t buffer[1024]{};
    if (api.copy_snapshot(plane, &lease, KivoMutableByteSpan{buffer, sizeof(buffer)}, &required) !=
        KivoP8ApiStatus_Ok) {
        return 9;
    }
    const auto* header = reinterpret_cast<const KivoSubtitleSnapshotBlobHeader*>(buffer);
    if (header->magic != KIVO_P8_SNAPSHOT_BLOB_MAGIC ||
        header->endian != KIVO_P8_SNAPSHOT_ENDIAN_LITTLE ||
        header->blob_revision != KIVO_P8_SNAPSHOT_BLOB_REVISION ||
        header->section_count != 4 ||
        header->section_table_offset != sizeof(KivoSubtitleSnapshotBlobHeader) ||
        header->total_bytes != required) {
        return 10;
    }
    if (header->metrics.packet_queue_bytes != 3) return 11;
    const auto* sections = reinterpret_cast<const KivoSubtitleSnapshotBlobSection*>(
        buffer + header->section_table_offset);
    if (sections[0].kind != KivoSubtitleSnapshotSection_TrackArray ||
        sections[1].kind != KivoSubtitleSnapshotSection_EvidenceSummaryArray ||
        sections[2].kind != KivoSubtitleSnapshotSection_FontAttachmentArray ||
        sections[3].kind != KivoSubtitleSnapshotSection_CommandSummaryArray) {
        return 12;
    }

    if (api.release_snapshot(plane, &lease) != KivoP8ApiStatus_Ok) return 13;
    if (api.release_snapshot(plane, &lease) != KivoP8ApiStatus_InvalidArgument) return 14;

    if (api.acquire_snapshot(plane, &request, &lease) != KivoP8ApiStatus_Ok) return 15;
    KivoSubtitleFrameLease frame{};
    KivoSubtitleFrameBuildRequest build_request = frame_request();
    if (api.build_frame(plane, &build_request, &frame) != KivoP8ApiStatus_Ok) return 16;
    if (api.close_plane(plane) != KivoP8ApiStatus_Ok) return 17;
    KivoSubtitleMetricsSnapshot metrics{};
    if (api.get_metrics(plane, &metrics) != KivoP8ApiStatus_Ok) return 18;
    if (metrics.frame_lease_leak_count != 1 || metrics.snapshot_lease_leak_count != 1) return 19;
    if (api.release_frame(plane, &frame) != KivoP8ApiStatus_Ok) return 20;
    if (api.release_snapshot(plane, &lease) != KivoP8ApiStatus_Ok) return 21;

    KivoSubtitleSnapshotLease closed{};
    if (api.acquire_snapshot(plane, &request, &closed) != KivoP8ApiStatus_Ok) return 22;
    if (closed.status != KivoSubtitleSnapshotLeaseStatus_Closed) return 23;

    api.destroy_plane(plane);
    return 0;
}
