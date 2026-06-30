#pragma once

#include "kivo/video/playback_graph/command_lifecycle_snapshot.hpp"
#include "kivo/video/playback_graph/command_token.hpp"
#include "kivo/video/playback_graph/playback_graph_observer.hpp"
#include "kivo/video/playback_graph/playback_graph_policy.hpp"
#include "kivo/video/playback_graph/playback_session.hpp"
#include "kivo/video/playback_graph/playback_session_snapshot.hpp"
#include "video/playback_graph/command/command_lifetime_registry.hpp"
#include "video/playback_graph/command/pending_transport_intent.hpp"
#include "video/playback_graph/coordination/recovery_replay_coordinator.hpp"
#include "video/playback_graph/p4_bridge/p4_state_sync_adapter.hpp"
#include "video/playback_graph/snapshot/snapshot_store.hpp"
#include "video/playback_graph/state/playback_graph_state_machine.hpp"

namespace kivo::video::playback_graph::runtime {

class PlaybackSessionRuntime {
public:
    explicit PlaybackSessionRuntime(const PlaybackGraphPolicy& policy);

    [[nodiscard]] CommandToken open(const OpenRequest& request) noexcept;
    [[nodiscard]] CommandToken start() noexcept;
    [[nodiscard]] CommandToken pause() noexcept;
    [[nodiscard]] CommandToken resume() noexcept;
    [[nodiscard]] CommandToken seek(const SeekRequest& request) noexcept;
    [[nodiscard]] CommandToken switch_audio_track(const AudioTrackSwitchRequest& request) noexcept;
    [[nodiscard]] CommandToken switch_video_track(const VideoTrackSwitchRequest& request) noexcept;
    [[nodiscard]] CommandToken switch_av_track_set(const AvTrackSetSwitchRequest& request) noexcept;
    [[nodiscard]] CommandToken close() noexcept;
    [[nodiscard]] CommandToken attach_video_surface(
        const VideoSurfaceBindingRequest& request) noexcept;
    [[nodiscard]] CommandToken detach_video_surface() noexcept;
    [[nodiscard]] CommandToken release_video_surface() noexcept;
    [[nodiscard]] CommandToken retry(const RecoveryActionRequest& request) noexcept;
    [[nodiscard]] CommandToken reopen(const RecoveryActionRequest& request) noexcept;

    [[nodiscard]] PlaybackSessionSnapshot snapshot() const noexcept;
    [[nodiscard]] PlaybackTimelineSnapshot query_timeline() const noexcept;
    [[nodiscard]] VideoSurfaceSnapshot query_video_surface() const noexcept;
    [[nodiscard]] DiagnosticsSummary query_diagnostics_summary() const noexcept;
    [[nodiscard]] SnapshotQueryResult<ClockSnapshot> query_clock() const noexcept;
    [[nodiscard]] SnapshotQueryResult<AudioQueueSnapshot> query_audio_queue() const noexcept;
    [[nodiscard]] SnapshotQueryResult<VideoQueueSnapshot> query_video_queue() const noexcept;
    [[nodiscard]] CommandLifecycleSnapshot query_command(PlaybackCommandId id) const noexcept;
    [[nodiscard]] SubscriptionToken subscribe_events(GraphObserverHandle observer) noexcept;
    void unsubscribe(SubscriptionToken token) noexcept;
    [[nodiscard]] CommandToken inject_system_event(const SystemEvent& event) noexcept;

private:
    [[nodiscard]] CommandToken accept_command(GraphCommandKind command,
                                              bool requires_sidecar) noexcept;
    [[nodiscard]] CommandToken reject(PlaybackGraphError error) const noexcept;
    void complete_if_accepted(CommandToken token,
                              CommandTerminalStatus terminal_status,
                              PlaybackGraphError error) noexcept;
    void publish_current_snapshot() noexcept;
    void sync_clock_store() noexcept;

    PlaybackGraphPolicy policy_{};
    PlaybackSessionId session_id_{1};
    PlaybackGraphId graph_id_{1};
    PlaybackGraphStateMachine state_machine_{};
    P4StateSyncAdapter p4_adapter_{};
    CommandLifetimeRegistry command_registry_;
    PendingTransportIntent pending_transport_intent_{};
    SubscriptionToken next_subscription_token_{1};
    SnapshotStore snapshot_store_{};
    RecoveryReplayCoordinator recovery_coordinator_{};
    std::uint64_t last_source_id_{0};
    std::int64_t duration_ms_{0};
    std::int64_t position_ms_{0};
    std::int64_t last_seek_target_ms_{0};
    bool seek_in_progress_{false};
    VideoSurfaceSnapshot video_surface_{};
};

}  // namespace kivo::video::playback_graph::runtime
