#include "video/playback_graph/coordination/seek_coordinator.hpp"

namespace kivo::video::playback_graph::runtime {

SeekOutcome SeekCoordinator::run_fake_seek(const SeekPlan& plan) noexcept {
    SeekOutcome outcome{};
    if (plan.target_timeline_ms < 0) {
        outcome.error = PlaybackGraphError::InvalidSeekTarget;
        return outcome;
    }

    outcome.accepted = true;
    outcome.phases = {SeekPhase::ValidateTarget,
                      SeekPhase::FreezeTransport,
                      SeekPhase::SourceSeek,
                      SeekPhase::DemuxSeek,
                      SeekPhase::AudioFlushBegin,
                      SeekPhase::VideoFlushBegin,
                      SeekPhase::AudioSeek,
                      SeekPhase::VideoSeek,
                      SeekPhase::ClockReset,
                      SeekPhase::PublishGeneration,
                      SeekPhase::CommitP4Revision,
                      SeekPhase::PublishGraphSnapshot,
                      SeekPhase::ReleaseBarrier,
                      SeekPhase::CompleteLogicalSeek};
    outcome.phase_count = 14;
    outcome.clock_reset = true;
    outcome.generation.tuple_id = next_generation_tuple_;
    outcome.generation.stream_stamp_id = next_generation_tuple_.value;
    outcome.generation.state = GenerationStampState::Active;
    next_generation_tuple_.value += 1;
    outcome.playback_revision = next_playback_revision_;
    next_playback_revision_.value += 1;
    outcome.graph_snapshot_revision = next_snapshot_revision_;
    next_snapshot_revision_.value += 1;
    outcome.logical_complete = true;
    outcome.background_recycle_pending = true;
    return outcome;
}

}  // namespace kivo::video::playback_graph::runtime
