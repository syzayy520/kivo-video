#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::memory {

/// MemoryBlockState enumerates memory block lifecycle states.
/// Reuse rule: only FreeInPool or Reclaimable can be reused.
/// FrozenPublished / BorrowedByConsumer / RetiredPendingRefs must NOT be reused.
enum class MemoryBlockState
{
    FreeInPool,
    WritableLeased,
    FrozenPublished,
    BorrowedByConsumer,
    RetiredPendingRefs,
    Reclaimable
};

/// MutableMemoryBlock is a writable memory block leased from a pool.
struct MutableMemoryBlock
{
    revision::MemoryBlockId id{};
    revision::MemoryPoolId pool_id{};
    primitives::Bytes capacity{};
    primitives::U64 alignment{0};
    std::byte* data{nullptr};
};

/// MutableBlockLease pairs a block with a lease nonce.
/// freeze_and_publish must receive MutableBlockLease, NOT just MemoryBlockId,
/// to prevent non-owner freeze, double freeze, stale block id freeze.
struct MutableBlockLease
{
    revision::MemoryBlockId block_id{};
    revision::MemoryPoolId pool_id{};
    primitives::U64 lease_nonce{0};
    MutableMemoryBlock* block{nullptr};
};

} // namespace kivo::playback::control_plane::memory
