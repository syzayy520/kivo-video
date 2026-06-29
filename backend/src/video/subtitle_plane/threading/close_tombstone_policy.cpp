#include "video/subtitle_plane/threading/close_tombstone_policy.hpp"

namespace kivo::video::subtitle_plane::runtime {

void apply_close_tombstones(PlaneStateRecord& record) noexcept {
    record.metrics.frame_lease_leak_count += record.metrics.active_frame_lease_count;
    record.metrics.snapshot_lease_leak_count += record.metrics.active_snapshot_lease_count;
    record.state = KivoSubtitlePlaneState_Closed;
    record.timeline = KivoSubtitleTimelineAvailability_Closed;
}

}  // namespace kivo::video::subtitle_plane::runtime
