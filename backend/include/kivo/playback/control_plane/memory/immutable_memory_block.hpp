#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::memory {

/// ImmutableMemoryBlock is a frozen, published, read-only memory block.
/// data points to memory pool managed stable block (does NOT own heavy deleter).
/// Lifetime holds lightweight token; no heavy reclaim in realtime/hot path.
struct ImmutableMemoryBlock
{
    revision::MemoryBlockId id{};
    revision::MemoryPoolId pool_id{};
    primitives::Bytes capacity{};
    primitives::Bytes size{};
    primitives::U64 alignment{0};
    const std::byte* data{nullptr};
    primitives::U64 lifetime_token{0};
};

} // namespace kivo::playback::control_plane::memory
