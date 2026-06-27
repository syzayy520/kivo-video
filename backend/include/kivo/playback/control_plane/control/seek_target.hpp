#pragma once

#include <optional>
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"

namespace kivo::playback::control_plane::control {

/// SeekTargetType enumerates seek target types.
enum class SeekTargetType
{
    TimelinePosition,
    SourceOffset,
    RelativeDelta
};

/// SeekAccuracy enumerates seek accuracy levels.
enum class SeekAccuracy
{
    FastApproximate,
    KeyframeHinted,
    ExactRequested,
    SourceNative
};

/// SeekTarget defines a seek request.
/// TimelinePosition must provide absolute_position_ms (VOD >= 0).
/// RelativeDelta must provide relative_delta_ms.
/// SourceOffset must provide source_offset.
struct SeekTarget
{
    SeekTargetType type{SeekTargetType::TimelinePosition};
    std::optional<primitives::MediaTimeMs> absolute_position_ms{};
    std::optional<primitives::MediaDeltaMs> relative_delta_ms{};
    std::optional<primitives::SourceOffset> source_offset{};
    SeekAccuracy accuracy{SeekAccuracy::FastApproximate};
};

} // namespace kivo::playback::control_plane::control
