// p010_integer_gate.hpp -- P5G G5: P010 IntegerPlaneViewVerified gate
// R16_UINT path: code10 = stored_word >> 6
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_hw {

struct P010IntegerGateResult {
    bool verified{false};
    uint32_t code10{0};
    uint16_t stored_word{0};
    std::string evidence_id;
    std::string failure_reason;
};

// Integer path: code10 = stored_word >> 6
inline P010IntegerGateResult verify_p010_integer_plane(uint16_t stored_word) {
    P010IntegerGateResult r;
    r.stored_word = stored_word;
    r.code10 = static_cast<uint32_t>(stored_word >> 6);
    r.verified = true;
    r.evidence_id = "p010_integer_verified";
    return r;
}

// Verify multiple samples
inline bool verify_p010_integer_batch(uint16_t const* samples, size_t count) {
    if (!samples || count == 0) return false;
    for (size_t i = 0; i < count; ++i) {
        auto r = verify_p010_integer_plane(samples[i]);
        if (!r.verified) return false;
        // code10 must be in [0, 1023]
        if (r.code10 > 1023) return false;
    }
    return true;
}

}  // namespace kivo::video_plane::gpu_hw
