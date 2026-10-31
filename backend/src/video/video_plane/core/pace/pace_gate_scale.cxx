#include "video/video_plane/core/pace/pace_gate_scale.hpp"

#include <cmath>

namespace kivo::video::core::pace {

namespace {

/// ppm 的换算系数：1 ppm = 1e-6。
constexpr double kPpmPerUnit = 1e6;

/// 秒的换算系数：1 s = 1e9 ns。
constexpr double kNanosPerSecond = 1e9;

}  // namespace

GateScale gate_scale(std::int64_t limit_ns,
                     std::int64_t span_ns,
                     double media_rate,
                     bool media_rate_measured,
                     std::int64_t drift_rebases) noexcept {
    GateScale out{};

    if (limit_ns <= 0) {
        out.ppm = measured_missing(kNoGateLimitReason);
        out.reach = measured_missing(kNoGateLimitReason);
        return out;
    }

    // ── 等价速率 ──
    //
    // 只在 `span_ns > 0` 时算。`span_ns == 0` 不给 0.0 当默认值：
    // 那个 0 会被读成"门严到不可用"，而事实是"没记录时长，换算无从谈起"。
    if (span_ns <= 0) {
        out.ppm = measured_missing(kNoSpanReason);
    } else {
        out.limit_ppm_equivalent =
            static_cast<double>(limit_ns) * kPpmPerUnit / static_cast<double>(span_ns);
        out.ppm = measured_ok();
    }

    // ── 撞门时间 ──
    if (!media_rate_measured) {
        out.reach = measured_missing(kRateUnmeasuredReason);
        return out;
    }

    // P14-W：门已经在修了，观测速率就不是"输入"，而是"修完之后剩下的"。
    // 用它去算"多久撞门"会把一个**已经撞过 49 次**的会话印成"还要 63.6 秒"。
    if (drift_rebases > 0) {
        out.reach = measured_missing(kGateActedReason);
        return out;
    }

    const double deviation = std::fabs(media_rate - 1.0);
    if (deviation < kMinReachDeviation) {
        out.reach = measured_missing(kRateTooCloseReason);
        return out;
    }

    out.limit_reach_s = static_cast<double>(limit_ns) / (deviation * kNanosPerSecond);
    out.reach = measured_ok();
    return out;
}

}  // namespace kivo::video::core::pace
