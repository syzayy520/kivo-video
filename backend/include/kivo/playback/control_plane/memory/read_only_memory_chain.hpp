#pragma once

#include <array>
#include <limits>
#include <optional>
#include "kivo/playback/control_plane/memory/immutable_memory_block.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::memory {

/// ReadOnlyMemorySegment references a slice of an ImmutableMemoryBlock.
struct ReadOnlyMemorySegment
{
    revision::MemoryBlockId block_id{};
    ImmutableMemoryBlock block{};
    primitives::Bytes offset{};
    primitives::Bytes size{};
};

/// SegmentListRef is a reference to a pre-allocated arena segment list
/// (overflow only; NOT used for normal reads).
struct SegmentListRef
{
    const ReadOnlyMemorySegment* segments{nullptr};
    primitives::Count count{0};
};

/// ReadOnlyMemoryChain carries read data with inline segments (no hot-path heap).
/// Normal read uses inline_segments only (up to 4).
/// Heap overflow forbidden in hot path.
/// Overflow only uses pre-allocated arena/pool via overflow_segments.
struct ReadOnlyMemoryChain
{
    std::array<ReadOnlyMemorySegment, 4> inline_segments{};
    primitives::U32 inline_count{0};
    std::optional<SegmentListRef> overflow_segments{};
    primitives::Bytes total_size{};

    constexpr bool is_inline_only() const noexcept { return !overflow_segments.has_value(); }

    constexpr bool bounds_valid() const noexcept
    {
        for (primitives::U32 i = 0; i < inline_count && i < 4; ++i)
        {
            const auto& seg = inline_segments[i];
            if (seg.offset.value > std::numeric_limits<primitives::U64>::max() - seg.size.value)
                return false;
            if (seg.offset.value + seg.size.value > seg.block.size.value)
                return false;
            if (seg.block.size.value > seg.block.capacity.value)
                return false;
        }
        return true;
    }
};

} // namespace kivo::playback::control_plane::memory
