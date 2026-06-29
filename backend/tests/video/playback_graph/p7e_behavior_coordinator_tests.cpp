#include "video/playback_graph/coordination/av_sync_micro_adjust.hpp"
#include "video/playback_graph/coordination/buffering_predicates.hpp"
#include "video/playback_graph/coordination/eos_drain_coordinator.hpp"
#include "video/playback_graph/coordination/protected_content_policy.hpp"
#include "video/playback_graph/coordination/seek_coordinator.hpp"
#include "video/playback_graph/coordination/system_resume_coordinator.hpp"
#include "video/playback_graph/coordination/track_switch_coordinator.hpp"
#include "video/playback_graph/recovery/recovery_replay_behavior.hpp"
#include "video/playback_graph/resources/resource_retire_recycle.hpp"

using namespace kivo::video::playback_graph;
using namespace kivo::video::playback_graph::runtime;

namespace {

[[nodiscard]] bool verify_buffering_predicates() {
    BufferingInputs av{};
    av.audio_ready = true;
    av.video_ready = true;
    if (!buffering_ready(av)) {
        return false;
    }
    BufferingInputs audio_only{};
    audio_only.video_selected = false;
    audio_only.audio_ready = true;
    if (!buffering_ready(audio_only)) {
        return false;
    }
    BufferingInputs video_only{};
    video_only.audio_selected = false;
    video_only.video_ready = true;
    if (!buffering_ready(video_only)) {
        return false;
    }
    BufferingInputs live{};
    live.live_source = true;
    live.underflow_duration_ms = 50;
    live.debounce_ms = 100;
    if (rebuffering_needed(live)) {
        return false;
    }
    live.underflow_duration_ms = 3000;
    return stalled_predicate(live);
}

[[nodiscard]] bool verify_seek_coordinator() {
    SeekCoordinator coordinator{};
    SeekPlan invalid{};
    invalid.target_timeline_ms = -1;
    if (coordinator.run_fake_seek(invalid).error != PlaybackGraphError::InvalidSeekTarget) {
        return false;
    }
    SeekPlan plan{};
    plan.target_timeline_ms = 1000;
    const auto outcome = coordinator.run_fake_seek(plan);
    return outcome.accepted && outcome.logical_complete && outcome.background_recycle_pending &&
           outcome.phase_count == 14 && outcome.clock_reset &&
           outcome.generation.state == GenerationStampState::Active &&
           outcome.playback_revision.value != 0 && outcome.graph_snapshot_revision.value != 0;
}

[[nodiscard]] bool verify_resource_retire() {
    ResourceRetireRecycle retire{};
    const auto id = retire.decide_retire();
    if (retire.performs_physical_recycle()) {
        return false;
    }
    if (!retire.mark_timeout(id)) {
        return false;
    }
    if (retire.acknowledge_recycle(id)) {
        return false;
    }
    if (retire.late_ack_discard_count() != 1) {
        return false;
    }
    const auto abandoned = retire.decide_retire();
    retire.abandon_after_close(abandoned);
    return retire.state(abandoned) == RetireSetState::AbandonedAfterClose;
}

[[nodiscard]] bool verify_eos_drain() {
    EosDrainCoordinator drain{};
    EosDrainPlan plan{};
    const auto success = drain.drain_fake(plan, false, false);
    if (!success.complete || success.required_bits != 4 || success.completed_bits != 4) {
        return false;
    }
    EosDrainPlan audio_only{};
    audio_only.video_selected = false;
    audio_only.presentation_active = false;
    if (drain.drain_fake(audio_only, false, false).required_bits != 2) {
        return false;
    }
    if (!drain.drain_fake(plan, true, false).last_frame_timeout) {
        return false;
    }
    return drain.drain_fake(plan, false, true).output_drained_timeout;
}

[[nodiscard]] bool verify_track_switch() {
    TrackSwitchCoordinator coordinator{TrackState{}};
    AudioTrackSwitchRequest same_audio{};
    same_audio.track_id = 1;
    if (coordinator.switch_audio(same_audio, false).kind != TrackSwitchResultKind::NoOp) {
        return false;
    }
    AudioTrackSwitchRequest audio{};
    audio.track_id = 2;
    const auto audio_outcome = coordinator.switch_audio(audio, false);
    if (!audio_outcome.audio_changed || !audio_outcome.micro_adjust_refreshed) {
        return false;
    }
    VideoTrackSwitchRequest video{};
    video.track_id = 3;
    if (!coordinator.switch_video(video, false).video_changed) {
        return false;
    }
    if (coordinator.switch_video(video, true).kind != TrackSwitchResultKind::RejectedSeekPriority) {
        return false;
    }
    AvTrackSetSwitchRequest av{};
    av.audio_track_id = 4;
    av.video_track_id = 4;
    av.atomicity = TrackSwitchAtomicity::AtomicAllOrNothing;
    if (coordinator.switch_av(av, false, true, true).kind != TrackSwitchResultKind::RolledBack) {
        return false;
    }
    av.atomicity = TrackSwitchAtomicity::BestEffortPartialAllowed;
    return coordinator.switch_av(av, false, true, false).kind == TrackSwitchResultKind::Degraded;
}

[[nodiscard]] bool verify_recovery_replay() {
    RecoveryReplayBehavior recovery{};
    RecoveryReplayPlan plan{};
    plan.seek = true;
    plan.transport = true;
    plan.track_switch = true;
    const auto invalid_seek =
        recovery.replay(plan, true, false, true, TransportIntentKind::Resume);
    if (!invalid_seek.paused_best_effort ||
        invalid_seek.replayed_transport_intent != TransportIntentKind::Resume) {
        return false;
    }
    const auto rollback_failed =
        recovery.replay(plan, false, true, false, TransportIntentKind::Pause);
    return rollback_failed.graph_failed && rollback_failed.error == PlaybackGraphError::RecoveryFailed;
}

[[nodiscard]] bool verify_av_sync() {
    AvSyncMicroAdjust sync{};
    AvSyncInputs inputs{};
    inputs.drift_ms = 60;
    const auto decision = sync.decide(inputs, true);
    if (decision.master != ClockMaster::Audio || !decision.micro_adjust_allowed ||
        !decision.capability_refreshed) {
        return false;
    }
    inputs.p6_micro_adjust_supported = false;
    const auto shrink = sync.decide(inputs, true);
    if (!shrink.rollback_required || !shrink.capability_changed) {
        return false;
    }
    inputs.p6_micro_adjust_capability_known = false;
    return !sync.decide(inputs, false).micro_adjust_allowed;
}

[[nodiscard]] bool verify_system_resume() {
    SystemResumeCoordinator resume{};
    SystemResumePlan plan{};
    if (!resume.resume_fake(plan, false, false).accepted) {
        return false;
    }
    if (!resume.resume_fake(plan, true, false).reconnect_timeout) {
        return false;
    }
    plan.source_reconnect_required = false;
    plan.source_reopen_required = true;
    if (!resume.resume_fake(plan, false, true).reopen_timeout) {
        return false;
    }
    plan.source_reopen_required = false;
    plan.dvr_window_shifted = true;
    plan.seek_target_expired = true;
    plan.timeline_discontinuity = true;
    const auto shifted = resume.resume_fake(plan, false, false);
    return shifted.expired_seek_target && shifted.timeline_discontinuity_evidence;
}

[[nodiscard]] bool verify_protected_policy() {
    ProtectedContentPolicy policy{};
    ProtectedContentInputs clear{};
    if (!policy.decide(clear).playback_allowed) {
        return false;
    }
    ProtectedContentInputs protected_missing_output{};
    protected_missing_output.protected_content = true;
    protected_missing_output.secure_output_available = false;
    protected_missing_output.stale_frame_visible = true;
    const auto decision = policy.decide(protected_missing_output);
    return !decision.playback_allowed && decision.evidence_redacted &&
           decision.screenshots_disabled && decision.thumbnails_disabled &&
           decision.stale_frame_blocked && decision.resource_retire_blocking;
}

}  // namespace

int main() {
    if (!verify_buffering_predicates()) {
        return 1;
    }
    if (!verify_seek_coordinator()) {
        return 2;
    }
    if (!verify_resource_retire()) {
        return 3;
    }
    if (!verify_eos_drain()) {
        return 4;
    }
    if (!verify_track_switch()) {
        return 5;
    }
    if (!verify_recovery_replay()) {
        return 6;
    }
    if (!verify_av_sync()) {
        return 7;
    }
    if (!verify_system_resume()) {
        return 8;
    }
    if (!verify_protected_policy()) {
        return 9;
    }
    return 0;
}
