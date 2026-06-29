#include "video/playback_graph/coordination/system_resume_coordinator.hpp"

namespace kivo::video::playback_graph::runtime {

SystemResumeOutcome SystemResumeCoordinator::resume_fake(const SystemResumePlan& plan,
                                                         bool inject_reconnect_timeout,
                                                         bool inject_reopen_timeout) noexcept {
    SystemResumeOutcome outcome{};
    if (plan.source_reconnect_required && inject_reconnect_timeout) {
        outcome.reconnect_timeout = true;
        outcome.error = PlaybackGraphError::RecoveryFailed;
        return outcome;
    }
    if (plan.source_reopen_required && inject_reopen_timeout) {
        outcome.reopen_timeout = true;
        outcome.error = PlaybackGraphError::RecoveryFailed;
        return outcome;
    }
    outcome.expired_seek_target = plan.dvr_window_shifted && plan.seek_target_expired;
    outcome.timeline_discontinuity_evidence = plan.timeline_discontinuity;
    outcome.accepted = !outcome.expired_seek_target;
    outcome.error = outcome.expired_seek_target ? PlaybackGraphError::InvalidSeekTarget
                                                : PlaybackGraphError::None;
    return outcome;
}

}  // namespace kivo::video::playback_graph::runtime
