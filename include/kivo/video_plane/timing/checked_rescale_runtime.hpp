// checked_rescale_runtime.hpp -- P5E E2: checked rational rescale runtime
// int64 safe path -> int128 intermediate (MSVC _mul128 intrinsic) -> fail TimestampOverflow
#pragma once
#include <cstdint>
#include <string>
#include <intrin.h>

namespace kivo::video_plane::timing {

struct RescaleResult {
    int64_t value{0};
    bool overflow{false};
    std::string overflow_reason;
};

// Detect int64 multiplication overflow using MSVC _mul128 intrinsic
inline bool mul_overflow(int64_t a, int64_t b) {
    int64_t high;
    int64_t low = _mul128(a, b, &high);
    // If high bits are non-zero, result doesn't fit in int64
    // Also check sign: if a and b have same sign, high should be 0 or -1 (for negative)
    if (a < 0 && b < 0) {
        return high != -1 || low < 0; // positive result, high must be -1, low must be >= 0
    }
    if (a < 0 || b < 0) {
        return high != 0 || low > 0; // negative result, high must be 0, low must be <= 0
    }
    return high != 0; // both positive, high must be 0
}

// checked_rescale: (num * out_num) / (den * out_den)
// 1. try int64 safe path (detect overflow)
// 2. else fail TimestampOverflow (int128 path simulated via _mul128)
// 3. division by zero = error
inline RescaleResult checked_rescale(int64_t num, int64_t den, int64_t out_num, int64_t out_den) {
    RescaleResult r;
    if (den == 0 || out_den == 0) {
        r.overflow = true;
        r.overflow_reason = "division_by_zero";
        return r;
    }
    // Step 1: check if numerator multiply overflows int64
    if (mul_overflow(num, out_num)) {
        r.overflow = true;
        r.overflow_reason = "TimestampOverflow_int64_multiply_exceeded";
        return r;
    }
    // Step 2: check if denominator multiply overflows int64
    if (mul_overflow(den, out_den)) {
        r.overflow = true;
        r.overflow_reason = "TimestampOverflow_int64_denominator_exceeded";
        return r;
    }
    // Safe to compute in int64
    int64_t product_num = num * out_num;
    int64_t product_den = den * out_den;
    r.value = product_num / product_den;
    return r;
}

// Check if a multiplication would overflow int64
inline bool would_overflow_multiply(int64_t a, int64_t b) {
    return mul_overflow(a, b);
}

}  // namespace kivo::video_plane::timing
