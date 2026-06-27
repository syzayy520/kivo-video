// p010_unorm_gate.hpp -- P5G G6: P010 UnormPlaneViewVerified gate
// UNORM fallback: code10 = round(sampled_unorm16 * 65535.0 / 64.0)
// Tolerance: code10_tolerance <= 1, normalized_luma <= 1/876, normalized_chroma <= 1/448
#pragma once
#include <cstdint>
#include <cmath>
#include <string>

namespace kivo::video_plane::gpu_hw {

struct P010UnormGateResult {
    bool verified{false};
    uint32_t code10{0};
    uint32_t reference_code10{0};
    int32_t code10_diff{0};
    bool within_tolerance{false};
    std::string evidence_id;
    std::string failure_reason;
};

// UNORM fallback: code10 = round(sampled_unorm16 * 1023.0 / 65535.0)
// Note: design says code10 = round(sampled_unorm16 * 65535.0 / 64.0)
// But that would give values > 1023. The correct formula is:
// stored_word is 16-bit with 10 valid bits in MSB, so sampled_unorm16
// represents the full 16-bit value. code10 = (sampled_unorm16 >> 6) for integer,
// or code10 = round(sampled_unorm16 * 1023.0 / 65535.0) for UNORM.
// Actually, P010 stores 10 bits in 16-bit words (6 LSB padding).
// UNORM sampling gives the full 16-bit value normalized to [0,1].
// To recover code10: code10 = round(sampled_unorm16 * 1023.0 / 65535.0)
inline P010UnormGateResult verify_p010_unorm_plane(uint16_t sampled_unorm16, uint16_t reference_stored_word) {
    P010UnormGateResult r;
    r.code10 = static_cast<uint32_t>(std::round(static_cast<double>(sampled_unorm16) * 1023.0 / 65535.0));
    r.reference_code10 = static_cast<uint32_t>(reference_stored_word >> 6);
    r.code10_diff = static_cast<int32_t>(r.code10) - static_cast<int32_t>(r.reference_code10);

    // Tolerance: code10_tolerance <= 1
    if (std::abs(r.code10_diff) <= 1) {
        r.within_tolerance = true;
        r.verified = true;
        r.evidence_id = "p010_unorm_verified_within_tolerance";
    } else {
        r.within_tolerance = false;
        r.verified = false;
        r.failure_reason = "code10_diff_exceeds_tolerance";
    }
    return r;
}

}  // namespace kivo::video_plane::gpu_hw
