// halves_tests.cpp — P13-O tests: 两个半程斜率"差多少算差"
//
// 这组测试守的是一个布尔说过三次谎之后换上的三态判据。三件事必须被钉死：
//   ① 判据是 `gap > 1.96 × pooled`，不是"置信区间端点相交"；
//   ② 任何一侧算不出误差 ⇒ `unknown`，**不许**退化成 `consistent`
//      （NaN 与任何数比较都是 false，所以漏查 NaN 会让"算不出来"直接变成"一致"）；
//   ③ `cumulative_gap_ns` 用两半程**各自的真实跨度**，不用"半程时长"假设等长。
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_drift_trend.hpp"
#include "video/video_plane/core/pace/pace_halves_agreement.hpp"

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

/// 浮点比较一律走相对容差：这些量全是从 sqrt / 乘法算出来的，
/// 用 `==` 只会让测试在换编译器时随机变红，而那和判据无关。
[[nodiscard]] bool near(double a, double b, double tol = 1e-9) {
    return std::fabs(a - b) <= tol * (1.0 + std::fabs(a) + std::fabs(b));
}

constexpr std::int64_t kSpan130s = 130'000'000'000;
constexpr std::int64_t kBudgetNs = 45'000'000;

[[nodiscard]] pace::HalvesFit fit(bool estimated, double slope_ppm, double se_ppm,
                                  std::int64_t span_ns) {
    pace::HalvesFit out;
    out.estimated = estimated;
    out.slope_ppm = slope_ppm;
    out.std_error_ppm = se_ppm;
    out.span_ns = span_ns;
    return out;
}

// ---- O1 差在噪声之内 ⇒ consistent ---------------------------------------
void t1_consistent_when_gap_is_within_noise() {
    const auto out = pace::compare_halves(fit(true, 1.0, 0.1, 65'000'000'000),
                                          fit(true, 1.2, 0.2, 65'000'000'000),
                                          kSpan130s, kBudgetNs);
    CHECK(out.estimated);
    CHECK(out.relation == pace::HalvesRelation::kConsistent);
    CHECK(near(out.gap_ppm, 0.2, 1e-12));
    // pooled = sqrt(0.1² + 0.2²) = sqrt(0.05)
    CHECK(near(out.pooled_error_ppm, std::sqrt(0.05), 1e-12));
    CHECK(near(out.z, 0.2 / std::sqrt(0.05), 1e-12));
    CHECK(out.resolution_sufficient);
}

// ---- O2 差超出噪声 ⇒ inconsistent ---------------------------------------
//
// 用 p13n-fwd-r1 的真实形状：+0.5975 / +0.0276。同号，但差 21 倍。
void t2_inconsistent_when_gap_exceeds_noise() {
    const auto out = pace::compare_halves(fit(true, 0.5975, 0.05, 65'000'000'000),
                                          fit(true, 0.0276, 0.05, 65'000'000'000),
                                          kSpan130s, kBudgetNs);
    CHECK(out.relation == pace::HalvesRelation::kInconsistent);
    CHECK(near(out.gap_ppm, 0.5699, 1e-12));
    CHECK(out.z > 1.96);
    // 三态与 z 必须自洽（O5）
    CHECK((out.z > 1.96) == (out.relation == pace::HalvesRelation::kInconsistent));
}

// ---- O3 一侧没估出来 ⇒ unknown，不许退化成 consistent -------------------
void t3_unknown_when_one_side_not_estimated() {
    const auto out = pace::compare_halves(fit(false, 1.0, 0.0, 65'000'000'000),
                                          fit(true, 3.0, 0.1, 65'000'000'000),
                                          kSpan130s, kBudgetNs);
    CHECK(!out.estimated);
    CHECK(out.relation == pace::HalvesRelation::kUnknown);
    // 误差那一组必须是 0（不可读），而不是"算出来是 0"。
    CHECK(out.pooled_error_ppm == 0.0);
    CHECK(out.resolution_sufficient == false);
    // 但斜率之差仍然可读：它只依赖斜率。
    CHECK(near(out.gap_ppm, 2.0, 1e-12));
}

// ---- O4 NaN / 负 SE 也必须落到 unknown ----------------------------------
//
// 这是本文件最危险的一条路：`NaN > 1.96*pooled` 为 false，
// 于是"算不出来"会被判成 `consistent` —— 一次失败伪装成一次通过。
void t4_non_finite_or_negative_error_is_unknown() {
    const double nan = std::nan("");
    const auto nan_case = pace::compare_halves(fit(true, 1.0, nan, 65'000'000'000),
                                               fit(true, 1.0, 0.1, 65'000'000'000),
                                               kSpan130s, kBudgetNs);
    CHECK(!nan_case.estimated);
    CHECK(nan_case.relation == pace::HalvesRelation::kUnknown);

    const auto inf_case = pace::compare_halves(fit(true, 1.0, INFINITY, 65'000'000'000),
                                               fit(true, 1.0, 0.1, 65'000'000'000),
                                               kSpan130s, kBudgetNs);
    CHECK(!inf_case.estimated);
    CHECK(inf_case.relation == pace::HalvesRelation::kUnknown);

    const auto neg_case = pace::compare_halves(fit(true, 1.0, -0.1, 65'000'000'000),
                                               fit(true, 1.0, 0.1, 65'000'000'000),
                                               kSpan130s, kBudgetNs);
    CHECK(!neg_case.estimated);
    CHECK(neg_case.relation == pace::HalvesRelation::kUnknown);
}

// ---- O5 置信带 = slope ± 1.96·SE，且 pooled 可复算 ----------------------
void t5_ci_and_pooled_are_recomputable() {
    const auto out = pace::compare_halves(fit(true, 1.0, 0.1, 65'000'000'000),
                                          fit(true, -2.0, 0.3, 65'000'000'000),
                                          kSpan130s, kBudgetNs);
    const double z = 1.96;
    CHECK(near(out.first_ci_low_ppm, 1.0 - z * 0.1, 1e-12));
    CHECK(near(out.first_ci_high_ppm, 1.0 + z * 0.1, 1e-12));
    CHECK(near(out.second_ci_low_ppm, -2.0 - z * 0.3, 1e-12));
    CHECK(near(out.second_ci_high_ppm, -2.0 + z * 0.3, 1e-12));
    CHECK(near(out.pooled_error_ppm, std::sqrt(0.01 + 0.09), 1e-12));
    // 反号也是"差得远"，不该被当成"中间掉头"单独豁免。
    CHECK(out.relation == pace::HalvesRelation::kInconsistent);
}

// ---- O6 pooled == 0 是合法输入：结论仍然有定义 ---------------------------
//
// 两侧都是完美直线时残差恒为 0，`gap/pooled` 是 0/0。
// 让 NaN 顺流而下会让这种"最干净"的证据反而判不出来。
void t6_zero_pooled_still_concludes() {
    const auto same = pace::compare_halves(fit(true, 1.0, 0.0, 65'000'000'000),
                                           fit(true, 1.0, 0.0, 65'000'000'000),
                                           kSpan130s, kBudgetNs);
    CHECK(same.estimated);
    CHECK(same.z == 0.0);
    CHECK(same.relation == pace::HalvesRelation::kConsistent);

    const auto differ = pace::compare_halves(fit(true, 1.0, 0.0, 65'000'000'000),
                                             fit(true, 2.0, 0.0, 65'000'000'000),
                                             kSpan130s, kBudgetNs);
    CHECK(std::isinf(differ.z));
    CHECK(differ.relation == pace::HalvesRelation::kInconsistent);
}

// ---- O7 cumulative_gap_ns 用各自的真实跨度 ------------------------------
//
// 两半程长度由整数除法决定，末尾那个点的归属会让两者差一格。
// 用"半程时长"假设等长，会把这个差悄悄算进结论里。
void t7_cumulative_gap_uses_each_span() {
    // 前半 1 ppm × 65 s = 65 000 ns；后半 −0.5 ppm × 70 s = −35 000 ns。
    const auto out = pace::compare_halves(fit(true, 1.0, 0.1, 65'000'000'000),
                                          fit(true, -0.5, 0.1, 70'000'000'000),
                                          kSpan130s, kBudgetNs);
    CHECK(out.cumulative_gap_ns == 100'000);
    // 若错用"两段都 65 s"，会得到 |65000 + 32500| = 97500 —— 差 2.5 µs。
    CHECK(out.cumulative_gap_ns != 97'500);
}

// ---- O8 SE 估不出来时，累积差仍然要出版 ---------------------------------
//
// 「知道差多少但不知道准不准」是真实存在的情形。把两者绑在同一个 flag 上，
// 会让这种证据连差多少都读不到。
void t8_cumulative_gap_survives_missing_error() {
    const auto out = pace::compare_halves(fit(false, 1.0, 0.0, 65'000'000'000),
                                          fit(false, 0.0, 0.0, 65'000'000'000),
                                          kSpan130s, kBudgetNs);
    CHECK(!out.estimated);
    CHECK(out.relation == pace::HalvesRelation::kUnknown);
    CHECK(out.cumulative_gap_ns == 65'000);
    CHECK(near(out.gap_ppm, 1.0, 1e-12));
}

// ---- O9 resolution_sufficient 的三种情形 --------------------------------
void t9_resolution_sufficiency() {
    // 130 s 上额度等效 45e6/130e9×1e6 ≈ 346.15 ppm；resolution 0.438 ≪ 346 ⇒ 够。
    const auto enough = pace::compare_halves(fit(true, 1.0, 0.1, 65'000'000'000),
                                             fit(true, 1.2, 0.2, 65'000'000'000),
                                             kSpan130s, kBudgetNs);
    CHECK(near(enough.budget_equivalent_ppm, kBudgetNs * 1e6 / static_cast<double>(kSpan130s),
               1e-12));
    CHECK(enough.resolution_sufficient);

    // 把尺子做粗：SE 各 100 ppm ⇒ resolution ≈ 277 ppm，仍小于 346；
    // 抬到 200 ppm ⇒ 554 ppm > 346 ⇒ 不够。
    const auto coarse = pace::compare_halves(fit(true, 1.0, 200.0, 65'000'000'000),
                                             fit(true, 1.2, 200.0, 65'000'000'000),
                                             kSpan130s, kBudgetNs);
    CHECK(!coarse.resolution_sufficient);
    CHECK(near(coarse.resolution_ppm, 1.96 * std::sqrt(200.0 * 200.0 * 2), 1e-9));

    // 跨度为 0 ⇒ 换算没有定义，必须 false（"无法判断够不够" ≠ "够"）。
    const auto degenerate = pace::compare_halves(fit(true, 1.0, 0.1, 0),
                                                 fit(true, 1.2, 0.2, 0),
                                                 0, kBudgetNs);
    CHECK(!degenerate.resolution_sufficient);
    CHECK(degenerate.budget_equivalent_ppm == 0.0);
}

// ---- O10 三态名字逐字稳定（改一个字节就是换 schema）--------------------
void t10_relation_names_are_stable() {
    CHECK(std::string(pace::halves_relation_name(pace::HalvesRelation::kConsistent)) ==
          "consistent");
    CHECK(std::string(pace::halves_relation_name(pace::HalvesRelation::kInconsistent)) ==
          "inconsistent");
    CHECK(std::string(pace::halves_relation_name(pace::HalvesRelation::kUnknown)) == "unknown");
}

// ── 端到端：`summarize_drift` 会不会真的把 verdict 降级 ──
//
// 前 10 个用例测的是 `compare_halves` 这个纯函数。但卡片真正的承诺是
// **`kInconsistent ⇒ drift_verdict = unknown`** —— 那条规则写在
// `pace_drift_trend.cxx` 里，纯函数测试碰不到它。真机四轮又恰好全是
// `consistent`（见 85-p13o-status.md §四），所以这条分支只能靠造序列覆盖。
//
// 不覆盖它的后果很具体：一份"两半程不是一条线"的证据照样报
// `within_budget`，而那个 `within_budget` 是拿一条错的直线乘出来的。

template <typename Fn>
void feed(pace::DriftSeries& series, std::int64_t duration_ns, std::int64_t step_ns, Fn lateness) {
    for (std::int64_t pts = 0; pts <= duration_ns; pts += step_ns) {
        series.note(pts, lateness(pts));
    }
}

// O11 单斜率（两半程同一条线）⇒ consistent，verdict 不被降级
void t11_single_slope_stays_consistent() {
    pace::DriftSeries series;
    // 0.5 ppm 的匀速漂移 + 一点确定性噪声（避免残差恒 0，让 HAC 有东西可估）。
    feed(series, 60'000'000'000, 50'000'000, [](std::int64_t pts) {
        const double drift = 0.5 * static_cast<double>(pts) / 1e6;  // ppm × ns / 1e6
        const double noise = static_cast<double>((pts / 50'000'000) % 7 - 3) * 50.0;
        return static_cast<std::int64_t>(drift + noise);
    });
    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.usable);
    CHECK(trend.halves.estimated);
    CHECK(trend.halves.relation == pace::HalvesRelation::kConsistent);
    // 同一条线 ⇒ 不该被降级。它可以是 within/over，但不能是 unknown。
    CHECK(trend.drift_verdict != pace::DriftVerdict::kUnknown);
}

// O12 两段速率差 10 倍 ⇒ inconsistent，且 verdict 必须降为 unknown
void t12_rate_change_downgrades_verdict() {
    pace::DriftSeries series;
    // 前 30 s 走 0.2 ppm，后 30 s 走 4 ppm：两个半程差 3.8 ppm，
    // 而噪声只有几十 ns —— 这个差远超噪声，必须判 inconsistent。
    feed(series, 60'000'000'000, 50'000'000, [](std::int64_t pts) {
        const std::int64_t half = 30'000'000'000;
        const double slope_ppm = pts < half ? 0.2 : 4.0;
        const double drift = slope_ppm * static_cast<double>(pts) / 1e6;
        const double noise = static_cast<double>((pts / 50'000'000) % 5 - 2) * 40.0;
        return static_cast<std::int64_t>(drift + noise);
    });
    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.usable);
    CHECK(trend.halves.estimated);
    CHECK(trend.halves.relation == pace::HalvesRelation::kInconsistent);
    // 这条就是卡片的承诺。
    CHECK(trend.drift_verdict == pace::DriftVerdict::kUnknown);
    // 降级不是因为"没样本"：斜率、累积量都算出来了，只是那条直线描述错了对象。
    CHECK(trend.slope_ppm > 0.0);
    CHECK(trend.halves.cumulative_gap_ns > 0);
}

struct TestCase {
    const char* name;
    void (*run)();
};

// ---- O13 完美直线：z 很大，但差得不够多 ---------------------------------
//
// 这一条来自 `p13l` 的预算单测（P13-P 撞到）：一份完美线性的 20 ppm 序列，
// 两侧残差只剩浮点噪声，SE ≈ 3.85e-15 ppm，于是 2.5e-14 ppm 的差也是 4.6 σ。
// 只看 z，`relation` 会被判成 `kInconsistent`，`drift_verdict` 随之降成
// `unknown` —— **最干净的证据反而报不出 `within_budget`**。
//
// 所以结论要过两道门：统计上分得开（z）+ 材料上差得起（累积 ns ≥ 1 µs）。
// 这一条同时钉住两件事：z 必须 > 1.96（证明第一道门确实放行了），
// 而 relation 必须是 `kConsistent`（证明第二道门拦住了）。
void t13_noise_level_gap_is_not_inconsistent() {
    const auto out = pace::compare_halves(fit(true, 20.0, 3.85e-15, 17'475'000'000),
                                          fit(true, 20.0 + 2.5e-14, 3.85e-15, 17'475'000'000),
                                          kSpan130s, kBudgetNs);
    CHECK(out.estimated);
    CHECK(out.z > 1.96);                       // 统计上"分得开"
    CHECK(out.cumulative_gap_ns < pace::kMinCumulativeGapNs);  // 材料上"差不起"
    CHECK(out.relation == pace::HalvesRelation::kConsistent);
}

const TestCase kCases[] = {
    {"O1 差在噪声内 = consistent", t1_consistent_when_gap_is_within_noise},
    {"O2 差超出噪声 = inconsistent", t2_inconsistent_when_gap_exceeds_noise},
    {"O3 一侧未估出 = unknown", t3_unknown_when_one_side_not_estimated},
    {"O4 NaN/负 SE = unknown", t4_non_finite_or_negative_error_is_unknown},
    {"O5 CI 与 pooled 可复算", t5_ci_and_pooled_are_recomputable},
    {"O6 pooled=0 仍有结论", t6_zero_pooled_still_concludes},
    {"O7 累积差用各自跨度", t7_cumulative_gap_uses_each_span},
    {"O8 无误差时累积差仍出版", t8_cumulative_gap_survives_missing_error},
    {"O9 resolution 三种情形", t9_resolution_sufficiency},
    {"O10 三态名字稳定", t10_relation_names_are_stable},
    {"O11 单斜率不降级 verdict", t11_single_slope_stays_consistent},
    {"O12 速率突变降级 verdict", t12_rate_change_downgrades_verdict},
    {"O13 浮点噪声级差异不判不一致", t13_noise_level_gap_is_not_inconsistent},
};

}  // namespace

int main() {
    std::printf("=== P13-O halves agreement tests ===\n");
    for (const auto& test_case : kCases) {
        const int failed_before = g_failed;
        test_case.run();
        std::printf("%s %s\n", g_failed == failed_before ? "[PASS]" : "[FAIL]", test_case.name);
    }
    std::printf("--- 断言 %d 项，失败 %d 项 ---\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
