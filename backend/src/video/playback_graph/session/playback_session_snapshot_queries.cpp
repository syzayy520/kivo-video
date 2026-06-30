#include "video/playback_graph/session/playback_session_runtime.hpp"

#include <algorithm>

namespace kivo::video::playback_graph::runtime {

PlaybackSessionSnapshot PlaybackSessionRuntime::snapshot() const noexcept {
    PlaybackSessionSnapshot snapshot{};
    snapshot.session_id = session_id_;
    snapshot.graph_id = graph_id_;
    snapshot.state = state_machine_.state();
    snapshot.playback_revision = p4_adapter_.current_playback_revision();
    snapshot.graph_snapshot_revision = state_machine_.snapshot_revision();
    snapshot.retained_command_count =
        static_cast<std::uint32_t>(command_registry_.active_record_count() +
                                   command_registry_.tombstone_count());
    snapshot.late_event_discard_count = command_registry_.late_event_discard_count();
    snapshot.closed = state_machine_.state() == PlaybackGraphState::Closed;
    return snapshot;
}

PlaybackTimelineSnapshot PlaybackSessionRuntime::query_timeline() const noexcept {
    PlaybackTimelineSnapshot timeline{};
    if (state_machine_.state() == PlaybackGraphState::Closed ||
        state_machine_.state() == PlaybackGraphState::NotCreated) {
        return timeline;
    }

    timeline.position_ms = position_ms_;
    timeline.duration_ms = duration_ms_;
    timeline.seek_in_progress = seek_in_progress_;
    timeline.last_seek_target_ms = last_seek_target_ms_;
    timeline.valid = duration_ms_ > 0;

    const auto audio = snapshot_store_.query_audio_queue();
    const auto video = snapshot_store_.query_video_queue();
    if (audio.ok() && video.ok()) {
        const auto buffered_end = position_ms_ + static_cast<std::int64_t>(audio.value.queued_duration_ms);
        timeline.buffered_range.start_ms = position_ms_;
        timeline.buffered_range.end_ms =
            duration_ms_ > 0 ? std::min(buffered_end, duration_ms_) : buffered_end;
    }

    return timeline;
}

SnapshotQueryResult<ClockSnapshot> PlaybackSessionRuntime::query_clock() const noexcept {
    auto clock = snapshot_store_.query_clock();
    if (clock.ok()) {
        clock.value.estimate_ms = position_ms_;
        clock.value.valid = duration_ms_ > 0;
    }
    return clock;
}

SnapshotQueryResult<AudioQueueSnapshot> PlaybackSessionRuntime::query_audio_queue() const noexcept {
    return snapshot_store_.query_audio_queue();
}

SnapshotQueryResult<VideoQueueSnapshot> PlaybackSessionRuntime::query_video_queue() const noexcept {
    return snapshot_store_.query_video_queue();
}

}  // namespace kivo::video::playback_graph::runtime