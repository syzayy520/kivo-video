#pragma once

#include <optional>
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"
#include "kivo/playback/control_plane/timeline/time_mapping_confidence.hpp"

namespace kivo::playback::control_plane::handoff {

/// StreamLayoutHint is P4 runtime layout policy authority.
/// ProbeEvidence is raw evidence. If conflict: StreamLayoutHint wins.
struct StreamLayoutHint
{
    bool has_known_duration{false};
    bool has_known_bitrate{false};
    bool has_reliable_byte_to_time_mapping{false};
    bool is_live_like{false};
    bool is_seekable{false};
    bool has_multiple_tracks{false};
    bool may_have_sparse_keyframes{false};
    primitives::Milliseconds duration_ms{};
    primitives::BitsPerSecond estimated_bitrate_bps{};
    timeline::TimeMappingConfidence time_mapping_confidence{timeline::TimeMappingConfidence::None};
    primitives::Milliseconds suggested_seek_warmup_ms{};
    primitives::Milliseconds max_reasonable_warmup_ms{};
    primitives::Bytes average_cluster_bytes{};
    primitives::Milliseconds average_cluster_duration_ms{};
};

} // namespace kivo::playback::control_plane::handoff
