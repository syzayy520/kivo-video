#include "video/subtitle_plane/abi/api_function_declarations.hpp"

#include "video/subtitle_plane/core/plane_store.hpp"
#include "video/subtitle_plane/frame/frame_lease_pool.hpp"
#include "video/subtitle_plane/threading/cancellation_poll.hpp"
#include "video/subtitle_plane/timeline/timeline_revision.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus KIVO_P8_CALL api_build_frame(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleFrameBuildRequest* const request,
    KivoSubtitleFrameLease* const out_lease) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    if (request == nullptr || out_lease == nullptr) {
        return KivoP8ApiStatus_NullPointer;
    }
    uint32_t cancelled = 0;
    const KivoP8ApiStatus cancel_status = poll_cancelled(request->cancellation, &cancelled);
    if (cancel_status != KivoP8ApiStatus_Ok || cancelled != 0u) {
        return KivoP8ApiStatus_NotReady;
    }
    if (compare_playback_revision(*record, request->playback_revision) == RevisionRelation::Stale) {
        *out_lease = KivoSubtitleFrameLease{};
        out_lease->snapshot_revision = record->snapshot_revision;
        out_lease->status = KivoSubtitleFrameLeaseStatus_Error;
        out_lease->error = KivoSubtitlePlaneError_SubtitleTimelineInconsistent;
        return KivoP8ApiStatus_Failed;
    }
    return acquire_frame_lease(*record, *request, out_lease);
}

KivoP8ApiStatus KIVO_P8_CALL api_release_frame(
    const KivoSubtitlePlaneHandle plane,
    const KivoSubtitleFrameLease* const lease) KIVO_P8_NOEXCEPT {
    PlaneStateRecord* const record = find_plane_record(plane);
    if (record == nullptr) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    return lease == nullptr ? KivoP8ApiStatus_NullPointer : release_frame_lease(*record, *lease);
}

}  // namespace kivo::video::subtitle_plane::runtime
