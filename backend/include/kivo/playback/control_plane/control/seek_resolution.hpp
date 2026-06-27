#pragma once

#include <optional>
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"

namespace kivo::playback::control_plane::control {

/// SeekResolutionKind enumerates seek resolution outcomes.
enum class SeekResolutionKind
{
    Exact,
    Approximate,
    KeyframeHinted,
    SourceOffsetOnly,
    Unsupported
};

/// SeekResolution defines seek resolution result.
/// control_plane_warmup_hint_ms is read warmup suggestion, NOT codec/keyframe exact preroll.
/// P4 does NOT parse keyframe, does NOT promise codec exact seek.
struct SeekResolution
{
    SeekResolutionKind kind{SeekResolutionKind::Approximate};
    std::optional<primitives::MediaTimeMs> resolved_position_ms{};
    std::optional<primitives::SourceOffset> resolved_source_offset{};
    bool requires_downstream_decode_preroll{false};
    primitives::Milliseconds control_plane_warmup_hint_ms{};
};

} // namespace kivo::playback::control_plane::control
