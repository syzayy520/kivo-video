// hac_tests.cpp — P13-K tests: 给斜率的标准误换一把承认自相关的尺子
//
// 这一组守四句话：
//   ① **残差正相关时，独立同分布假设会低估 SE。** AR(1) 残差（φ = 0.9）下
//      HAC 的 SE 必须明显大于 IID 的 —— 做不到这条，这个单元就只是换了个公式。
//   ② **白噪声下两者必须接近。** 只在正相关时才变大、别的时候不变，
//      才说明膨胀比来自自相关而不是来自实现本身。
//   ③ **同一份数据，HAC 必须能被独立复算。** 测试里另写一份最朴素的实现
//      （把残差全存进 vector 再逐 lag 求和），两边逐位对齐 —— 否则"可复算"
//      只是一句声称，证据里那些 `hac_lags` 也成了装饰。
//   ④ **半程斜率分得开"持续漂移"和"中途跳一下"。** 两者全序列斜率一样，
//      结论却相反，而这条区别没有任何一个单点指标能回答。
//
// 纯桌面计算：没有设备、没有时钟、没有睡眠。
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_drift_trend.hpp"
#include "video/video_plane/core/pace/pace_hac.hpp"

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
constexpr std::int64_t kStep = pace::DriftSeries::kSampleIntervalNs;

template <typename Fn>
void feed(pace::DriftSeries& series, int count, Fn lateness_of) {
    std::int64_t media = 0;
    for (int i = 0; i < count; ++i) {
        series.note(media, lateness_of(i));
        media += kStep;
    }
}

/// xorshift64：确定性、无平台差异。
struct Rng {
    std::uint64_t state;
    std::uint64_t next() noexcept {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    }
    /// 近似正态（12 个均匀量求和），半宽约 `half_range_ns × 3`。
    double normal(double sigma_ns) noexcept {
        double acc = 0.0;
        for (int i = 0; i < 12; ++i) {
            acc += static_cast<double>(next() % 1000) / 1000.0;
        }
        return (acc - 6.0) * sigma_ns;
    }
};

bool near(double value, double expected, double tolerance) {
    const double diff = value - expected;
    return (diff < 0 ? -diff : diff) <= tolerance;
}

/// AR(1) 残差：`e_i = φ·e_{i−1} + w_i`。这是真机上 `lateness` 的粗模型 ——
/// vsync 相位、CPU 调频、抽稀本身抹平慢变分量，都会让相邻样本同向。
void feed_ar1(pace::DriftSeries& series, int count, double phi, double noise_sigma_ns,
              std::uint64_t seed) {
    Rng rng{seed};
    double previous = 0.0;
    std::int64_t media = 0;
    for (int i = 0; i < count; ++i) {
        previous = phi * previous + rng.normal(noise_sigma_ns);
        series.note(media, static_cast<std::int64_t>(previous));
        media += kStep;
    }
}

/// 测试专用的朴素 HAC：把残差整条存下来，再逐 lag 老老实实求和。
///
/// 与实现刻意走不同的路（那边不占数组、每个 lag 现算残差），
/// 两条路给出同一个数才叫"可复算"。
double naive_hac_std_error(const pace::DriftSeries& series, double slope_per_ns,
                           std::int32_t lags) {
    const std::int32_t n = series.size();
    const std::int64_t origin = series.at(0).media_ns;
    double sum_t = 0.0;
    double sum_y = 0.0;
    for (std::int32_t i = 0; i < n; ++i) {
        sum_t += static_cast<double>(series.at(i).media_ns - origin);
        sum_y += static_cast<double>(series.at(i).lateness_ns);
    }
    const double mean_t = sum_t / static_cast<double>(n);
    const double mean_y = sum_y / static_cast<double>(n);
    const double intercept = mean_y - slope_per_ns * mean_t;

    std::vector<double> u(static_cast<std::size_t>(n), 0.0);
    double sxx = 0.0;
    for (std::int32_t i = 0; i < n; ++i) {
        const double t = static_cast<double>(series.at(i).media_ns - origin);
        const double x = t - mean_t;
        const double e = static_cast<double>(series.at(i).lateness_ns) -
                         (intercept + slope_per_ns * t);
        sxx += x * x;
        u[static_cast<std::size_t>(i)] = x * e;
    }

    double kernel = 0.0;
    for (std::int32_t i = 0; i < n; ++i) {
        kernel += u[static_cast<std::size_t>(i)] * u[static_cast<std::size_t>(i)];
    }
    for (std::int32_t lag = 1; lag <= lags; ++lag) {
        double acc = 0.0;
        for (std::int32_t i = 0; i + lag < n; ++i) {
            acc += u[static_cast<std::size_t>(i)] * u[static_cast<std::size_t>(i + lag)];
        }
        kernel += 2.0 * (1.0 - static_cast<double>(lag) / (static_cast<double>(lags) + 1.0)) *
                  acc;
    }
    const double dof = static_cast<double>(n) / static_cast<double>(n - 2);
    const double variance = dof * kernel / (sxx * sxx);
    return variance > 0.0 ? std::sqrt(variance) : 0.0;
}

// ① 带宽规则本身。钉死几个点，是因为证据里的 `hac_lags` 要靠它复算（判据 K1）——
//    规则一变，历史证据里的每个 L 都会变成无法验证的数。
void t01_bartlett_bandwidth_rule() {
    CHECK(pace::bartlett_lags(0) == 0);
    CHECK(pace::bartlett_lags(3) == 1);      // 0.03^(2/9) ≈ 0.459 → 1.83 → 1
    CHECK(pace::bartlett_lags(30) == 3);     // 0.3^(2/9)  ≈ 0.765 → 3.06 → 3
    CHECK(pace::bartlett_lags(100) == 4);    // 恰好 4.0（这条是规则的定义点）
    CHECK(pace::bartlett_lags(700) == 6);    // 7^(2/9)  ≈ 1.541 → 6.16 → 6
    CHECK(pace::bartlett_lags(2048) == 7);   // 20.48^(2/9) ≈ 1.956 → 7.82 → 7
    // 单调不减：片源变长时带宽不该往回缩。
    CHECK(pace::bartlett_lags(4096) >= pace::bartlett_lags(2048));
}

// ② φ = 0.9 的 AR(1) 残差：HAC 的 SE 必须明显大于 IID 的。
//
//    这是本单元存在的**唯一理由**。若这条不过，整个 P13-K 就是白干。
//    理论膨胀比（方差）≈ (1+φ)/(1−φ) = 19，被 Bartlett 截断到 L=6 后约 5~6，
//    对应 SE 膨胀约 2.3~2.5。这里不钉死具体值，只要求"明显更大"，
//    因为膨胀比取决于噪声实现，钉死它等于把随机数种子写进判据。
void t02_positive_autocorrelation_inflates_the_error() {
    pace::DriftSeries series;
    feed_ar1(series, 800, 0.9, 40'000.0, 0x9E3779B97F4A7C15ULL);

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.hac_estimated);
    CHECK(trend.hac_lags == pace::bartlett_lags(800));
    CHECK(trend.hac_lag1_autocorrelation > 0.5);  // 正相关必须被看见
    CHECK(trend.slope_ppm_std_error > trend.slope_ppm_std_error_iid);
    CHECK(trend.slope_ppm_std_error_inflation > 2.0);
    // 膨胀比就是两个 SE 相除 —— 它是**可复算**的，不是另一个独立结论。
    CHECK(near(trend.slope_ppm_std_error_inflation,
               trend.slope_ppm_std_error / trend.slope_ppm_std_error_iid, 1e-9));
}

// ③ 白噪声：两条路必须接近。只在正相关时才变大、别的时候不动，
//    才说明那个膨胀比来自自相关，而不是来自"我们的实现本来就偏大"。
void t03_white_noise_leaves_the_error_roughly_unque() {
    pace::DriftSeries series;
    feed_ar1(series, 800, 0.0, 40'000.0, 0xD1B54A32D192ED03ULL);

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.hac_estimated);
    CHECK(trend.hac_lag1_autocorrelation > -0.15);
    CHECK(trend.hac_lag1_autocorrelation < 0.15);
    // 允许一定偏离：L=6 的核在有限样本上本来就不是严格 1.0。
    CHECK(trend.slope_ppm_std_error_inflation > 0.5);
    CHECK(trend.slope_ppm_std_error_inflation < 2.0);
}

// ④ 负相关（交替符号）：HAC 应当**小于** IID。这条守的是方向 ——
//    如果实现只是无脑放大，负相关那侧会一起变大，那就成了"保守"而非"正确"。
void t04_negative_autocorrelation_shrinks_the_error() {
    pace::DriftSeries series;
    feed(series, 800, [](int i) {
        // 交替 ±20 µs，唯一的方向性就是负自相关。
        return (i % 2 == 0) ? (20 * kUs) : (-20 * kUs);
    });

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.hac_estimated);
    CHECK(trend.hac_lag1_autocorrelation < -0.5);
    CHECK(trend.slope_ppm_std_error < trend.slope_ppm_std_error_iid);
    CHECK(trend.slope_ppm_std_error_inflation < 0.5);
}

// ⑤ 独立复算：另写一份朴素实现，两边必须逐位对齐。
//
//    没有这条，证据里的 `hac_lags` / `hac_lag1_autocorrelation` 就只是
//    "实现说了算"的两个数 —— 而这份项目反复栽的正是"校验器复算的是同一个公式"。
void t05_hac_is_recomputable_by_a_naive_implementation() {
    pace::DriftSeries series;
    Rng rng{0x2545F4914F6CDD1DULL};
    double previous = 0.0;
    std::int64_t media = 0;
    for (int i = 0; i < 900; ++i) {
        // 同时带漂移（0.3 ppm）与自相关残差，避免退化成"残差为零"那条路。
        previous = 0.7 * previous + rng.normal(30'000.0);
        const auto lateness = static_cast<std::int64_t>(previous) + static_cast<std::int64_t>(
                                                                      0.3 * static_cast<double>(media) / 1000.0);
        series.note(media, lateness);
        media += kStep;
    }

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.hac_estimated);
    const double expected = naive_hac_std_error(series, trend.slope_ppm / 1e6, trend.hac_lags);
    const double actual = trend.slope_ppm_std_error / 1e6;
    CHECK(expected > 0.0);
    CHECK(near(actual, expected, expected * 1e-9));
}

// ⑥ 完美直线：残差严格为 0 ⇒ SE = 0，且 HAC 与 IID 同为 0、膨胀比为 1。
//
//    这是"零点校准"的 HAC 版：能在自己刻的刻度上读出 0 误差，
//    才说明那个膨胀比不是凭空生出来的。
void t06_perfect_line_has_zero_error_on_both_rulers() {
    pace::DriftSeries series;
    // 800 点 × 50 ms = 40 s：必须越过 `kMinSpanNs` 那道 30 s 的门，
    // 否则整条趋势 `usable = false`，测的就不是"完美直线"而是"太短"。
    feed(series, 800, [](int i) {
        return static_cast<std::int64_t>(i) * 1'000;  // 每 50 ms 涨 1 µs = 20 ppm
    });

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.usable);
    CHECK(near(trend.slope_ppm, 20.0, 1e-9));
    CHECK(trend.hac_estimated);
    // 残差在数学上严格为 0，但浮点下两条路各自留下一点舍入残渣：
    // IID 那条正好落到 0，HAC 那边因为逐 lag 重算残差会留下 1e−9 量级的尾数。
    // 所以钉的是"远小于斜率本身"，不是逐位等于 0 —— 后者是无意义的严格。
    CHECK(trend.slope_ppm_std_error_iid == 0.0);
    CHECK(trend.slope_ppm_std_error < 1e-6);
    CHECK(trend.slope_ppm_std_error_inflation == 1.0);  // 两个 0 不能除，退回 1
    CHECK(trend.slope_detectable);
}

// ⑦ 样本不足：`estimated` 必须是 false，且 SE **沿用 IID 值**而不是被清零 ——
//    清零等于宣称"没有误差"，那是完全相反的结论。
void t07_too_few_samples_is_reported_not_hidden() {
    pace::DriftSeries series;
    series.note(0, 10 * kUs);
    series.note(kStep, 12 * kUs);

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.usable == false);
    CHECK(trend.hac_estimated == false);
    CHECK(trend.slope_ppm_std_error == trend.slope_ppm_std_error_iid);
    CHECK(trend.slope_detectable == false);  // n < 3 时明确否掉
}

// ⑧ 持续漂移：两个半程同号。这是"半程诊断"的正常形态。
void t08_steady_drift_agrees_across_halves() {
    pace::DriftSeries series;
    feed(series, 600, [](int i) {
        return static_cast<std::int64_t>(i) * 2'000;  // 每 50 ms 涨 2 µs = 40 ppm
    });

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(near(trend.slope_first_half_ppm, 40.0, 1e-6));
    CHECK(near(trend.slope_second_half_ppm, 40.0, 1e-6));
    CHECK(trend.slope_halves_agree);
}

// ⑨ ★ 中途跳一下：两个半程**反号**，而全序列斜率照样是一个正数。
//
//    这条是本卡最想要的那张图。全序列斜率说"漂了 +X ppm"，
//    半程说"前半段 0、后半段被那一跳拉起来"—— 前者会一直累积，
//    后者不会继续变大，两者的修法完全不同。只给一个斜率，读者分不开。
void t09_a_single_step_looks_like_drift_until_you_split_it() {
    pace::DriftSeries series;
    constexpr int kCount = 600;
    const int jump_at = kCount * 3 / 4;  // 后 1/4 处一次性错位 +50 ms
    feed(series, kCount, [jump_at](int i) {
        return (i < jump_at) ? 0 : (50 * kMs);
    });

    const pace::DriftTrend trend = pace::summarize_drift(series);
    // 前半段完全平（0 ppm），后半段被那一跳拉起来 —— 方向相反。
    CHECK(trend.slope_first_half_ppm == 0.0);
    CHECK(trend.slope_second_half_ppm > 0.0);
    CHECK(trend.slope_halves_agree == false);
    // 而全序列斜率是正的：单看它，这就是一条"漂移"。
    CHECK(trend.slope_ppm > 0.0);
}

// ⑩ SE 一变，CI 与 detectable 必须跟着变 —— 它们不是独立的三个结论。
void t10_confidence_band_is_derived_from_the_hac_error() {
    pace::DriftSeries series;
    feed_ar1(series, 800, 0.9, 40'000.0, 0x9E3779B97F4A7C15ULL);

    const pace::DriftTrend trend = pace::summarize_drift(series);
    const double half = 1.96 * trend.slope_ppm_std_error;
    CHECK(near(trend.slope_ci_low_ppm, trend.slope_ppm - half, 1e-9));
    CHECK(near(trend.slope_ci_high_ppm, trend.slope_ppm + half, 1e-9));
    const bool excludes_zero = (trend.slope_ci_low_ppm > 0.0) || (trend.slope_ci_high_ppm < 0.0);
    CHECK(trend.slope_detectable == (trend.slope_ppm_std_error > 0.0 ? excludes_zero : false));
}

// ⑪ 尺寸守卫：`PaceSummary` 会被整块复制（stop 之后发布一次），
//    `DriftTrend` 是它里面最大的一块。P13-K 一口气加了 9 个字段，
//    这条把"还能不能整块复制"钉成一个数 —— 涨到 1 KB 就该改成共享而非拷贝。
void t11_struct_stays_copyable() {
    const std::size_t size = sizeof(pace::DriftTrend);
    std::printf("  DriftTrend = %zu bytes\n", size);
    CHECK(size <= 640);
}

}  // namespace

int main() {
    t01_bartlett_bandwidth_rule();
    t02_positive_autocorrelation_inflates_the_error();
    t03_white_noise_leaves_the_error_roughly_unque();
    t04_negative_autocorrelation_shrinks_the_error();
    t05_hac_is_recomputable_by_a_naive_implementation();
    t06_perfect_line_has_zero_error_on_both_rulers();
    t07_too_few_samples_is_reported_not_hidden();
    t08_steady_drift_agrees_across_halves();
    t09_a_single_step_looks_like_drift_until_you_split_it();
    t10_confidence_band_is_derived_from_the_hac_error();
    t11_struct_stays_copyable();

    std::printf("p13k hac tests: %d checks, %d failed\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
