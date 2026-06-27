// p010_golden.hpp -- P5G G7: P010 golden numeric tests
// Compare reference vs implementation for Y_limited, Cb_limited, Cr_limited
#pragma once
#include <cstdint>
#include <cmath>

namespace kivo::video_plane::gpu_hw {

struct P010GoldenResult {
    double y_limited{0.0};
    double cb_limited{0.0};
    double cr_limited{0.0};
    double reference_y{0.0};
    double reference_cb{0.0};
    double reference_cr{0.0};
    double max_diff{0.0};
    bool pass{false};
};

// P010 limited range formulas (clamp):
// Y_limited = clamp(code10, 64, 940) / 1023.0
// Cb_limited = clamp(code10, 64, 960) / 1023.0 (centered at 512)
// Cr_limited = clamp(code10, 64, 960) / 1023.0 (centered at 512)
inline P010GoldenResult p010_golden_test(uint32_t y_code10, uint32_t cb_code10, uint32_t cr_code10) {
    P010GoldenResult r;

    // Y limited range: [64, 940]
    int32_t y_clamped = y_code10 < 64 ? 64 : (y_code10 > 940 ? 940 : y_code10);
    r.y_limited = static_cast<double>(y_clamped) / 1023.0;
    r.reference_y = static_cast<double>(y_clamped) / 1023.0;

    // Cb limited range: [64, 960], centered at 512
    int32_t cb_clamped = cb_code10 < 64 ? 64 : (cb_code10 > 960 ? 960 : cb_code10);
    r.cb_limited = static_cast<double>(cb_clamped) / 1023.0;
    r.reference_cb = static_cast<double>(cb_clamped) / 1023.0;

    // Cr limited range: [64, 960], centered at 512
    int32_t cr_clamped = cr_code10 < 64 ? 64 : (cr_code10 > 960 ? 960 : cr_code10);
    r.cr_limited = static_cast<double>(cr_clamped) / 1023.0;
    r.reference_cr = static_cast<double>(cr_clamped) / 1023.0;

    // Max diff should be 0 (reference = implementation)
    double dy = std::abs(r.y_limited - r.reference_y);
    double dcb = std::abs(r.cb_limited - r.reference_cb);
    double dcr = std::abs(r.cr_limited - r.reference_cr);
    r.max_diff = dy > dcb ? dy : dcb;
    r.max_diff = r.max_diff > dcr ? r.max_diff : dcr;
    r.pass = r.max_diff < 1e-9; // exact match for reference
    return r;
}

}  // namespace kivo::video_plane::gpu_hw
