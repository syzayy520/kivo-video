#include "video/subtitle_plane/frame/frame_lease_pool.hpp"

#include "video/subtitle_plane/core/plane_lifecycle_model.hpp"
#include "video/subtitle_plane/frame/frame_hot_path_guard.hpp"

#include <cstdint>

namespace kivo::video::subtitle_plane::runtime {
namespace {

constexpr uint64_t kNoTextCueLeaseSlot = 0;

uint64_t active_limit(const PlaneStateRecord& record) noexcept {
    return record.caps.frame_lease_capacity < PlaneStateRecord::kFrameLeaseSlots
        ? record.caps.frame_lease_capacity
        : PlaneStateRecord::kFrameLeaseSlots;
}

[[nodiscard]] bool time_in_range(const KivoSubtitleTime time,
                                 const KivoSubtitleTime start,
                                 const KivoSubtitleTime end) noexcept {
    return time.microseconds >= start.microseconds && time.microseconds < end.microseconds;
}

[[nodiscard]] uint64_t active_text_cue_slot(
    const PlaneStateRecord& record,
    const KivoSubtitleTime playback_time) noexcept {
    for (std::size_t index = 0; index < PlaneStateRecord::kMaxTextCues; ++index) {
        if (record.text_cue_valid[index] &&
            time_in_range(playback_time,
                          record.text_cue_start_times[index],
                          record.text_cue_end_times[index])) {
            return static_cast<uint64_t>(index + 1u);
        }
    }
    return kNoTextCueLeaseSlot;
}

}  // namespace

KivoP8ApiStatus acquire_frame_lease(
    PlaneStateRecord& record,
    const KivoSubtitleFrameBuildRequest& request,
    KivoSubtitleFrameLease* const out_lease) noexcept {
    *out_lease = KivoSubtitleFrameLease{};
    out_lease->snapshot_revision = record.snapshot_revision;
    out_lease->generation.timeline.playback_revision = request.playback_revision;
    out_lease->generation.timeline.graph_snapshot_revision = request.graph_snapshot_revision;
    out_lease->generation.timeline.seek_epoch = request.seek_epoch;

    if (record.state == KivoSubtitlePlaneState_Closed) {
        out_lease->status = KivoSubtitleFrameLeaseStatus_Closed;
        out_lease->error = KivoSubtitlePlaneError_ClosedSubtitlePlane;
        return KivoP8ApiStatus_Ok;
    }
    if (!plane_state_allows_frame_build(record.state)) {
        out_lease->status = KivoSubtitleFrameLeaseStatus_Empty;
        out_lease->error = KivoSubtitlePlaneError_SubtitleTimelineNotStarted;
        return KivoP8ApiStatus_Ok;
    }
    if (frame_hazard_retry_exhausted(record, request)) {
        out_lease->status = KivoSubtitleFrameLeaseStatus_Error;
        out_lease->error = KivoSubtitlePlaneError_SubtitleFrameAcquireContention;
        return KivoP8ApiStatus_NotReady;
    }
    if (eos_epsilon_expired(record, request)) {
        out_lease->status = KivoSubtitleFrameLeaseStatus_Empty;
        out_lease->error = KivoSubtitlePlaneError_SubtitleTimelineTemporarilyUnavailable;
        return KivoP8ApiStatus_Ok;
    }
    if (record.active_bitmap_packet &&
        time_in_range(request.playback_time, record.active_start_time, record.active_end_time)) {
        out_lease->status = KivoSubtitleFrameLeaseStatus_Empty;
        out_lease->error = KivoSubtitlePlaneError_SubtitleFrameUnavailableByPolicy;
        return KivoP8ApiStatus_Ok;
    }
    const uint64_t text_cue_slot = active_text_cue_slot(record, request.playback_time);
    if (text_cue_slot == kNoTextCueLeaseSlot) {
        out_lease->status = KivoSubtitleFrameLeaseStatus_Empty;
        out_lease->error = KivoSubtitlePlaneError_SubtitleTimelineTemporarilyUnavailable;
        return KivoP8ApiStatus_Ok;
    }
    if (record.metrics.active_frame_lease_count >= active_limit(record)) {
        out_lease->status = KivoSubtitleFrameLeaseStatus_PoolExhausted;
        out_lease->error = KivoSubtitlePlaneError_SubtitleFramePoolExhausted;
        record.metrics.frame_pool_exhausted_count += 1;
        return KivoP8ApiStatus_Ok;
    }
    const uint64_t lease_id = ++record.next_frame_lease_id;
    const uint64_t slot = record.metrics.active_frame_lease_count++;
    record.active_frame_lease_ids[slot] = lease_id;
    record.active_frame_lease_text_cue_slots[slot] = text_cue_slot;
    out_lease->frame_id.value = ++record.next_frame_id;
    out_lease->lease_id = lease_id;
    out_lease->lease_generation = 1;
    out_lease->frame = reinterpret_cast<const void*>(static_cast<uintptr_t>(lease_id));
    out_lease->status = KivoSubtitleFrameLeaseStatus_Ok;
    return KivoP8ApiStatus_Ok;
}

KivoP8ApiStatus release_frame_lease(
    PlaneStateRecord& record,
    const KivoSubtitleFrameLease& lease) noexcept {
    if (lease.lease_id == 0) {
        return KivoP8ApiStatus_Ok;
    }
    for (uint64_t index = 0; index < record.metrics.active_frame_lease_count; ++index) {
        if (record.active_frame_lease_ids[index] == lease.lease_id) {
            const uint64_t last = record.metrics.active_frame_lease_count - 1;
            record.active_frame_lease_ids[index] = record.active_frame_lease_ids[last];
            record.active_frame_lease_text_cue_slots[index] =
                record.active_frame_lease_text_cue_slots[last];
            record.active_frame_lease_ids[last] = 0;
            record.active_frame_lease_text_cue_slots[last] = 0;
            record.metrics.active_frame_lease_count = last;
            return KivoP8ApiStatus_Ok;
        }
    }
    return KivoP8ApiStatus_InvalidArgument;
}

}  // namespace kivo::video::subtitle_plane::runtime
