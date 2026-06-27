#pragma once

#include <optional>
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/memory/read_only_memory_chain.hpp"
#include "kivo/playback/control_plane/buffer/stream_end_kind.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"

namespace kivo::playback::control_plane::buffer {

/// BufferItemKind enumerates buffer item kinds.
/// BufferEngine maintains one ordered item stream.
/// Data, discontinuity, EOF, LiveEdge all preserve order.
/// Marker occupies logical queue position.
/// Marker may be flushed by revision barrier.
/// Marker is NOT silently dropped.
enum class BufferItemKind
{
    Data,
    Discontinuity,
    EndMarker
};

/// BufferDiscontinuity marks a gap in the ordered stream.
struct BufferDiscontinuity
{
    primitives::U32 reason_code{0};
    revision::SessionGeneration generation{};
};

/// BufferEndMarker marks stream end within the ordered stream.
struct BufferEndMarker
{
    StreamEndKind kind{StreamEndKind::None};
    revision::RevisionStamp stamp{};
    std::optional<primitives::SourceOffset> final_offset{};
    std::optional<primitives::MediaTimeMs> projected_position_ms{};
    primitives::MonotonicTimestamp observed_at{};
};

/// BufferItem is a single ordered item in the buffer stream.
/// ReaderReadResult::DataAndEndMarker must be split into
/// BufferItem(Data) + BufferItem(EndMarker).
struct BufferItem
{
    BufferItemKind kind{BufferItemKind::Data};
    revision::RevisionStamp stamp{};
    std::optional<memory::ReadOnlyMemoryChain> chunk{};
    std::optional<BufferDiscontinuity> discontinuity{};
    std::optional<BufferEndMarker> end_marker{};
};

} // namespace kivo::playback::control_plane::buffer
