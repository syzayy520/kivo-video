#pragma once

#include <cstdint>

#include "kivo/video/playback_graph/track_switch_request.hpp"
#include "kivo/video/playback_graph/playback_graph_error.hpp"

namespace kivo::video::playback_graph::runtime {

enum class TrackSwitchResultKind {
    Completed,
    NoOp,
    RejectedSeekPriority,
    RolledBack,
    Degraded
};

struct TrackState {
    std::uint64_t audio_track_id{1};
    std::uint64_t video_track_id{1};
    bool p6_micro_adjust_capability_known{true};
};

struct TrackSwitchOutcome {
    TrackSwitchResultKind kind{TrackSwitchResultKind::Completed};
    PlaybackGraphError error{PlaybackGraphError::None};
    bool audio_changed{false};
    bool video_changed{false};
    bool rollback_attempted{false};
    bool micro_adjust_refreshed{false};
};

class TrackSwitchCoordinator {
public:
    explicit TrackSwitchCoordinator(TrackState state);

    [[nodiscard]] TrackSwitchOutcome switch_audio(const AudioTrackSwitchRequest& request,
                                                  bool seek_in_progress) noexcept;
    [[nodiscard]] TrackSwitchOutcome switch_video(const VideoTrackSwitchRequest& request,
                                                  bool seek_in_progress) noexcept;
    [[nodiscard]] TrackSwitchOutcome switch_av(const AvTrackSetSwitchRequest& request,
                                               bool seek_in_progress,
                                               bool inject_video_failure,
                                               bool rollback_success) noexcept;

    [[nodiscard]] TrackState state() const noexcept;

private:
    TrackState state_{};
};

}  // namespace kivo::video::playback_graph::runtime
