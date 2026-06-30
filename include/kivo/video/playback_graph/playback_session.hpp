#pragma once

#include <cstdint>

#include "kivo/video/playback_graph/binding/video_surface_binding.hpp"
#include "kivo/video/playback_graph/clock/playback_timeline_snapshot.hpp"
#include "kivo/video/playback_graph/command_lifecycle_snapshot.hpp"
#include "kivo/video/playback_graph/command_token.hpp"
#include "kivo/video/playback_graph/evidence/diagnostics_summary.hpp"
#include "kivo/video/playback_graph/playback_graph_event.hpp"
#include "kivo/video/playback_graph/playback_graph_ids.hpp"
#include "kivo/video/playback_graph/playback_graph_observer.hpp"
#include "kivo/video/playback_graph/playback_graph_policy.hpp"
#include "kivo/video/playback_graph/playback_session_snapshot.hpp"
#include "kivo/video/playback_graph/recovery/recovery_action_request.hpp"
#include "kivo/video/playback_graph/seek_request.hpp"
#include "kivo/video/playback_graph/snapshot_query_result.hpp"
#include "kivo/video/playback_graph/track_switch_request.hpp"

namespace kivo::video::playback_graph {

struct OpenRequest {
    std::uint64_t source_id{0};
    std::uint64_t initial_audio_track_id{0};
    std::uint64_t initial_video_track_id{0};
    bool audio_selected{true};
    bool video_selected{true};
};

class PlaybackSession {
public:
    PlaybackSession();
    explicit PlaybackSession(const PlaybackGraphPolicy& policy);
    ~PlaybackSession();

    PlaybackSession(const PlaybackSession&) = delete;
    PlaybackSession& operator=(const PlaybackSession&) = delete;
    PlaybackSession(PlaybackSession&& other) noexcept;
    PlaybackSession& operator=(PlaybackSession&& other) noexcept;

    CommandToken open(const OpenRequest& request) noexcept;
    CommandToken start() noexcept;
    CommandToken pause() noexcept;
    CommandToken resume() noexcept;
    CommandToken seek(const SeekRequest& request) noexcept;
    CommandToken switch_audio_track(const AudioTrackSwitchRequest& request) noexcept;
    CommandToken switch_video_track(const VideoTrackSwitchRequest& request) noexcept;
    CommandToken switch_av_track_set(const AvTrackSetSwitchRequest& request) noexcept;
    CommandToken close() noexcept;
    CommandToken attach_video_surface(const VideoSurfaceBindingRequest& request) noexcept;
    CommandToken detach_video_surface() noexcept;
    CommandToken release_video_surface() noexcept;
    CommandToken retry(const RecoveryActionRequest& request) noexcept;
    CommandToken reopen(const RecoveryActionRequest& request) noexcept;
    PlaybackSessionSnapshot snapshot() const noexcept;
    PlaybackTimelineSnapshot query_timeline() const noexcept;
    VideoSurfaceSnapshot query_video_surface() const noexcept;
    DiagnosticsSummary query_diagnostics_summary() const noexcept;
    SnapshotQueryResult<ClockSnapshot> query_clock() const noexcept;
    SnapshotQueryResult<AudioQueueSnapshot> query_audio_queue() const noexcept;
    SnapshotQueryResult<VideoQueueSnapshot> query_video_queue() const noexcept;
    CommandLifecycleSnapshot query_command(PlaybackCommandId id) const noexcept;
    SubscriptionToken subscribe_events(GraphObserverHandle observer) noexcept;
    void unsubscribe(SubscriptionToken token) noexcept;
    CommandToken inject_system_event(const SystemEvent& event) noexcept;

private:
    void* impl_{nullptr};
};

}  // namespace kivo::video::playback_graph
