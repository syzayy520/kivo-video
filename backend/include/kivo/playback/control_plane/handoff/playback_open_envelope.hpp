#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/handoff/p3_handoff_ref.hpp"
#include "kivo/playback/control_plane/handoff/p3_probe_evidence_view.hpp"
#include "kivo/playback/control_plane/handoff/stream_layout_hint.hpp"
#include "kivo/playback/control_plane/handoff/interleave_profile.hpp"
#include "kivo/playback/control_plane/runtime/runtime_policy_update.hpp"

namespace kivo::playback::control_plane::handoff {

/// PlaybackOpenEnvelope is the open-time policy envelope from P3 handoff.
/// P4 reads all runtime policies from this envelope; it must NOT modify
/// selected_candidate or any P3 handoff field.
/// Policy fields not yet backed by a dedicated P4 contract are carried as
/// opaque const pointers (filled by later gates P4G-031+).
struct PlaybackOpenEnvelope
{
    revision::SessionId session_id{};
    P3SourceSessionRef source_session{};
    P3ReaderFactoryRef reader_factory{};
    P3CandidateSetRef candidate_set{};
    P3SelectedCandidateRef selected_candidate{};
    const void* execution_context{nullptr};
    const void* buffer_requirement{nullptr};
    const InterleaveProfile* interleave_profile{nullptr};
    const StreamLayoutHint* stream_layout_hint{nullptr};
    const void* read_throttle_policy{nullptr};
    const void* low_speed_policy{nullptr};
    const void* reader_stall_policy{nullptr};
    const void* shutdown_policy{nullptr};
    const void* pause_buffer_policy{nullptr};
    const void* playback_rate_policy{nullptr};
    const void* live_stream_policy{nullptr};
    const void* track_switch_policy{nullptr};
    const void* resource_budget{nullptr};
    const void* idle_timeout_policy{nullptr};
    const void* failure_policy{nullptr};
    runtime::RuntimeRevisionHook runtime_revision_hook{};
    const P3ProbeEvidenceView* probe_evidence{nullptr};
    const void* recovery_plan{nullptr};
    P3HandoffEvidenceView handoff_evidence{};
};

} // namespace kivo::playback::control_plane::handoff
