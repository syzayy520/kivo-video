#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::memory {

/// MemoryPoolStrategy enumerates memory pool allocation strategies.
enum class MemoryPoolStrategy
{
    FixedSlab,
    SizeClassSlab,
    LargeBlockFallback
};

/// MemoryPoolPolicy defines pool sizing, alignment, and reclaim rules.
/// Reuse rules:
/// - Only state == FreeInPool or Reclaimable can be reused.
/// - FrozenPublished / BorrowedByConsumer / RetiredPendingRefs must NOT be reused.
/// - refcount is safety guard, not business state.
/// Deleter rules:
/// - shared_ptr/lifetime token deleter must NOT do heavy MemoryPool work.
/// - Deleter may enqueue lightweight retired-block token.
/// - Actual reclaim on MemoryPool maintenance/control context.
/// - No heavy reclaim under internal lock.
struct MemoryPoolPolicy
{
    MemoryPoolStrategy strategy{MemoryPoolStrategy::FixedSlab};
    primitives::Bytes default_block_size{};
    primitives::Bytes large_block_threshold{};
    primitives::U64 alignment_bytes{0};
    primitives::Bytes max_total_bytes{};
    primitives::Count max_block_count{};
    bool allow_reuse{true};
    bool allow_backpressure_when_exhausted{false};
    bool shrink_on_pause{false};
    bool reclaim_on_seek{false};
    bool reclaim_on_stop{false};
    bool reclaim_on_idle{false};
};

} // namespace kivo::playback::control_plane::memory
