#pragma once

#include <optional>
#include <string>
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"

namespace kivo::playback::control_plane::handoff {

/// P3PlaybackHandoffRef is a read-only reference to P3 PlaybackHandoffContract.
/// P4 does NOT directly modify P3 handoff.
/// P4 only reads via P3-approved adapter.
struct P3PlaybackHandoffRef
{
    const void* opaque_p3_handoff{nullptr};
    primitives::U64 p3_schema_version{0};
};

/// P3SourceSessionRef references P3 SourceSession.
struct P3SourceSessionRef { const void* ref{nullptr}; };

/// P3ReaderFactoryRef references P3 reader factory.
struct P3ReaderFactoryRef { const void* ref{nullptr}; };

/// P3CandidateSetRef references P3 PlaybackCandidateSet.
struct P3CandidateSetRef { const void* ref{nullptr}; };

/// P3SelectedCandidateRef references P3 selected candidate (read-only).
/// P4 must NOT change provider decision.
struct P3SelectedCandidateRef { const void* ref{nullptr}; };

/// P3RuntimeRevisionView is a read-only view of P3 RuntimeRevisionResult.
struct P3RuntimeRevisionView
{
    std::string trigger{};
    std::string action{};
    bool p4_action_hint{false};
};

/// P3HandoffEvidenceView records import evidence and conflicts.
struct P3HandoffEvidenceView
{
    bool import_successful{false};
    std::string conflict_notes{};
};

} // namespace kivo::playback::control_plane::handoff
