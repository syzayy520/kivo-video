#pragma once

#include "kivo/video/playback_graph/playback_graph_error.hpp"

namespace kivo::video::playback_graph::runtime {

struct SystemResumePlan {
    bool source_reconnect_required{true};
    bool source_reopen_required{false};
    bool dvr_window_shifted{false};
    bool seek_target_expired{false};
    bool timeline_discontinuity{false};
};

struct SystemResumeOutcome {
    bool accepted{false};
    bool bounded_queries_only{true};
    bool reconnect_timeout{false};
    bool reopen_timeout{false};
    bool expired_seek_target{false};
    bool timeline_discontinuity_evidence{false};
    PlaybackGraphError error{PlaybackGraphError::None};
};

class SystemResumeCoordinator {
public:
    [[nodiscard]] SystemResumeOutcome resume_fake(const SystemResumePlan& plan,
                                                  bool inject_reconnect_timeout,
                                                  bool inject_reopen_timeout) noexcept;
};

}  // namespace kivo::video::playback_graph::runtime
