#pragma once

#include <optional>
#include "kivo/playback/control_plane/memory/mutable_memory_block.hpp"

namespace kivo::playback::control_plane::memory {

/// MemoryLeaseStatus enumerates lease acquisition outcomes.
enum class MemoryLeaseStatus
{
    Granted,
    BackpressureRequired,
    PoolExhausted,
    Cancelled
};

/// MemoryLeaseResult is the result of an acquire request.
struct MemoryLeaseResult
{
    MemoryLeaseStatus status{MemoryLeaseStatus::PoolExhausted};
    std::optional<MutableBlockLease> lease{};
};

/// IReadBufferAllocator is the zero-copy memory lease/freeze/publish interface.
/// Rules:
/// - freeze_and_publish must receive MutableBlockLease, NOT just MemoryBlockId.
/// - ImmutableMemoryBlock does NOT own heavy deleter.
/// - Lifetime holds lightweight token, no heavy reclaim in realtime/hot path.
/// - Actual reclaim by MemoryPool maintenance/control context.
class IReadBufferAllocator
{
public:
    virtual ~IReadBufferAllocator() = default;

    virtual MemoryLeaseResult acquire(
        primitives::Bytes min_size,
        primitives::Bytes max_size,
        primitives::U64 alignment) = 0;

    virtual std::optional<ImmutableMemoryBlock> freeze_and_publish(
        const MutableBlockLease& lease,
        primitives::Bytes written_size) = 0;

    virtual void release_unpublished(const MutableBlockLease& lease) = 0;
};

} // namespace kivo::playback::control_plane::memory
