#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/core/plane_store.hpp"
#include "video/subtitle_plane/snapshot/snapshot_blob_writer.hpp"
#include "video/subtitle_plane/snapshot/snapshot_lease_pool.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_acquire_snapshot(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleSnapshotAcquireRequest* const request,
    KivoSubtitleSnapshotLease* const out_lease) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) return KivoP8ApiStatus_InvalidArgument;
    if (request == nullptr || out_lease == nullptr) return KivoP8ApiStatus_NullPointer;
    if ((request->flags & 0x80000000u) != 0u) {
        *out_lease = KivoSubtitleSnapshotLease{};
        out_lease->snapshot_revision = record->snapshot_revision;
        out_lease->status = KivoSubtitleSnapshotLeaseStatus_Error;
        out_lease->error = KivoSubtitlePlaneError_SubtitleFrameAcquireContention;
        return KivoP8ApiStatus_NotReady;
    }
    return acquire_snapshot_lease(*record, out_lease);
}

KivoP8ApiStatus KIVO_P8_CALL api_copy_snapshot(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleSnapshotLease* const lease,
    const KivoMutableByteSpan dst,
    uint64_t* const out_required_bytes) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) return KivoP8ApiStatus_InvalidArgument;
    if (lease == nullptr || out_required_bytes == nullptr) return KivoP8ApiStatus_NullPointer;
    if (lease->status != KivoSubtitleSnapshotLeaseStatus_Ok ||
        lease->snapshot_lease_id.value == 0) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    return write_snapshot_blob(*record, dst, out_required_bytes);
}

KivoP8ApiStatus KIVO_P8_CALL api_release_snapshot(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleSnapshotLease* const lease) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) return KivoP8ApiStatus_InvalidArgument;
    return lease == nullptr ? KivoP8ApiStatus_NullPointer : release_snapshot_lease(*record, *lease);
}

}  // namespace kivo::video::subtitle_plane::runtime
