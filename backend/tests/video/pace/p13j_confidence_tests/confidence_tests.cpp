// confidence_tests.cpp — P13-J tests: 给漂移量配误差棒
//
// 这一组守四句话：
//   ① **0.25 ms 桶中心会把一整段真实差异吃成同一个数。** 真机上 29 µs~250 µs
//      铺满 220 µs，八窗中位数却全是 125000 —— 一模一样的"没有漂移"和
//      "尺子上只有一个刻度"长得完全一样，只有换原始值才分得开。
//   ② **斜率必须带标准误。** `r2 = 5.5e-05` 时 `-0.022183` 的六位小数全是噪声，
//      没有 SE 就等于宣称自己没那么精确的东西很精确。
//   ③ **CI 含 0 时不许说看得见。** 这是 `slope_detectable` 唯一的意义，
//      也是"小于 X ppm"这个说法的唯一来源。
//   ④ **量不出来和量出零是两件事。** `usable == false` 时必须靠它否掉整条趋势，
//      而不是塞一个看起来很舒服的 0 SE。
//
// 纯桌面计算：没有设备、没有时钟、没有睡眠。
#include <array>
#include <cstdint>
#include <cstdio>

#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_drift_trend.hpp"
#include "video/video_plane/core/pace/pace_histogram.hpp"
#include "video/video_plane/core/pace/pace_window_stats.hpp"

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

/// 每帧走一格 → 每帧必被接受。`lateness_of(i)` 给出第 i 个点的 lateness。
template <typename Fn>
void feed(pace::DriftSeries& series, int count, Fn lateness_of) {
    std::int64_t media = 0;
    for (int i = 0; i < count; ++i) {
        series.note(media, lateness_of(i));
        media += kStep;
    }
}

/// xorshift64：确定性、无平台差异。随机测试必须可复现，
/// 否则"这次没红"到底说明什么就没人说得清。
struct Rng {
    std::uint64_t state;
    std::uint64_t next() noexcept {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    }
    std::int64_t symmetric(std::int64_t half_range_ns) noexcept {
        const auto span = static_cast<std::uint64_t>(2 * half_range_ns);
        return static_cast<std::int64_t>(next() % span) - half_range_ns;
    }
};

bool near(double value, double expected, double tolerance) {
    const double diff = value - expected;
    return (diff < 0 ? -diff : diff) <= tolerance;
}

// ① 完美线性 + 已知斜率：−1000 ppm 必须被原样量出来，SE 为 0，且判定为"看得见"。
//
//    这是尺子的**零点校准**。尺子读不出一个自己刚刻上去的刻度，后面所有
//    关于"漂移很小"的结论都没有意义。
void t01_perfect_linear_slope_is_recovered() {
    pace::DriftSeries series;
    // lateness = −0.001 × 媒体时刻 → 每秒偏 −1 ms → −1000 ppm。
    // 用整除而不是浮点：`media / 1000` 在 50 ms 步长上恰好是整数，
    // 残差严格为 0，于是"完美线性"这个前提是真的，不是近似的。
    feed(series, 2001, [](int i) { return -static_cast<std::int64_t>(i) * 50'000; });

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.usable);
    CHECK(near(trend.slope_ppm, -1000.0, 1e-6));
    // 残差理论上是 0，浮点累加后是 1e-6 ppm 量级的余数 —— 断言"比斜率小六个数量级"，
    // 不要断言 `== 0.0`：那会把浮点行为钉进测试，换个编译器就红。
    CHECK(trend.slope_ppm_std_error < 1e-3);
    CHECK(trend.slope_detectable);  // 可以忽略的误差带里，−1000 ppm 仍然远离 0
    CHECK(trend.r2 >= 1.0 - 1e-9);
}

// ② 纯噪声：CI 必须含 0，`slope_detectable` 必须 false，SE 必须是个正经数。
//
//    这条是"小于 X ppm"这句话的来源。没有它，噪声里挑出来的斜率
//    会被当成漂移报出去。
void t02_pure_noise_has_zero_inside_the_confidence_band() {
    pace::DriftSeries series;
    Rng rng{0x9E3779B97F4A7C15ULL};
    feed(series, 2001, [&rng](int) { return rng.symmetric(200 * kUs); });

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.usable);
    CHECK(trend.slope_ppm_std_error > 0.0);
    CHECK(trend.slope_ci_low_ppm < 0.0);
    CHECK(trend.slope_ci_high_ppm > 0.0);
    CHECK(trend.slope_detectable == false);
    // 量级检查：±200 µs 的噪声、100 s 跨度、2000 个点，SE 该在零点几个 ppm。
    // 写成一个区间而不是一个数 —— 这里的目的是抓"量级错了 1000 倍"，
    // 不是把实现里的常数抄进测试。
    CHECK(trend.slope_ppm_std_error > 0.005);
    CHECK(trend.slope_ppm_std_error < 1.0);
}

// ③ CI 必须能由 `slope_ppm` 与 `slope_ppm_std_error` 复算出来。
//
//    第三方拿到证据要能自己验：这是 `kivo.p12c.sync/8` 里三个数并排存在的理由。
//    若 CI 用了别的系数（或者干脆是另一条路径算的），复算就对不上。
void t03_confidence_band_is_recomputable() {
    pace::DriftSeries series;
    Rng rng{0x2545F4914F6CDD1DULL};
    feed(series, 1200, [&rng](int) { return rng.symmetric(80 * kUs); });

    const pace::DriftTrend trend = pace::summarize_drift(series);
    const double half = 1.96 * trend.slope_ppm_std_error;
    CHECK(near(trend.slope_ci_low_ppm, trend.slope_ppm - half, 1e-9));
    CHECK(near(trend.slope_ci_high_ppm, trend.slope_ppm + half, 1e-9));
    // 两端关于斜率对称：不对称就说明有一端被钳过，那 CI 就不再是 CI。
    CHECK(near(trend.slope_ci_high_ppm - trend.slope_ppm,
               trend.slope_ppm - trend.slope_ci_low_ppm, 1e-9));
}

// ④ ★ 本卡的核心反例：八窗差异全在**同一个桶内**。
//
//    每窗一个中位数：30 µs、55 µs … 205 µs，真实跨距 175 µs（2.0 ppm），
//    而它们全部落在桶 512（0~250 µs，中心 125 µs）。
//
//    构造上有个陷阱，第一版就踩了：`percentile_ns()` 会把结果钳进 `[min, max]`。
//    若窗内 lateness 恒定，min == max == 中位数，桶中心被钳回原值 ——
//    看起来"没塌"，反例完全失效（变异 C1 因此只红了三行 `exact` 标记）。
//    真机上不塌是因为每窗都有偶发尖峰把 `max` 顶到 125 µs 之上。
//    所以这里**每窗埋一个 300 µs 的尖峰**：它不进中位数（1/250），
//    但把 `max` 顶上去，让桶中心落在范围内。
//
//    旧实现（桶中心）给出八个 125000：delta 0、win_ppm 0。
//    新实现必须给出八个不同的值，delta 恰好 175000。
void t04_window_medians_survive_the_bucket() {
    pace::DriftSeries series;
    // 2001 点 / 100 s，每窗 250 点（末窗 251）。窗 w 的中位数是 30+w×25 µs。
    const auto lateness_of = [](int i) {
        const int window = (i / 250 > 7) ? 7 : (i / 250);
        if (i % 250 == 7) {
            return 300 * kUs;  // 尖峰：顶高超采样窗的 max，不进中位数
        }
        const std::int64_t base = 30 * kUs + static_cast<std::int64_t>(window) * 25 * kUs;
        return base + ((i % 2 == 0) ? 0 : 2 * kUs);
    };
    feed(series, 2001, lateness_of);

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.window_median_exact);
    CHECK(trend.window_median_fallback_windows == 0);
    CHECK(trend.window_count == 8);

    for (int w = 0; w < 8; ++w) {
        const std::int64_t expected = 30 * kUs + static_cast<std::int64_t>(w) * 25 * kUs;
        CHECK(trend.window_median_ns[static_cast<std::size_t>(w)] == expected);
    }
    CHECK(trend.delta_median_ns == 175 * kUs);
    // 窗口斜率用 8 个窗**中心**回归，横坐标跨度是 87.5 s 而不是 100 s，
    // 故约 2.00 ppm（阶梯的解析值是 1.75 ppm，两者差这 14% 是口径造成的，不是错）。
    // 这条只依赖中位数 —— 尖峰不进中位数，所以这里可以钉死。
    CHECK(trend.slope_from_windows_ppm > 1.99);
    CHECK(trend.slope_from_windows_ppm < 2.01);

    // 同一批点，用桶中心算会是什么：全部塌成一个数。这就是旧实现。
    pace::SignedHistogram bucket_only;
    for (int i = 0; i < 2001; ++i) {
        bucket_only.add(lateness_of(i));
    }
    // 桶 512 的中心 = −128 ms + 512 × 0.25 ms + 0.125 ms = 125 µs。
    CHECK(bucket_only.percentile_ns(0.50) == 125 * kUs);
    CHECK(bucket_only.percentile_ns(0.50) != trend.window_median_ns[0]);
}

// ④b 同一个阶梯，去掉尖峰：逐点斜率必须落在解析值上。
//
//    与 ④ 分开是因为尖峰会影响逐点回归（8 个 300 µs 的点会拉高均值），
//    而它**不影响**中位数。所以"窗口是精确的"由 ④ 守，
//    "逐点回归量得对"由这里守，两件事不互相拖累。
//
//    解析值 1.976 ppm：阶梯从 30 µs 到 205 µs、跨 100 s，但首末窗点数不等
//    （250 与 251），所以不是漂亮的 1.75。
void t04b_point_slope_matches_the_analytic_value() {
    pace::DriftSeries series;
    feed(series, 2001, [](int i) {
        const int window = (i / 250 > 7) ? 7 : (i / 250);
        return 30 * kUs + static_cast<std::int64_t>(window) * 25 * kUs;
    });

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.slope_ppm > 1.96);
    CHECK(trend.slope_ppm < 1.99);
    CHECK(trend.slope_from_windows_ppm > 1.99);
    CHECK(trend.slope_from_windows_ppm < 2.01);
}

// ⑤ 超采样窗必须显式降级，不许安静地给出一个桶中心。
//
//    600 个点挤在第 0 窗（其余窗只有一个点、跨度拉到 1000 s）。
//    这时 scratch 512 装不下，`exact` 必须为 false 且计数上报。
//
//    lateness 一半 10 µs 一半 200 µs（**极值跨过桶中心**，否则 `percentile_ns()`
//    会钳到 max，测的就不是桶中心了）。真实中位数是 10 µs，而退化后报 125 µs ——
//    差了 12 倍。这就是为什么这个降级必须一路传到证据里：不标记的话，
//    读者会把 125 µs 当成一窗的真实中位数。
void t05_oversampled_window_degrades_visibly() {
    pace::DriftSeries series;
    int i = 0;
    std::int64_t media = 0;
    for (; i < 600; ++i) {  // 第 0 窗：0 ~ 29.95 s
        series.note(media, (i % 2 == 0) ? 10 * kUs : 200 * kUs);
        media += kStep;
    }
    series.note(1000'000 * kMs, 10 * kUs);  // 跳到 1000 s，把跨度拉大

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.window_samples[0] == 600);
    CHECK(trend.window_median_fallback_windows == 1);
    CHECK(trend.window_median_exact == false);
    // 桶 512 的中心：−128 ms + 512 × 0.25 ms + 0.125 ms = 125 µs。
    CHECK(trend.window_median_ns[0] == 125 * kUs);
}

// ⑥ 两个点算不出残差方差。这时 SE 为 0 是"算不出"，不是"没有误差" ——
//    整条趋势必须被 `usable` 否掉，`slope_detectable` 也不许为真。
void t06_two_points_cannot_produce_a_standard_error() {
    pace::DriftSeries series;
    feed(series, 2, [](int i) { return static_cast<std::int64_t>(i) * 1'000; });

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.usable == false);
    CHECK(trend.slope_ppm_std_error == 0.0);
    CHECK(trend.slope_detectable == false);
    CHECK(trend.unusable_reason[0] != '\0');
}

// ⑦ ★ SE 必须随基线变长而收缩。
//
//    这条守的是"尺子真的在变细"，而不是"我们加了个字段"。`Sxx ∝ n·span²`，
//    残差方差不变时 `SE ∝ 1/(span·√n)` —— 点数与跨度同时翻四倍，SE 该缩到 1/8。
//    方向错了或压根没动，这个字段就只是个装饰数字。
void t07_standard_error_shrinks_as_the_baseline_grows() {
    constexpr int kBase = 500;

    pace::DriftSeries small;
    Rng rng_a{0xD1B54A32D192ED03ULL};
    feed(small, kBase, [&rng_a](int) { return rng_a.symmetric(100 * kUs); });

    pace::DriftSeries large;
    Rng rng_b{0xD1B54A32D192ED03ULL};
    // 抽稀门按 50 ms 格点走，缩短步长会被拒 —— 所以只能靠"更长的片源"来加样本。
    // 点数 × 4、跨度 × 4（100 s → 400 s），这正是真实取证里延长播放做的事。
    feed(large, kBase * 4, [&rng_b](int) { return rng_b.symmetric(100 * kUs); });

    const pace::DriftTrend a = pace::summarize_drift(small);
    const pace::DriftTrend b = pace::summarize_drift(large);
    CHECK(a.slope_ppm_std_error > 0.0);
    CHECK(b.slope_ppm_std_error > 0.0);
    // 点数 ×4、跨度 ×4 ⇒ Sxx ×64、残差方差不变 ⇒ SE 应当缩到 1/8 左右。
    // 用宽松区间（1/12 ~ 1/5）抓"完全没收缩"和"收缩方向反了"这两类错。
    const double ratio = b.slope_ppm_std_error / a.slope_ppm_std_error;
    CHECK(ratio < 0.20);
    CHECK(ratio > 0.083);
}

// ⑧ 中位数的秩口径必须与 `SignedHistogram::percentile_ns(0.50)` 一致。
//
//    两份中位数会同时出现在证据里（窗口分布 vs 整场分布）。
//    口径不同，读者会发现"同一批点的 p50 有两个值" —— 而那是我们的错，
//    不是数据的问题。
void t08_rank_convention_matches_the_histogram() {
    // n = 8：rank = 4 → 第 4 个（1 基）→ 下标 3 → 值 3。
    pace::DriftSeries even;
    feed(even, 8, [](int i) { return static_cast<std::int64_t>(i) * 1'000; });
    pace::SignedHistogram fallback;
    const pace::WindowMedian m8 = pace::median_of_slice(even, pace::WindowSlice{0, 8}, fallback);
    CHECK(m8.exact);
    CHECK(m8.samples == 8);
    CHECK(m8.value_ns == 3'000);

    // n = 7：rank = 4 → 下标 3 → 值 3。上取整与下取整在这里差一位，必须钉住。
    pace::DriftSeries odd;
    feed(odd, 7, [](int i) { return static_cast<std::int64_t>(i) * 1'000; });
    const pace::WindowMedian m7 = pace::median_of_slice(odd, pace::WindowSlice{0, 7}, fallback);
    CHECK(m7.exact);
    CHECK(m7.value_ns == 3'000);
}

// ⑨ `split_windows` 的边界规则必须与趋势里用的那套逐字一致。
//
//    末点归最后一窗（`offset == span` 时整除会得到 `window_count`）。
//    这条边界写错，最后一个点就落进一个不存在的窗，样本数总和少 1 ——
//    而判据 I1（Σ window_samples == window_sample_count）正是这么抓的。
void t09_window_split_keeps_the_last_point() {
    pace::DriftSeries series;
    feed(series, 101, [](int) { return 0; });

    std::array<pace::WindowSlice, 8> slices{};
    const std::int32_t recorded = pace::split_windows(series, 8, slices.data());
    CHECK(recorded == 8);

    std::int32_t total = 0;
    for (int w = 0; w < 8; ++w) {
        total += slices[static_cast<std::size_t>(w)].count;
    }
    CHECK(total == 101);  // 一个点都不许丢

    // 每个窗的起点必须是它前一个窗的终点（区间表示的前提）。
    for (int w = 1; w < 8; ++w) {
        const pace::WindowSlice prev = slices[static_cast<std::size_t>(w - 1)];
        const pace::WindowSlice cur = slices[static_cast<std::size_t>(w)];
        if (cur.count > 0 && prev.count > 0) {
            CHECK(prev.first + prev.count == cur.first);
        }
    }

    // 末点必须落在最后一窗里，不能被钳丢。
    const pace::WindowSlice last = slices[7];
    CHECK(last.first + last.count == 101);
}

}  // namespace

int main() {
    t01_perfect_linear_slope_is_recovered();
    t02_pure_noise_has_zero_inside_the_confidence_band();
    t03_confidence_band_is_recomputable();
    t04_window_medians_survive_the_bucket();
    t04b_point_slope_matches_the_analytic_value();
    t05_oversampled_window_degrades_visibly();
    t06_two_points_cannot_produce_a_standard_error();
    t07_standard_error_shrinks_as_the_baseline_grows();
    t08_rank_convention_matches_the_histogram();
    t09_window_split_keeps_the_last_point();

    std::printf("p13j confidence: %d checks, %d failed\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
