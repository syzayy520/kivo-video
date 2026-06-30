#include "video/playback_graph/snapshot/snapshot_store.hpp"

namespace kivo::video::playback_graph::runtime {

GraphSnapshotRevision SnapshotStore::publish(const PlaybackSessionSnapshot& snapshot) noexcept {
    auto stored = snapshot;
    stored.graph_snapshot_revision.value =
        snapshots_.empty() ? 1 : snapshots_.back().graph_snapshot_revision.value + 1;
    snapshots_.push_back(stored);
    audio_.selected = true;
    audio_.queued_duration_ms = 120;
    audio_.queued_bytes = 4096;
    video_.selected = true;
    video_.queued_frames = 3;
    video_.queued_duration_ms = 120;
    clock_.valid = true;
    clock_.estimate_ms = 120;
    clock_.error_bound_ms = 2;
    return stored.graph_snapshot_revision;
}

PlaybackSessionSnapshot SnapshotStore::latest_session_snapshot() const noexcept {
    return snapshots_.empty() ? PlaybackSessionSnapshot{} : snapshots_.back();
}

SnapshotQueryResult<AudioQueueSnapshot> SnapshotStore::query_audio_queue() const noexcept {
    return SnapshotQueryResult<AudioQueueSnapshot>{audio_, SnapshotQueryError::None};
}

SnapshotQueryResult<VideoQueueSnapshot> SnapshotStore::query_video_queue() const noexcept {
    return SnapshotQueryResult<VideoQueueSnapshot>{video_, SnapshotQueryError::None};
}

SnapshotQueryResult<ClockSnapshot> SnapshotStore::query_clock() const noexcept {
    return SnapshotQueryResult<ClockSnapshot>{clock_, SnapshotQueryError::None};
}

void SnapshotStore::set_clock_estimate(std::int64_t estimate_ms, bool valid) noexcept {
    clock_.estimate_ms = estimate_ms;
    clock_.valid = valid;
}

std::size_t SnapshotStore::retained_snapshot_count() const noexcept {
    return snapshots_.size();
}

}  // namespace kivo::video::playback_graph::runtime
