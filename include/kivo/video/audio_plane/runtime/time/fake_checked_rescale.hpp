// fake_checked_rescale.hpp — P6B runtime: fake overflow-checked PTS rescale
// Header-only fake implementation. Platform-neutral: no _mul128, no __int128.
#pragma once

#include <cstdint>
#include <cstdlib>
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/time/audio_checked_rescale.hpp"
#include "kivo/video/audio_plane/time/audio_time_base.hpp"

namespace kivo::video::audio_plane::runtime::time {

using kivo::video::audio_plane::time::TimestampOverflow;

// Fake overflow-checked rescale. Converts a PTS from one rational time base
// to another using only pre-check overflow detection (no 128-bit math).
//
// Formula (with pre-checks before each multiply):
//   intermediate = (pts * dst_num) / src_num        // checked multiply
//   result       = (intermediate * src_den) / dst_den  // checked multiply
class FakeCheckedRescale {
public:
    FakeCheckedRescale() = default;

    // Rescale `pts` from (src_num/src_den) to (dst_num/dst_den).
    // On any overflow or zero denominator, returns MultiplyOverflow.
    kivo::Expected<int64_t, TimestampOverflow> rescale(
        int64_t pts,
        int64_t src_num,
        int64_t src_den,
        int64_t dst_num,
        int64_t dst_den) const {
        // Denominator zero checks first.
        if (src_den == 0 || dst_den == 0) {
            return kivo::Expected<int64_t, TimestampOverflow>{
                TimestampOverflow::MultiplyOverflow};
        }
        if (src_num == 0) {
            return kivo::Expected<int64_t, TimestampOverflow>{int64_t{0}};
        }

        // stage 1: intermediate = (pts * src_num) / src_den
        // Pre-check: |pts| * |src_num| must fit in int64_t.
        if (!mul_fits(pts, src_num)) {
            return kivo::Expected<int64_t, TimestampOverflow>{
                TimestampOverflow::MultiplyOverflow};
        }
        int64_t stage1_num = pts * src_num;
        int64_t intermediate = stage1_num / src_den;

        // stage 2: result = (intermediate * dst_den) / dst_num
        if (!mul_fits(intermediate, dst_den)) {
            return kivo::Expected<int64_t, TimestampOverflow>{
                TimestampOverflow::MultiplyOverflow};
        }
        int64_t stage2_num = intermediate * dst_den;
        int64_t result = stage2_num / dst_num;
        return kivo::Expected<int64_t, TimestampOverflow>{result};
    }

private:
    // Platform-neutral overflow pre-check for a * b on int64_t.
    // Uses absolute values; INT64_MIN is treated as overflowing (conservative).
    static bool mul_fits(int64_t a, int64_t b) noexcept {
        if (a == 0 || b == 0) {
            return true;
        }
        int64_t abs_a = abs64(a);
        int64_t abs_b = abs64(b);
        if (abs_a > INT64_MAX / abs_b) {
            return false;
        }
        return true;
    }

    static int64_t abs64(int64_t v) noexcept {
        if (v < 0) {
            // INT64_MIN cannot be negated; treat as INT64_MAX (conservative).
            return v == INT64_MIN ? INT64_MAX : -v;
        }
        return v;
    }
};

}  // namespace kivo::video::audio_plane::runtime::time
