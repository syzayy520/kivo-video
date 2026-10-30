#pragma once

#include <optional>
#include "kivo/playback/control_plane/timeline/time_mapping_confidence.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"

namespace kivo::playback::control_plane::timeline {

/// BufferedRange captures a buffered source/time span.
struct BufferedRange
{
    std::optional<primitives::SourceRange> buffered_source_range{};
    std::optional<primitives::MediaTimeMs> buffered_time_begin{};
    std::optional<primitives::MediaTimeMs> buffered_time_end{};
};

/// SourceTimelineState belongs to the current StreamRevision.
/// StreamRevision increment clears eof_seen/live_edge_seen unless new reader confirms.
/// Buffer flush clears buffer ranges.
/// Buffer flush does NOT preserve old revision markers.
struct SourceTimelineState
{
    std::optional<BufferedRange> buffered_range{};
    TimeMappingConfidence time_mapping_confidence{TimeMappingConfidence::None};
    bool eof_seen{false};
    bool live_edge_seen{false};
};

} // namespace kivo::playback::control_plane::timeline
