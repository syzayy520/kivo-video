#include "video/subtitle_plane/snapshot/snapshot_lease_pool.hpp"

namespace kivo::video::subtitle_plane::runtime {
namespace {

constexpr uint64_t kSnapshotLeaseSlots = 64;

uint64_t active_limit(const PlaneStateRecord& record) noexcept {
    return record.caps.snapshot_active_lease_capacity < kSnapshotLeaseSlots
        ? record.caps.snapshot_active_lease_capacity
        : kSnapshotLeaseSlots;
}

}  // namespace

KivoP8ApiStatus acquire_snapshot_lease(
    PlaneStateRecord& record,
    KivoSubtitleSnapshotLease* const out_lease) noexcept {
    *out_lease = KivoSubtitleSnapshotLease{};
    out_lease->snapshot_revision = record.snapshot_revision;
    if (record.state == KivoSubtitlePlaneState_Closed) {
        out_lease->status = KivoSubtitleSnapshotLeaseStatus_Closed;
        out_lease->error = KivoSubtitlePlaneError_ClosedSubtitlePlane;
        return KivoP8ApiStatus_Ok;
    }
    if (record.metrics.active_snapshot_lease_count >= active_limit(record)) {
        out_lease->status = KivoSubtitleSnapshotLeaseStatus_Error;
        out_lease->error = KivoSubtitlePlaneError_SubtitleSnapshotLeaseInvalid;
        return KivoP8ApiStatus_Ok;
    }
    const uint64_t lease_id = ++record.next_snapshot_lease_id;
    const uint64_t slot = record.metrics.active_snapshot_lease_count++;
    record.active_snapshot_lease_ids[slot] = lease_id;
    out_lease->snapshot_lease_id.value = lease_id;
    out_lease->snapshot_lease_generation = 1;
    out_lease->snapshot_revision = record.snapshot_revision;
    out_lease->status = KivoSubtitleSnapshotLeaseStatus_Ok;
    record.metrics.snapshot_count += 1;
    return KivoP8ApiStatus_Ok;
}

KivoP8ApiStatus release_snapshot_lease(
    PlaneStateRecord& record,
    const KivoSubtitleSnapshotLease& lease) noexcept {
    if (lease.snapshot_lease_id.value == 0) {
        return KivoP8ApiStatus_Ok;
    }
    for (uint64_t index = 0; index < record.metrics.active_snapshot_lease_count; ++index) {
        if (record.active_snapshot_lease_ids[index] == lease.snapshot_lease_id.value) {
            const uint64_t last = record.metrics.active_snapshot_lease_count - 1;
            record.active_snapshot_lease_ids[index] = record.active_snapshot_lease_ids[last];
            record.active_snapshot_lease_ids[last] = 0;
            record.metrics.active_snapshot_lease_count = last;
            record.metrics.retired_snapshot_count += 1;
            return KivoP8ApiStatus_Ok;
        }
    }
    return KivoP8ApiStatus_InvalidArgument;
}

}  // namespace kivo::video::subtitle_plane::runtime
