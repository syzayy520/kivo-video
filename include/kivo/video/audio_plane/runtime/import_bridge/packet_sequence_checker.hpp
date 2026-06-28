// packet_sequence_checker.hpp — P6B import_bridge: sequence & timestamp checks
// HEADER-ONLY. Detects monotonicity violations, duplicates, and timestamp jumps.
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/packet/audio_packet_id.hpp"

namespace kivo::video::audio_plane::runtime::import_bridge {

// SequenceCheckResult: outcome of a packet sequence check.
enum class SequenceCheckResult : int32_t {
    Monotonic = 0,                       // strictly increasing
    NonMonotonicWithDiscontinuity = 1,  // gap but is_discontinuity flag set → ok
    NonMonotonicViolation = 2,          // gap without discontinuity → error
    Duplicate = 3,                       // same sequence as last → error
};

// TimestampCheckResult: outcome of a PTS monotonicity check.
enum class TimestampCheckResult : int32_t {
    Ok = 0,           // pts >= prev_pts
    BackwardJump = 1, // pts < prev_pts (non-wrap)
    Wrap = 2,         // large negative delta interpreted as timestamp wrap
};

// PacketSequenceChecker
// Stateless checker for packet sequence and PTS monotonicity.
// No backend; pure arithmetic over uint64_t / int64_t values.
class PacketSequenceChecker {
public:
    PacketSequenceChecker() = default;

    // Check packet sequence against the last known sequence.
    SequenceCheckResult check(
        packet::AudioPacketSequence sequence,
        bool is_discontinuity,
        packet::AudioPacketSequence last_sequence) const noexcept
    {
        const uint64_t cur = sequence.value;
        const uint64_t prev = last_sequence.value;
        if (cur == prev) {
            return SequenceCheckResult::Duplicate;
        }
        if (cur > prev) {
            return SequenceCheckResult::Monotonic;
        }
        // cur < prev: non-monotonic
        if (is_discontinuity) {
            return SequenceCheckResult::NonMonotonicWithDiscontinuity;
        }
        return SequenceCheckResult::NonMonotonicViolation;
    }

    // Check PTS monotonicity. Wrap is detected when the backward jump exceeds
    // half of the int64_t range (heuristic for timestamp wraparound).
    TimestampCheckResult check_timestamp(
        int64_t pts,
        int64_t prev_pts,
        bool is_monotonic_sequence) const noexcept
    {
        if (pts >= prev_pts) {
            return TimestampCheckResult::Ok;
        }
        // Backward jump. Distinguish wrap from violation by magnitude.
        const uint64_t delta = static_cast<uint64_t>(prev_pts - pts);
        constexpr uint64_t wrap_threshold =
            static_cast<uint64_t>(1) << 62;  // half of int64_t positive range
        if (delta > wrap_threshold) {
            return TimestampCheckResult::Wrap;
        }
        // A non-monotonic sequence already signals a discontinuity; treat the
        // backward PTS as a wrap rather than a violation in that case.
        if (!is_monotonic_sequence) {
            return TimestampCheckResult::Wrap;
        }
        return TimestampCheckResult::BackwardJump;
    }
};

}  // namespace kivo::video::audio_plane::runtime::import_bridge
