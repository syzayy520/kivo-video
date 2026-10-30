#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/handoff/p3_handoff_ref.hpp"
#include "kivo/playback/control_plane/timeline/playback_timeline_state.hpp"

namespace kivo::playback::control_plane::output_boundary {

/// SubtitleAvailability describes subtitle availability state.
struct SubtitleAvailability
{
    bool has_subtitle_track{false};
    bool downloaded{false};
    bool burn_in_required{false};
};

/// SubtitlePipelineInputPlan is the P4 -> P7 subtitle output boundary plan.
/// P4 must NOT output subtitle bitmap or UI event.
struct SubtitlePipelineInputPlan
{
    revision::SessionId session_id{};
    revision::StreamRevision stream_revision{};
    handoff::P3SelectedCandidateRef selected_candidate{};
    const timeline::PlaybackTimelineState* timeline_snapshot{nullptr};
    SubtitleAvailability subtitle_availability{};
};

} // namespace kivo::playback::control_plane::output_boundary
