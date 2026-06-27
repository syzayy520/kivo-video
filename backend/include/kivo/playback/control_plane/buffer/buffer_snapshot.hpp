#pragma once

#include <optional>
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"

namespace kivo::playback::control_plane::buffer {

/// BufferLevel enumerates buffer fill levels.
enum class BufferLevel
{
    Empty,
    Low,
    Moderate,
    High,
    Full
};

/// BufferFenceKind enumerates active fence kinds on the buffer.
enum class BufferFenceKind
{
    None,
    Seek,
    Flush,
    Stop,
    TrackSwitch,
    Recovery
};

/// BufferedTimeRange captures the buffered time span.
struct BufferedTimeRange
{
    primitives::MediaTimeMs begin{};
    primitives::MediaTimeMs end{};
};

/// BufferSnapshot is an immutable buffer state snapshot.
struct BufferSnapshot
{
    revision::RevisionStamp stamp{};
    primitives::Bytes buffered_bytes{};
    primitives::Bytes capacity_bytes{};
    std::optional<primitives::SourceRange> buffered_source_range{};
    std::optional<BufferedTimeRange> buffered_time_range{};
    BufferLevel level{BufferLevel::Empty};
    bool eof_seen{false};
    bool live_edge_seen{false};
    bool buffer_has_pending_discontinuity{false};
    BufferFenceKind fence_kind{BufferFenceKind::None};
};

} // namespace kivo::playback::control_plane::buffer
