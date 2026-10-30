#pragma once

namespace kivo::playback::control_plane::timeline {

/// TimeMappingConfidence sole authority: timeline/time_mapping_confidence.hpp
enum class TimeMappingConfidence
{
    None,
    Estimated,
    P3Hinted,
    P5Observed,
    Stable
};

} // namespace kivo::playback::control_plane::timeline
