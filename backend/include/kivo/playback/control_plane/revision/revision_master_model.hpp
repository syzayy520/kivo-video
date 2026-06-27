#pragma once

#include <cstdint>
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::revision {

using primitives::U64;

// ─── ID Types ───

struct SessionId { U64 value{0}; constexpr SessionId() noexcept = default; constexpr explicit SessionId(U64 v) noexcept : value(v) {} };
struct CommandId { U64 value{0}; constexpr CommandId() noexcept = default; constexpr explicit CommandId(U64 v) noexcept : value(v) {} };
struct ReaderInstanceId { U64 value{0}; constexpr ReaderInstanceId() noexcept = default; constexpr explicit ReaderInstanceId(U64 v) noexcept : value(v) {} };
struct MemoryBlockId { U64 value{0}; constexpr MemoryBlockId() noexcept = default; constexpr explicit MemoryBlockId(U64 v) noexcept : value(v) {} };
struct MemoryPoolId { U64 value{0}; constexpr MemoryPoolId() noexcept = default; constexpr explicit MemoryPoolId(U64 v) noexcept : value(v) {} };
struct TrackSwitchFenceId { U64 value{0}; constexpr TrackSwitchFenceId() noexcept = default; constexpr explicit TrackSwitchFenceId(U64 v) noexcept : value(v) {} };
struct FeedbackId { U64 value{0}; constexpr FeedbackId() noexcept = default; constexpr explicit FeedbackId(U64 v) noexcept : value(v) {} };
struct EventSubscriptionId { U64 value{0}; constexpr EventSubscriptionId() noexcept = default; constexpr explicit EventSubscriptionId(U64 v) noexcept : value(v) {} };

// ─── Epoch / Revision Types ───

/// SessionGeneration identifies session lifecycle generation.
struct SessionGeneration { U64 value{0}; constexpr SessionGeneration() noexcept = default; constexpr explicit SessionGeneration(U64 v) noexcept : value(v) {} };
/// CommandSequence identifies command submission order.
struct CommandSequence { U64 value{0}; constexpr CommandSequence() noexcept = default; constexpr explicit CommandSequence(U64 v) noexcept : value(v) {} };
/// ReadEpoch identifies reader/read generation.
struct ReadEpoch { U64 value{0}; constexpr ReadEpoch() noexcept = default; constexpr explicit ReadEpoch(U64 v) noexcept : value(v) {} };
/// SeekEpoch identifies seek fence generation. Only Control Thread generates this.
struct SeekEpoch { U64 value{0}; constexpr SeekEpoch() noexcept = default; constexpr explicit SeekEpoch(U64 v) noexcept : value(v) {} };
/// RecoveryEpoch identifies recovery process generation.
struct RecoveryEpoch { U64 value{0}; constexpr RecoveryEpoch() noexcept = default; constexpr explicit RecoveryEpoch(U64 v) noexcept : value(v) {} };
/// StreamRevision identifies P4 -> P5/P6/P7 encoded stream generation.
struct StreamRevision { U64 value{0}; constexpr StreamRevision() noexcept = default; constexpr explicit StreamRevision(U64 v) noexcept : value(v) {} };
/// TimelineRevision identifies timeline snapshot generation.
struct TimelineRevision { U64 value{0}; constexpr TimelineRevision() noexcept = default; constexpr explicit TimelineRevision(U64 v) noexcept : value(v) {} };
/// P4RuntimePolicyRevision identifies P4 runtime policy update generation.
struct P4RuntimePolicyRevision { U64 value{0}; constexpr P4RuntimePolicyRevision() noexcept = default; constexpr explicit P4RuntimePolicyRevision(U64 v) noexcept : value(v) {} };

} // namespace kivo::playback::control_plane::revision
