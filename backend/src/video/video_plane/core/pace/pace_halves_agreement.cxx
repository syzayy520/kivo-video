#include "video/video_plane/core/pace/pace_halves_agreement.hpp"

#include <cmath>

namespace kivo::video::core::pace {
namespace {

/// `1 ppm` 在一个跨度上积成多少纳秒：`ppm × span_ns / 1e6`。
///
/// 抽成函数是因为它在这个文件里出现两次（两个半程各一次），
/// 而两处写成两个略有出入的表达式时，`cumulative_gap_ns` 会变成
/// 「两个不同口径的数相减」—— 那种错不会让任何一个数变 NaN。
[[nodiscard]] double accumulated_ns(double slope_ppm, std::int64_t span_ns) noexcept {
    return slope_ppm * static_cast<double>(span_ns) / 1e6;
}

}  // namespace

const char* halves_relation_name(HalvesRelation relation) noexcept {
    switch (relation) {
        case HalvesRelation::kConsistent:
            return "consistent";
        case HalvesRelation::kInconsistent:
            return "inconsistent";
        case HalvesRelation::kUnknown:
            break;
    }
    return "unknown";
}

HalvesAgreement compare_halves(const HalvesFit& first,
                               const HalvesFit& second,
                               std::int64_t total_span_ns,
                               std::int64_t budget_ns) noexcept {
    HalvesAgreement out;
    out.first_span_ns = first.span_ns;
    out.second_span_ns = second.span_ns;

    // ── 先填只依赖斜率的那一格 ──
    //
    // `cumulative_gap_ns` 不需要标准误，所以在任何 SE 判据之前就算好。
    // 它回答的是「这条混合斜率可能错多少纳秒」—— 一个纯粹由两个斜率和
    // 两段跨度决定的算术事实。把它排在 `estimated` 之后，
    // 会让一份 SE 估不出来的证据连「差多少」都读不到。
    const bool slopes_finite = std::isfinite(first.slope_ppm) &&
                               std::isfinite(second.slope_ppm);
    if (slopes_finite) {
        out.gap_ppm = std::fabs(first.slope_ppm - second.slope_ppm);
        const double gap_ns = std::fabs(accumulated_ns(first.slope_ppm, first.span_ns) -
                                       accumulated_ns(second.slope_ppm, second.span_ns));
        // 四舍五入而不是截断：截断会让每一份证据都系统性偏向 0，
        // 而这个字段存在的理由正是「别让那个 0 看起来像没差」。
        out.cumulative_gap_ns = static_cast<std::int64_t>(gap_ns + 0.5);
    }

    // ── 误差那一组 ──
    //
    // 三个条件缺一不可：两侧都估出来了、SE 非负、全部有限。
    // 少查一个，`sqrt(se1² + se2²)` 就会在某个 NaN 输入上给出 NaN，
    // 而 NaN 与任何数比较都是 false —— 于是 `gap > 1.96·pooled` 为假，
    // 一个算不出来的证据会被判成 `consistent`。**这是本文件最危险的一条路。**
    const bool errors_usable =
        first.estimated && second.estimated &&
        std::isfinite(first.std_error_ppm) && std::isfinite(second.std_error_ppm) &&
        first.std_error_ppm >= 0.0 && second.std_error_ppm >= 0.0;
    if (!errors_usable) {
        // `estimated` 保持 false，`relation` 保持 `kUnknown`。
        // 不在这里补一个"看起来安全"的 consistent ——
        // 「量不出来」与「量出来是一致的」是两件相反的事。
        return out;
    }

    out.estimated = true;
    out.first_std_error_ppm = first.std_error_ppm;
    out.second_std_error_ppm = second.std_error_ppm;
    const double half1 = kConfidenceZ * first.std_error_ppm;
    const double half2 = kConfidenceZ * second.std_error_ppm;
    out.first_ci_low_ppm = first.slope_ppm - half1;
    out.first_ci_high_ppm = first.slope_ppm + half1;
    out.second_ci_low_ppm = second.slope_ppm - half2;
    out.second_ci_high_ppm = second.slope_ppm + half2;

    // 两侧独立 ⇒ 差值方差是两者方差之和。用"区间端点是否相交"会漏掉
    // 一侧极宽把另一侧整个包住的情形，而那种情形恰恰最该报不一致。
    const double pooled =
        std::sqrt(first.std_error_ppm * first.std_error_ppm +
                  second.std_error_ppm * second.std_error_ppm);
    out.pooled_error_ppm = pooled;
    out.resolution_ppm = kConfidenceZ * pooled;

    // 额度换算成同一把秤上的 ppm：`额度_ns / 总跨度_ns × 1e6`。
    // 跨度非正时这个换算没有定义 —— 置 0 并让 `resolution_sufficient` 保持 false，
    // 因为「无法判断够不够」不等于「够」。
    if (total_span_ns > 0 && budget_ns > 0) {
        out.budget_equivalent_ppm = static_cast<double>(budget_ns) * 1e6 /
                                    static_cast<double>(total_span_ns);
        out.resolution_sufficient = out.resolution_ppm <= out.budget_equivalent_ppm;
    }

    // `pooled == 0` 是合法输入：两侧都是完美直线（残差恒为 0）。
    // 那时比值无定义，但**结论有定义**：gap 为 0 就是同一条线，非 0 就是两条线。
    // 用 0/0 让 NaN 顺流而下，会让这种"最干净"的证据反而判不出来。
    if (pooled > 0.0) {
        out.z = out.gap_ppm / pooled;
    } else {
        out.z = (out.gap_ppm > 0.0) ? INFINITY : 0.0;
    }

    // 结论要过两道门：**统计上分得开**（z）且**材料上差得起**（累积 ns）。
    //
    // 只过第一道时，一份完美线性的序列会被判成 `kInconsistent`：残差只剩浮点噪声，
    // 标准误落到 1e-15 ppm，于是 1e-14 ppm 的差也成了 6 σ，`drift_verdict` 被降成
    // `unknown` —— 最干净的证据反而判不出来（`p13l` 的预算单测撞到两次）。
    // 第二道门的出处见 `kMinCumulativeGapNs`。
    const bool statistically_apart = out.gap_ppm > kConfidenceZ * pooled;
    const bool materially_apart = out.cumulative_gap_ns >= kMinCumulativeGapNs;
    out.relation = (statistically_apart && materially_apart) ? HalvesRelation::kInconsistent
                                                            : HalvesRelation::kConsistent;
    return out;
}

}  // namespace kivo::video::core::pace
