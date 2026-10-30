#include "video/video_plane/core/pace/pace_drift_budget.hpp"

#include <cmath>

namespace kivo::video::core::pace {
namespace {

/// `int64` 的安全上界，留一点余量给舍入。
///
/// 为什么不直接依赖 `llround` 的边界行为：超出范围时它是未定义行为，
/// 而在 MSVC 上恰好返回 `LLONG_MIN` —— 一份荒谬输入会变成一个
/// **负的累积漂移**，看起来像"音频跑到了前面"。宁可显式判掉。
constexpr double kMaxSafeNs = 9.0e18;

/// `ppm × 秒 → ns`。`ppm` 单位是 µs/s，所以要再乘 1e3；
/// 用 `span_ns` 做分母时这一步并进除数：`ppm · ns / 1e6`。
///
/// 返回 `false` 表示这个数落不进 `int64` —— 那时整个累积量都不可信，
/// 调用方必须当成"没算出来"，而不是拿一个钳过的数去和额度比。
bool to_ns(double ppm, std::int64_t span_ns, std::int64_t& out) noexcept {
    const double value = ppm * static_cast<double>(span_ns) / 1e6;
    if (!std::isfinite(value) || value > kMaxSafeNs || value < -kMaxSafeNs) {
        return false;
    }
    out = static_cast<std::int64_t>(std::llround(value));
    return true;
}

}  // namespace

const char* drift_verdict_name(DriftVerdict verdict) noexcept {
    switch (verdict) {
        case DriftVerdict::kWithinBudget:
            return "within_budget";
        case DriftVerdict::kOverBudget:
            return "over_budget";
        case DriftVerdict::kUnknown:
        default:
            // `default` 兜到 `unknown` 而不是随便给个名字：拼错的枚举值
            // 应当表现为"没有结论"，而不是伪装成一个具体的判定。
            return "unknown";
    }
}

DriftAccumulation accumulate_drift(double slope_ppm, double slope_std_error_ppm,
                                   std::int64_t span_ns, bool usable) noexcept {
    DriftAccumulation out;
    out.threshold_ns = kAudibleThresholdNs;

    // 非有限数 / 负的 SE / 不前进的跨度：这三样都意味着"输入已经不成立了"。
    // 负 SE 单独判，是因为它会让 `half` 变成负数，于是 `low > high` ——
    // 置信带两端交换，`farthest` 挑出来的仍是同一个数，错误被悄悄吞掉。
    if (!std::isfinite(slope_ppm) || !std::isfinite(slope_std_error_ppm) ||
        slope_std_error_ppm < 0.0 || span_ns <= 0) {
        return out;
    }

    const double half = kConfidenceZ * slope_std_error_ppm;
    const double low = slope_ppm - half;
    const double high = slope_ppm + half;
    const double farthest = (std::fabs(low) >= std::fabs(high)) ? low : high;

    std::int64_t point_ns = 0;
    std::int64_t upper_ns = 0;
    if (!to_ns(slope_ppm, span_ns, point_ns) ||
        !to_ns(std::fabs(farthest), span_ns, upper_ns)) {
        return out;
    }

    out.estimated = true;
    out.accumulated_ns = point_ns;
    out.accumulated_upper_ns = upper_ns;

    // 数算完了，但没有资格下结论 —— 这是本单元唯一一处"有事实、无判定"。
    if (!usable) {
        out.verdict = DriftVerdict::kUnknown;
        return out;
    }

    // `>` 而不是 `>=`：恰好等于额度算"未超过"。
    // 这个开闭区间必须写死一处，否则测试和实现各拿一个口径，
    // 谁也不会发现 45 ms 整那一刻两份结论相反。
    out.verdict = (upper_ns > kAudibleThresholdNs) ? DriftVerdict::kOverBudget
                                                   : DriftVerdict::kWithinBudget;
    return out;
}

}  // namespace kivo::video::core::pace
