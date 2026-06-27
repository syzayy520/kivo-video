#pragma once

#include <optional>
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"

namespace kivo::playback::control_plane::handoff {

/// InitialTimeRange is a read-only time range view from P3 probe.
struct InitialTimeRange
{
    primitives::MediaTimeMs begin{};
    primitives::MediaTimeMs end{};
};

/// P3ProbeEvidenceView is a read-only view of P3 ProbeEvidence.
/// Sole authority: handoff/p3_probe_evidence_view.hpp
struct P3ProbeEvidenceView
{
    bool has_duration{false};
    primitives::Milliseconds duration_ms{};
    bool is_seekable{false};
    bool has_reliable_byte_to_time_mapping{false};
    bool is_live_like{false};
    primitives::BitsPerSecond estimated_bitrate_bps{};
    std::optional<primitives::SourceRange> source_range{};
    std::optional<InitialTimeRange> initial_time_range{};
    bool encrypted_hint{false};
};

} // namespace kivo::playback::control_plane::handoff
