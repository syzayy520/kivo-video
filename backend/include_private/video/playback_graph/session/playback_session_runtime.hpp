#pragma once

#include "kivo/video/playback_graph/command_lifecycle_snapshot.hpp"
#include "kivo/video/playback_graph/command_token.hpp"
#include "kivo/video/playback_graph/playback_graph_observer.hpp"
#include "kivo/video/playback_graph/playback_graph_policy.hpp"
#include "kivo/video/playback_graph/playback_session.hpp"
#include "kivo/video/playback_graph/playback_session_snapshot.hpp"
#include "video/playback_graph/command/command_lifetime_registry.hpp"
#include "video/playback_graph/command/pending_transport_intent.hpp"
#include "video/playback_graph/p4_bridge/p4_state_sync_adapter.hpp"
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

    [[nodiscard]] PlaybackSessionSnapshot snapshot() const noexcept;
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

    PlaybackGraphPolicy policy_{};
    PlaybackSessionId session_id_{1};
    PlaybackGraphId graph_id_{1};
    PlaybackGraphStateMachine state_machine_{};
    P4StateSyncAdapter p4_adapter_{};
    CommandLifetimeRegistry command_registry_;
    PendingTransportIntent pending_transport_intent_{};
    SubscriptionToken next_subscription_token_{1};
};

}  // namespace kivo::video::playback_graph::runtime
