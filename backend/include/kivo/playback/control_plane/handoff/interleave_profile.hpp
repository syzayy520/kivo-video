#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::handoff {

/// ContainerInterleaveKind describes container interleave pattern.
enum class ContainerInterleaveKind
{
    Unknown,
    LowInterleave,
    NormalInterleave,
    HeavyInterleave,
    BurstClustered,
    FragmentedMp4Like,
    BlurayLike,
    LiveLike
};

/// InterleaveProfile describes readahead recommendations.
struct InterleaveProfile
{
    ContainerInterleaveKind kind{ContainerInterleaveKind::Unknown};
    bool audio_video_interleaved{false};
    bool sparse_keyframe_risk{false};
    bool bursty_cluster_risk{false};
    bool unknown_time_mapping{false};
    primitives::Milliseconds recommended_readahead_ms{};
    primitives::Milliseconds max_safe_readahead_ms{};
    primitives::Bytes recommended_readahead_bytes{};
    primitives::Bytes max_safe_readahead_bytes{};
};

} // namespace kivo::playback::control_plane::handoff
