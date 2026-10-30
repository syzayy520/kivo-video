#include "video/video_plane/core/pace/pace_drift_trend.hpp"

#include "video/video_plane/core/pace/pace_histogram.hpp"
#include "video/video_plane/core/pace/pace_hac.hpp"
#include "video/video_plane/core/pace/pace_window_stats.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace kivo::video::core::pace {
namespace {

/// 一次最小二乘。输入是 `(t_ns, y_ns)` 对，`t` 以首点为原点。
///
/// 为什么 `t` 要先减掉原点：60 秒片源的绝对媒体时刻是 6e10 ns 量级，
/// 直接平方会到 3.6e21 —— 越出 `int64`，也超出 `double` 的有效整数位。
/// 减掉原点后 `t ≤ 6e10`，平方仍在 `double` 的 53 位整数范围内。
struct Regression {
    double slope_per_ns = 0.0;
    double r2 = 0.0;
    /// 斜率的标准误（ns/ns）。`slope_per_ns / slope_se_per_ns` 就是 t 统计量。
    ///
    /// 为什么必须一起算：只有斜率没有标准误时，"−0.022 ppm"看起来比
    /// "0.000 ppm"精确得多，而在 `r2 = 5.5e-05` 之下两者其实同样没用。
    /// 证据里出现六位小数就是在向读者暗示一个它并不具备的分辨力。
    double slope_se_per_ns = 0.0;
};

/// 累加器：`n / Σt / Σy / Σtt / Σty`（单位都是 ns 的浮点）。
struct Accumulator {
    double n = 0.0;
    double sum_t = 0.0;
    double sum_y = 0.0;
    double sum_tt = 0.0;
    double sum_ty = 0.0;
    double sum_yy = 0.0;

    void add(double t_ns, double y_ns) noexcept {
        n += 1.0;
        sum_t += t_ns;
        sum_y += y_ns;
        sum_tt += t_ns * t_ns;
        sum_ty += t_ns * y_ns;
        sum_yy += y_ns * y_ns;
    }

    [[nodiscard]] Regression solve() const noexcept {
        Regression out;
        if (n < 2.0) {
            return out;
        }
        // 中心化矩：`Sxx = Σ(t−t̄)²`，`Sxy = Σ(t−t̄)(y−ȳ)`。
        //
        // 为什么不留 `nΣt² − (Σt)²` 那套：它的量纲是 `n·Sxx`，
        // 算 SSres 时忘了除 n 就把残差整块减成负数，`r2` 会被钳成 1.0 ——
        // 于是一条纯噪声序列在证据里显示"完美线性"。单测 `t05` 就是这么打红的。
        const double sxx = sum_tt - sum_t * sum_t / n;
        if (sxx <= 0.0) {
            return out;  // 时间不前进：没有斜率可言
        }
        const double sxy = sum_ty - sum_t * sum_y / n;
        out.slope_per_ns = sxy / sxx;

        const double mean_y = sum_y / n;
        const double sstot = sum_yy - n * mean_y * mean_y;
        if (sstot <= 0.0) {
            // 所有 y 完全相同：要么真的零抖动，要么样本退化。
            // 报 0 而不是 1 —— "r2 = 1" 会被读成"完美的线性漂移"。
            out.r2 = 0.0;
            return out;
        }
        // `SSres = SStot − Sxy²/Sxx`（带截距的最小二乘恒等式）。
        const double ssres = sstot - sxy * sxy / sxx;
        const double clamped = ssres < 0.0 ? 0.0 : ssres;  // 浮点误差可能给 −1e−9
        out.r2 = 1.0 - clamped / sstot;

        // 斜率标准误：`sqrt(σ̂² / Sxx)`，`σ̂² = SSres / (n − 2)`（两个参数：截距 + 斜率）。
        //
        // `n < 3` 时自由度 ≤ 0，估算不出残差方差 —— 这时 SE 留 0 并**由调用方
        // 用 `usable` 否掉整条趋势**，而不是塞一个看起来很小的数进来。
        // SE 为 0 在报告里意味着"无残差"（完美线性），那是完全不同的结论。
        if (n >= 3.0) {
            out.slope_se_per_ns = std::sqrt(clamped / (n - 2.0) / sxx);
        }
        return out;
    }
};

/// 1 ppm = 每秒偏 1 µs = 1000 ns/s。斜率单位是 ns/ns，故乘 1e6。
[[nodiscard]] double to_ppm(double slope_per_ns) noexcept { return slope_per_ns * 1e6; }

}  // namespace

DriftTrend summarize_drift(const DriftSeries& series) noexcept {
    DriftTrend out;
    const std::int32_t n = series.size();
    out.samples_used = n;
    out.truncated = series.truncated();
    out.segments = series.segments();
    // P13-M：段内最宽的空洞。与 `segments` 一起在 `n == 0` / 跨度过短的
    // 提前返回**之前**填好 —— 短到算不出斜率的证据同样需要知道"这条段里
    // 有个多宽的洞"，否则不可用的原因会只剩一个 "fewer than 30 samples"，
    // 而真实原因可能是那 30 个点被一个大洞摊在了一百秒里。
    out.segment_largest_gap_cells = series.largest_gap_cells();
    out.segment_contiguous = series.contiguous();

    if (n == 0) {
        out.unusable_reason = "the drift series is empty (no decimated sample was accepted)";
        return out;
    }

    const std::int64_t t0 = series.at(0).media_ns;
    const std::int64_t t_end = series.at(n - 1).media_ns;
    out.span_ns = t_end - t0;
    if (out.span_ns <= 0) {
        out.unusable_reason = "media time does not advance across the sequence";
        return out;
    }

    // ── 逐点回归 ──
    Accumulator all;
    for (std::int32_t i = 0; i < n; ++i) {
        const DriftPoint point = series.at(i);
        all.add(static_cast<double>(point.media_ns - t0),
                static_cast<double>(point.lateness_ns));
    }
    const Regression point_fit = all.solve();
    out.slope_ppm = to_ppm(point_fit.slope_per_ns);
    out.r2 = point_fit.r2;

    // ── 前后半程（诊断，不参与任何判定）──
    //
    // 全序列斜率答不出"这是一条持续的漂移，还是中途跳了一下"。两种情形
    // 给出同样的斜率，含义却相反：前者是时钟速率差（会一直累积），
    // 后者是一次性错位（不会继续变大，修的方向完全不同）。
    // 各取一半再各拟合一次，两个方向一比就分开了。
    {
        const std::int32_t half = n / 2;
        Accumulator first_half;
        Accumulator second_half;
        for (std::int32_t i = 0; i < n; ++i) {
            const DriftPoint point = series.at(i);
            const double t = static_cast<double>(point.media_ns - t0);
            const double y = static_cast<double>(point.lateness_ns);
            if (i < half) {
                first_half.add(t, y);
            } else {
                second_half.add(t, y);
            }
        }
        // 半程斜率要留 `slope_per_ns` 原件：HAC 的残差 `e_i = y_i − (a + b·t_i)`
        // 必须用**这一段自己的**斜率，传 ppm 或传全序列的斜率都会让 `γ0`
        // 与真实残差方差无关 —— 那样算出的 SE 是另一个量，而且不报错。
        const double first_slope_per_ns = first_half.solve().slope_per_ns;
        const double second_slope_per_ns = second_half.solve().slope_per_ns;
        out.slope_first_half_ppm = to_ppm(first_slope_per_ns);
        out.slope_second_half_ppm = to_ppm(second_slope_per_ns);
        out.slope_halves_agree = (out.slope_first_half_ppm > 0.0 &&
                                  out.slope_second_half_ppm > 0.0) ||
                                 (out.slope_first_half_ppm < 0.0 &&
                                  out.slope_second_half_ppm < 0.0);

        // ── 半程各自的误差棒与"差多少算差"（P13-O）──
        //
        // 两段各取自己那一段的 HAC：Bartlett 核里的 `L` 钳制依赖这一段的 `n`，
        // 拿全序列的 `L` 去算半程，会在半程点少时把 `L` 撑得比可用残差还大。
        const HacSlopeError first_hac =
            estimate_hac_slope_error_range(series, 0, half, first_slope_per_ns);
        const HacSlopeError second_hac =
            estimate_hac_slope_error_range(series, half, n, second_slope_per_ns);

        // 跨度取**真实首末差**而不是 `n/2 × 采样间隔`：两半程的长度由整数除法决定，
        // 末尾那个点的归属会让两者差一格；用真实跨度就不必假设它们等长。
        HalvesFit first_fit;
        first_fit.estimated = first_hac.estimated;
        first_fit.slope_ppm = out.slope_first_half_ppm;
        first_fit.std_error_ppm = to_ppm(first_hac.std_error_per_ns);
        first_fit.span_ns = half > 1 ? series.at(half - 1).media_ns - series.at(0).media_ns : 0;

        HalvesFit second_fit;
        second_fit.estimated = second_hac.estimated;
        second_fit.slope_ppm = out.slope_second_half_ppm;
        second_fit.std_error_ppm = to_ppm(second_hac.std_error_per_ns);
        second_fit.span_ns =
            n - 1 > half ? series.at(n - 1).media_ns - series.at(half).media_ns : 0;

        out.halves = compare_halves(first_fit, second_fit, out.span_ns, kAudibleThresholdNs);
    }

    // ── 误差棒（P13-J 起，P13-K 换模型）──
    //
    // 逐点回归的斜率本身是精确的（输入是 ns 整数），但它描述的这条线
    // 只解释了 `r2` 那么点方差。`r2 = 5.5e-05` 时斜率的标准误
    // 比斜率本身大一个数量级 —— 不给标准误，读者看到的六位小数
    // 全是噪声，却会读成"精确测到 −0.022 ppm"。
    //
    // P13-K 把 IID 版换成 HAC 版：`lateness` 的残差正相关，IID 假设让
    // 有效样本数被高估，SE 偏小、CI 过窄，于是 r3 那条 `r2 = 0.007` 的线
    // 被判成"看得见"。旧值原地保留在 `_iid` 里，供人对照这次修正动了多少。
    //
    // 这里**不因 `usable == false` 而清零**：清零等于宣称"没有误差"，
    // 而样本少时的真实情况是"误差大到无法估计"。要否掉这条趋势，
    // 用 `usable`，不要用一个假的 0 SE。
    out.slope_ppm_std_error_iid = to_ppm(point_fit.slope_se_per_ns);

    const HacSlopeError hac = estimate_hac_slope_error(series, point_fit.slope_per_ns);
    out.hac_estimated = hac.estimated;
    out.hac_lags = hac.lags;
    out.hac_lag1_autocorrelation = hac.lag1_autocorrelation;
    // 估不出来就沿用 IID 值 —— 但 `hac_estimated = false` 会一路写进证据，
    // 读者不会把"没修正"当成"修正后一样"。
    out.slope_ppm_std_error =
        hac.estimated ? to_ppm(hac.std_error_per_ns) : out.slope_ppm_std_error_iid;
    // 膨胀比取 **SE 之比**而不是方差之比：读者眼前摆着两个 SE，
    // 比例就得是同一把秤上量出来的两个数直接相除，否则还得自己开方。
    out.slope_ppm_std_error_inflation =
        (out.slope_ppm_std_error_iid > 0.0)
            ? (out.slope_ppm_std_error / out.slope_ppm_std_error_iid)
            : 1.0;

    const double half_width = 1.96 * out.slope_ppm_std_error;
    out.slope_ci_low_ppm = out.slope_ppm - half_width;
    out.slope_ci_high_ppm = out.slope_ppm + half_width;
    if (out.slope_ppm_std_error > 0.0) {
        out.slope_detectable = (out.slope_ci_low_ppm > 0.0) || (out.slope_ci_high_ppm < 0.0);
    } else {
        // SE == 0 有两种来源：残差恰为 0（完美线性），或 `n < 3` 算不出方差。
        // 前者"看得见"，后者"算不出"，靠斜率是否为 0 区分不了 —— 于是按
        // 保守的一侧处理：只有斜率非零才算看得见，否则一律 false。
        // 单测里这两种情况都要覆盖，不能靠这一个 `else` 猜。
        out.slope_detectable = (out.slope_ppm != 0.0);
    }

    // ── 分窗：每窗一个中位数 ──
    //
    // P13-J 之前这里每窗建一个 `SignedHistogram` 取桶中心。真机上 2013/2048
    // 个点落在同一个桶里（0~250 µs，桶中心 125 µs），于是**八个窗的中位数
    // 全是 125000**，`delta_median_ns` 恒 0、`slope_from_windows_ppm` 恒 0。
    // 那不是"没有漂移"，是"尺子上只有一个刻度"：真实 lateness 从 29 µs 到
    // 250 µs 铺了整整 220 µs，全被桶中心一口吃掉。
    //
    // 现在改从原始值 `nth_element` 选，桶只给超采样窗兜底（见 `pace_window_stats`）。
    std::array<WindowSlice, DriftTrend::kWindowCount> slices{};
    const std::int32_t recorded_windows =
        split_windows(series, DriftTrend::kWindowCount, slices.data());
    (void)recorded_windows;

    const std::int64_t span = out.span_ns;
    Accumulator windows;
    SignedHistogram fallback;  // 8 KB 栈，只在超采样窗上被填
    std::int32_t recorded = 0;
    std::int32_t fallback_windows = 0;
    // P13-N：只为汇总出一个"整批的精确代表值"。8 个 int64 = 64 B 栈。
    std::array<std::int64_t, DriftTrend::kWindowCount> medians{};
    for (std::int32_t w = 0; w < DriftTrend::kWindowCount; ++w) {
        const WindowSlice slice = slices[static_cast<std::size_t>(w)];
        if (slice.count == 0) {
            continue;
        }
        const WindowMedian median = median_of_slice(series, slice, fallback);
        if (!median.exact) {
            ++fallback_windows;
        }
        medians[static_cast<std::size_t>(recorded)] = median.value_ns;
        const std::int64_t start = t0 + span * w / DriftTrend::kWindowCount;
        const std::int64_t width = span / DriftTrend::kWindowCount;
        out.window_start_ns[static_cast<std::size_t>(recorded)] = start;
        out.window_median_ns[static_cast<std::size_t>(recorded)] = median.value_ns;
        out.window_samples[static_cast<std::size_t>(recorded)] = median.samples;
        // 窗口中心作为该点的横坐标：中位数代表整个窗口，坐标取中心而不是起点，
        // 否则斜率会被系统性地偏掉半个窗宽。
        windows.add(static_cast<double>(start + width / 2 - t0),
                    static_cast<double>(median.value_ns));
        ++recorded;
    }
    out.window_count = recorded;
    out.window_median_fallback_windows = fallback_windows;
    out.window_median_exact = (fallback_windows == 0);

    // 整批的精确参照（P13-N）。秩取 `(n−1)/2`，与 `SignedHistogram::percentile_ns(0.50)`
    // 的上中位口径一致（n = 8 → 第 4 个，n = 7 → 第 4 个）。两个中位数用两套秩，
    // 它们就没法放在同一张表里比 —— 而这张卡存在的全部理由就是让它们能比。
    if (recorded > 0) {
        std::sort(medians.begin(), medians.begin() + recorded);
        out.median_of_window_medians_ns = medians[static_cast<std::size_t>((recorded - 1) / 2)];
    }
    if (recorded >= 2) {
        const Regression window_fit = windows.solve();
        out.slope_from_windows_ppm = to_ppm(window_fit.slope_per_ns);
        out.first_window_median_ns = out.window_median_ns[0];
        out.last_window_median_ns = out.window_median_ns[static_cast<std::size_t>(recorded - 1)];
        out.delta_median_ns = out.last_window_median_ns - out.first_window_median_ns;
    } else {
        out.slope_from_windows_ppm = 0.0;
        out.first_window_median_ns = out.window_median_ns[0];
        out.last_window_median_ns = out.window_median_ns[0];
        out.delta_median_ns = 0;
    }

    // ── 可用性 ──
    //
    // `n < 3` 时残差方差无从估计（自由度 ≤ 0），`slope_detectable` 那个
    // `else` 分支只能按"斜率非零就算看得见"处理 —— 而两个点必过一条直线，
    // 任何两点的斜率都"非零"。这里明确否掉，不给那个分支留歧义。
    //
    // 两道统计门只**设原因**、不提前返回（P13-L 改的单出口）：
    // 提前返回会让一份样本不足的证据留下 `drift_accumulated_ns = 0`
    // 而 `slope_ppm` 非零 —— 那个 0 会被读成"没漂"，正是本卡要消灭的读法。
    // 数照算、结论不给，由 `drift_verdict = unknown` 表达。
    if (n < 3) {
        out.slope_detectable = false;
    }
    if (n < DriftTrend::kMinSamples) {
        out.unusable_reason =
            "fewer than 30 decimated samples; a slope over that few points describes noise, "
            "not drift";
    } else if (out.span_ns < DriftTrend::kMinSpanNs) {
        out.unusable_reason =
            "time span is shorter than 30 s; a 0.1% rate mismatch accumulates only 8 ms over "
            "8 s, which is the same size as the jitter itself, so it cannot be separated";
    } else {
        out.usable = true;
        out.unusable_reason = "";
    }

    // ── 累积量与额度判读（P13-L）──
    //
    // `slope_detectable` 到此为止只回答了"与 0 分得开吗"。读者真正要问的是
    // "要紧吗"，而这两件事可以完全脱钩：0.48 ppm × 35 s = 17 µs（听不出来，
    // 但 `detectable` 为真）；反过来 CI 含 0 时远端积起来也可能越过 45 ms。
    // 所以这里把 ppm 积回 ns，再对着一个写明出处的额度比一次。
    const DriftAccumulation accumulated = accumulate_drift(
        out.slope_ppm, out.slope_ppm_std_error, out.span_ns, out.usable);
    out.drift_accumulated_ns = accumulated.accumulated_ns;
    out.drift_accumulated_upper_ns = accumulated.accumulated_upper_ns;
    out.drift_budget_ns = accumulated.threshold_ns;
    out.drift_verdict = accumulated.verdict;

    // `kInconsistent` 是**反证**，不是"没意见"：
    // 两个半程不是同一条线时，`slope × span` 这个乘法没有定义域 ——
    // 它可能严重低估（p13n-fwd-r1 前半 0.6 ppm 在 65 s 里积了 38.8 µs，
    // 而全序列 −0.0015 ppm × 130 s 给出 −0.2 µs）。
    // 所以这时不给结论，而不是给一个"在上限内"的结论。
    //
    // `kConsistent` 与 `kUnknown` 都不改 verdict：前者是支持，后者是没意见。
    // 只有反证才推翻。
    if (out.halves.relation == HalvesRelation::kInconsistent) {
        out.drift_verdict = DriftVerdict::kUnknown;
    }

    return out;
}

}  // namespace kivo::video::core::pace
