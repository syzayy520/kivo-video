// budget_tests.cpp — P13-L tests: 把"看得见"翻译成"要紧不要紧"
//
// 这一组守四句话：
//   ① **分得开 ≠ 要紧。** CI 不含 0 而累积量只有十几微秒的情形必须被判
//      `within_budget` —— 做不到这条，`slope_detectable` 就还是会被读成"要出事"。
//   ② **反方向同样要成立。** CI 含 0（看不见）时，远端积起来仍可能越过额度。
//      只守 ① 会让这个单元变成"detectable 的复读机"。
//   ③ **上界只增不减。** 保守上界必须 ≥ 点估计的绝对值；它取的是置信带远端，
//      不是把点估计放大一个系数。
//   ④ **算不出来不等于没问题。** 非有限输入、负 SE、样本不足，一律 `unknown`，
//      不许落进 `within_budget` —— 那是让一次失败伪装成一次通过。
//
// 纯桌面计算：没有设备、没有时钟、没有睡眠。
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "video/video_plane/core/pace/pace_drift_budget.hpp"
#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_drift_trend.hpp"

namespace pace = kivo::video::core::pace;

namespace {

int g_checked = 0;
int g_failed = 0;

#define CHECK(expr)                                                        \
    do {                                                                   \
        ++g_checked;                                                       \
        if (!(expr)) {                                                     \
            ++g_failed;                                                    \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #expr);  \
        }                                                                  \
    } while (false)

constexpr std::int64_t kMs = 1'000'000;
constexpr std::int64_t kUs = 1'000;

bool near(double a, double b, double tol) {
    return std::fabs(a - b) <= tol;
}

/// 喂一条线性 lateness 的序列。`per_sample_ns` 是每个抽稀样本的增量，
/// 样本间隔 50 ms ⇒ 斜率 = per_sample_ns / 50 ms。
void feed_linear(pace::DriftSeries& series, int count, std::int64_t per_sample_ns) {
    std::int64_t media = 0;
    for (int i = 0; i < count; ++i) {
        series.note(media, per_sample_ns * static_cast<std::int64_t>(i));
        media += pace::DriftSeries::kSampleIntervalNs;
    }
}

// ① 额度与线格式名字。三个名字逐字钉死 —— 改一个字节就是换了 schema。
void t01_the_budget_and_its_names_are_literal() {
    CHECK(pace::kAudibleThresholdNs == 45 * kMs);
    CHECK(near(pace::kConfidenceZ, 1.96, 1e-12));
    CHECK(std::strcmp(pace::drift_verdict_name(pace::DriftVerdict::kWithinBudget),
                      "within_budget") == 0);
    CHECK(std::strcmp(pace::drift_verdict_name(pace::DriftVerdict::kOverBudget),
                      "over_budget") == 0);
    CHECK(std::strcmp(pace::drift_verdict_name(pace::DriftVerdict::kUnknown), "unknown") == 0);
}

// ② 零斜率零误差：三个零 + 未超额度。这是"真的没漂"，不是"算不出来"。
void t02_nothing_drifting_is_within_budget_not_unknown() {
    const pace::DriftAccumulation acc = pace::accumulate_drift(0.0, 0.0, 100'000'000'000LL, true);
    CHECK(acc.estimated);
    CHECK(acc.accumulated_ns == 0);
    CHECK(acc.accumulated_upper_ns == 0);
    CHECK(acc.threshold_ns == pace::kAudibleThresholdNs);
    CHECK(acc.verdict == pace::DriftVerdict::kWithinBudget);
}

// ③ 点估计手算：1 ppm × 100 s = 100 µs。ppm 是 µs/s，这条把单位钉死。
void t03_one_ppm_over_hundred_seconds_is_a_hundred_microseconds() {
    const pace::DriftAccumulation acc =
        pace::accumulate_drift(1.0, 0.0, 100'000'000'000LL, true);
    CHECK(acc.estimated);
    CHECK(acc.accumulated_ns == 100 * kUs);
    CHECK(acc.accumulated_upper_ns == 100 * kUs);
    CHECK(acc.verdict == pace::DriftVerdict::kWithinBudget);
}

// ④ 负斜率：点估计带符号，上界不带。方向不该影响"会不会被听出来"。
void t04_negative_slope_keeps_a_signed_estimate_and_a_positive_bound() {
    // −1001 ppm × 45 s = −45.045 ms：超过额度。
    const pace::DriftAccumulation acc =
        pace::accumulate_drift(-1001.0, 0.0, 45'000'000'000LL, true);
    CHECK(acc.estimated);
    CHECK(acc.accumulated_ns == -45'045'000);
    CHECK(acc.accumulated_upper_ns == 45'045'000);
    CHECK(acc.verdict == pace::DriftVerdict::kOverBudget);
}

// ⑤ 上界只增不减：保守上界必须 ≥ 点估计的绝对值。
//    否则"最坏能坏到哪"这句话比点估计还乐观，它就不叫上界了。
void t05_the_upper_bound_never_undercuts_the_point_estimate() {
    double seed = 0.5;
    for (int i = 0; i < 50; ++i) {
        // 确定性地铺一批斜率与 SE，从亚 ppm 到几十 ppm。
        seed = seed * 1.37 + 0.11;
        const double slope = (seed - 1.0) * 40.0;
        const double se = std::fabs(seed) * 3.0;
        const std::int64_t span = 30'000'000'000LL + static_cast<std::int64_t>(i) * 1'000'000'000LL;
        const pace::DriftAccumulation acc = pace::accumulate_drift(slope, se, span, true);
        CHECK(acc.estimated);
        if (!acc.estimated) {
            continue;
        }
        const std::int64_t point_abs =
            acc.accumulated_ns < 0 ? -acc.accumulated_ns : acc.accumulated_ns;
        CHECK(acc.accumulated_upper_ns >= point_abs);
        CHECK(acc.accumulated_upper_ns >= 0);
    }
}

// ⑥ 开闭区间：恰好 45 ms 算"未超过"。这一处口径必须只有一个答案。
void t06_exactly_at_the_budget_is_still_within() {
    // 1000 ppm × 45 s = 45 ms，正好压在额度上。
    const pace::DriftAccumulation on_edge =
        pace::accumulate_drift(1000.0, 0.0, 45'000'000'000LL, true);
    CHECK(on_edge.accumulated_upper_ns == pace::kAudibleThresholdNs);
    CHECK(on_edge.verdict == pace::DriftVerdict::kWithinBudget);

    // 再多 1 ppm（45 µs 累积）就越过去了。
    const pace::DriftAccumulation over =
        pace::accumulate_drift(1001.0, 0.0, 45'000'000'000LL, true);
    CHECK(over.accumulated_upper_ns > pace::kAudibleThresholdNs);
    CHECK(over.verdict == pace::DriftVerdict::kOverBudget);
}

// ⑦ 本卡存在的理由（反向）：**看不见，但可能听得出来。**
//    CI 含 0 ⇒ `slope_detectable` 为假，而 CI 远端积起来 80 ms。
//    若 verdict 是从 detectable 抄来的，这条会红。
void t07_an_invisible_drift_can_still_be_audible() {
    // 0 ± 1.96·400 ppm，跨度 102 s：上界 = 784 ppm × 102 s ≈ 80 ms。
    const pace::DriftAccumulation acc = pace::accumulate_drift(0.0, 400.0, 102'000'000'000LL, true);
    CHECK(acc.estimated);
    CHECK(acc.accumulated_ns == 0);
    CHECK(acc.accumulated_upper_ns > pace::kAudibleThresholdNs);
    CHECK(acc.verdict == pace::DriftVerdict::kOverBudget);
}

// ⑧ 沙盒边界：非有限输入 / 负 SE / 不前进的跨度，一律算不出来。
void t08_invalid_inputs_are_not_silently_zeroed() {
    const double nan = std::nan("");
    const double inf = 1e308 * 10.0;

    CHECK(!pace::accumulate_drift(nan, 0.0, 1'000'000'000LL, true).estimated);
    CHECK(!pace::accumulate_drift(1.0, nan, 1'000'000'000LL, true).estimated);
    CHECK(!pace::accumulate_drift(inf, 0.0, 1'000'000'000LL, true).estimated);
    // 负 SE：half 会变负、CI 两端交换，错误会被 `farthest` 悄悄吞掉。
    CHECK(!pace::accumulate_drift(1.0, -0.5, 1'000'000'000LL, true).estimated);
    CHECK(!pace::accumulate_drift(1.0, 0.0, 0, true).estimated);
    CHECK(!pace::accumulate_drift(1.0, 0.0, -1, true).estimated);

    for (const pace::DriftAccumulation& acc :
         {pace::accumulate_drift(nan, 0.0, 1'000'000'000LL, true),
          pace::accumulate_drift(1.0, -0.5, 1'000'000'000LL, true)}) {
        CHECK(acc.verdict == pace::DriftVerdict::kUnknown);
        CHECK(acc.accumulated_ns == 0);
        CHECK(acc.accumulated_upper_ns == 0);
    }
}

// ⑨ 趋势不可用：数照算，判定不给。
//    `unknown` 不是"漂移很小"，是"这份证据没资格说漂移大不大"。
void t09_unusable_trend_keeps_the_numbers_but_withholds_the_verdict() {
    const pace::DriftAccumulation acc =
        pace::accumulate_drift(1000.0, 0.0, 20'000'000'000LL, /*usable=*/false);
    CHECK(acc.estimated);  // 20 s × 1000 ppm = 20 ms，算得出来
    CHECK(acc.accumulated_ns == 20 * kMs);
    CHECK(acc.verdict == pace::DriftVerdict::kUnknown);
}

// ⑩ 端到端：真序列走 `summarize_drift`，`drift_*` 必须与手算一致。
//    两个方向都验 —— 小漂移进去是 within，大漂移进去是 over。
void t10_summarize_drift_fills_the_budget_fields() {
    // 20 ppm：700 个样本、699 个 50 ms 间隔 ⇒ 跨度 34.95 s，累积 699 µs。
    {
        pace::DriftSeries series;
        feed_linear(series, 700, 1'000);  // 1 µs / 样本 = 20 ppm
        const pace::DriftTrend trend = pace::summarize_drift(series);
        CHECK(trend.usable);
        CHECK(near(trend.slope_ppm, 20.0, 1e-6));
        CHECK(trend.drift_budget_ns == pace::kAudibleThresholdNs);
        CHECK(near(static_cast<double>(trend.drift_accumulated_ns), 699'000.0, 2.0));
        CHECK(trend.drift_accumulated_upper_ns ==
              static_cast<std::int64_t>(std::llabs(trend.drift_accumulated_ns)));
        CHECK(trend.drift_verdict == pace::DriftVerdict::kWithinBudget);
        // 这一条是本卡的整个论点：完美线性 ⇒ 看得见，但只有 0.7 ms。
        CHECK(trend.slope_detectable);
    }
    // 2000 ppm：同样的 700 个样本 ⇒ 累积 69.9 ms，越过 45 ms。
    {
        pace::DriftSeries series;
        feed_linear(series, 700, 100'000);  // 100 µs / 样本 = 2000 ppm
        const pace::DriftTrend trend = pace::summarize_drift(series);
        CHECK(trend.usable);
        CHECK(near(trend.slope_ppm, 2000.0, 1e-6));
        CHECK(near(static_cast<double>(trend.drift_accumulated_ns), 69'900'000.0, 5.0));
        CHECK(trend.drift_verdict == pace::DriftVerdict::kOverBudget);
    }
}

// ⑪ 样本不足的趋势：`drift_accumulated_ns` 不许因为提前返回而留成 0，
//    否则一份 20 点的证据会写着"斜率 2000 ppm、累积 0 ns"，两句自相矛盾。
void t11_short_series_still_reports_its_accumulation() {
    pace::DriftSeries series;
    feed_linear(series, 20, 100'000);  // 20 点 = 0.95 s 跨度，远不到 30 s
    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(!trend.usable);
    CHECK(trend.unusable_reason != nullptr && trend.unusable_reason[0] != '\0');
    // 19 个间隔 × 100 µs = 1.9 ms。
    CHECK(near(static_cast<double>(trend.drift_accumulated_ns), 1'900'000.0, 2.0));
    CHECK(trend.drift_accumulated_ns != 0);
    CHECK(trend.drift_verdict == pace::DriftVerdict::kUnknown);
}

}  // namespace

int main() {
    t01_the_budget_and_its_names_are_literal();
    t02_nothing_drifting_is_within_budget_not_unknown();
    t03_one_ppm_over_hundred_seconds_is_a_hundred_microseconds();
    t04_negative_slope_keeps_a_signed_estimate_and_a_positive_bound();
    t05_the_upper_bound_never_undercuts_the_point_estimate();
    t06_exactly_at_the_budget_is_still_within();
    t07_an_invisible_drift_can_still_be_audible();
    t08_invalid_inputs_are_not_silently_zeroed();
    t09_unusable_trend_keeps_the_numbers_but_withholds_the_verdict();
    t10_summarize_drift_fills_the_budget_fields();
    t11_short_series_still_reports_its_accumulation();

    std::printf("p13l budget tests: %d checks, %d failed\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
