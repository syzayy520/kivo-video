#pragma once

#include <vector>

#include "kivo/video/playback_graph/playback_session_snapshot.hpp"
#include "kivo/video/playback_graph/snapshot_query_result.hpp"

namespace kivo::video::playback_graph::runtime {

class SnapshotStore {
public:
    GraphSnapshotRevision publish(const PlaybackSessionSnapshot& snapshot) noexcept;

    [[nodiscard]] PlaybackSessionSnapshot latest_session_snapshot() const noexcept;
    [[nodiscard]] SnapshotQueryResult<AudioQueueSnapshot> query_audio_queue() const noexcept;
    [[nodiscard]] SnapshotQueryResult<VideoQueueSnapshot> query_video_queue() const noexcept;
    [[nodiscard]] SnapshotQueryResult<ClockSnapshot> query_clock() const noexcept;
    [[nodiscard]] std::size_t retained_snapshot_count() const noexcept;

private:
    std::vector<PlaybackSessionSnapshot> snapshots_{};
    AudioQueueSnapshot audio_{};
    VideoQueueSnapshot video_{};
    ClockSnapshot clock_{};
};

}  // namespace kivo::video::playback_graph::runtime
