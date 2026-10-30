// drift_series_tests.cpp — P13-D tests: 抽稀序列与漂移趋势
//
// 这一组测试守的是三句话，每一句都对应一个曾经会被读错的结论：
//   ① **抽稀按媒体时间，不按帧序号** —— 同片长 30fps 与 60fps 必须采到同样多的点。
//      守不住这条，60fps 的 p99 永远比 30fps 差，而"差"是密度造的。
//   ② **斜率要能算回已知答案** —— 喂一条 1000 ppm 的直线，必须算回 1000 ppm。
//      守不住这条，长片上的"漂移"就只是拟合噪声。
//   ③ **不可用必须带理由** —— 片段太短 / 时间不前进时，`usable=false` + 非空 reason。
//      守不住这条，"斜率 0"会被读成"没有漂移"。
//
// 纯桌面计算：没有设备、没有时钟、没有睡眠。
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

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

#define CHECK_NEAR(actual, expected, tolerance)                                        \
    do {                                                                               \
        ++g_checked;                                                                   \
        const double check_actual = static_cast<double>(actual);                       \
        const double check_expected = static_cast<double>(expected);                   \
        const double check_tolerance = static_cast<double>(tolerance);                 \
        if (std::fabs(check_actual - check_expected) > check_tolerance) {              \
            ++g_failed;                                                                \
            std::printf("  FAIL %s:%d  %.6f vs %.6f (tol %.6f)\n", __FILE__, __LINE__, \
                        check_actual, check_expected, check_tolerance);                \
        }                                                                              \
    } while (false)

constexpr std::int64_t kMs = 1'000'000;
constexpr std::int64_t kSecond = 1'000 * kMs;

/// 确定性噪声：不用 `<random>`，免得同一份用例在不同 STL 上给出不同的数。
class Noise {
public:
    explicit Noise(std::uint64_t seed) : state_(seed) {}

    /// 返回 [−amplitude, +amplitude)
    [[nodiscard]] double next(double amplitude) {
        state_ = state_ * 6364136223846793005ULL + 1442695040888963407ULL;
        const std::uint64_t high = state_ >> 33;                    // 31 位
        const double unit = static_cast<double>(high) / 2147483648.0;  // [0, 1)
        return (unit * 2.0 - 1.0) * amplitude;
    }

private:
    std::uint64_t state_;
};

/// 按帧率喂满一段媒体时间。
///
/// @param series      目标序列
/// @param duration_ns 片长
/// @param frame_ns    帧间隔（30fps → 33'333'333；60fps → 16'666'666）
/// @param lateness    给定媒体时刻的 lateness（ns）
template <typename Fn>
void feed(pace::DriftSeries& series, std::int64_t duration_ns, std::int64_t frame_ns, Fn lateness) {
    for (std::int64_t pts = 0; pts <= duration_ns; pts += frame_ns) {
        series.note(pts, lateness(pts));
    }
}

// ── ① 抽稀与帧率无关 ──────────────────────────────────────────────

void t01_same_duration_same_sample_count_across_frame_rates() {
    pace::DriftSeries at30;
    pace::DriftSeries at60;
    feed(at30, 8 * kSecond, kSecond / 30, [](std::int64_t) { return 0; });
    feed(at60, 8 * kSecond, kSecond / 60, [](std::int64_t) { return 0; });

    // 首帧 + 8 s / 50 ms = 160 个格点 → 161 个样本。
    //
    // 这条断言在实现的第一版上**打红过**：那一版的门是"距上一次被接受的 pts ≥ 50 ms"，
    // 30fps 的帧距 33.3 ms 够不上，最早一帧落在 66.7 ms，节奏退化成 15 Hz ——
    // 结果是 30fps 采 121 个、60fps 采 161 个，抽稀反而制造了新的不可比。
    // 改成绝对格点后才对齐。数字留在这里，防止有人把门再"简化"回去。
    CHECK_NEAR(at30.size(), 161, 2);
    CHECK_NEAR(at60.size(), 161, 2);
    // 这条才是本卡的全部意义：两个帧率的样本数差不超过 1 个点。
    CHECK(std::abs(at30.size() - at60.size()) <= 1);
    // 帧数确实翻倍了 —— 否则上面那条只是"没喂进去"
    CHECK(at60.observed_frames() > at30.observed_frames() * 3 / 2);
}

void t02_first_sample_is_always_accepted() {
    pace::DriftSeries series;
    CHECK(series.note(0, 100));
    CHECK(series.size() == 1);
    // 紧接着的一帧（未到一个采样间隔）被拒
    CHECK(!series.note(kSecond / 60, 200));
    CHECK(series.size() == 1);
    CHECK(series.skipped_frames() == 1);
    CHECK(series.observed_frames() == 2);
}

void t03_skip_ratio_matches_frame_rate() {
    pace::DriftSeries series;
    feed(series, 8 * kSecond, kSecond / 60, [](std::int64_t) { return 0; });
    CHECK(series.observed_frames() == 481);  // 0..8s 闭区间，60fps
    CHECK(series.total_samples() + series.skipped_frames() == series.observed_frames());
    CHECK(series.skipped_frames() > 300);
}

// ── ② 斜率算得回已知答案 ────────────────────────────────────────────

void t04_recovers_known_1000ppm_ramp() {
    pace::DriftSeries series;
    // lateness = pts × 1e−3 → 每秒偏 1 ms = 1000 ppm
    feed(series, 60 * kSecond, kSecond / 20, [](std::int64_t pts) {
        return static_cast<std::int64_t>(static_cast<double>(pts) * 1e-3);
    });
    const pace::DriftTrend trend = pace::summarize_drift(series);

    CHECK(trend.usable);
    CHECK_NEAR(trend.slope_ppm, 1000.0, 5.0);
    CHECK(trend.r2 > 0.99);
    // 两道斜率必须同号、同量级
    CHECK(trend.slope_from_windows_ppm > 0.0);
    CHECK_NEAR(trend.slope_from_windows_ppm, 1000.0, 60.0);
    // 窗口中位数代表窗口**中心**，所以首末之差是 7/8 个片长 × 斜率 = 52.5 ms，
    // 不是 60 ms。写成 60 会逼着实现去"修"一个本来正确的量。
    CHECK_NEAR(trend.delta_median_ns, 52.5 * static_cast<double>(kMs), 1.0 * static_cast<double>(kMs));
}

void t05_flat_with_noise_reports_no_drift() {
    // 逐帧抖动 ±0.5 ms（比真实调度噪声大得多），媒体时间完全匀速
    pace::DriftSeries noisy;
    Noise noise(20261030);
    for (std::int64_t pts = 0; pts <= 60 * kSecond; pts += kSecond / 60) {
        noisy.note(pts, static_cast<std::int64_t>(noise.next(500'000.0)));
    }

    const pace::DriftTrend trend = pace::summarize_drift(noisy);
    std::printf("  [t05] slope=%.3f ppm windows=%.3f ppm r2=%.4f samples=%d\n",
                trend.slope_ppm, trend.slope_from_windows_ppm, trend.r2, trend.samples_used);
    CHECK(trend.usable);
    // 噪声不该被读成漂移：远小于人耳阈值（1 ppm = 1 µs/s）
    CHECK(std::fabs(trend.slope_ppm) < 50.0);
    // 线性度低 —— 这正是"光有斜率没有 r2"会撒的谎
    CHECK(trend.r2 < 0.5);
}

void t06_two_slopes_agree_on_noisy_ramp() {
    pace::DriftSeries series;
    Noise noise(7);
    feed(series, 60 * kSecond, kSecond / 30, [&noise](std::int64_t pts) {
        const double ramp = static_cast<double>(pts) * 2e-3;  // 2000 ppm
        return static_cast<std::int64_t>(ramp + noise.next(300'000.0));
    });
    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.usable);
    CHECK(static_cast<double>(trend.slope_ppm) * static_cast<double>(trend.slope_from_windows_ppm) > 0.0);
    const double ratio = trend.slope_from_windows_ppm / trend.slope_ppm;
    CHECK(ratio > 0.8 && ratio < 1.2);
}

// ── ③ 不可用必须带理由 ────────────────────────────────────────────

void t07_short_clip_is_not_usable() {
    pace::DriftSeries series;
    feed(series, 8 * kSecond, kSecond / 30, [](std::int64_t pts) {
        return static_cast<std::int64_t>(static_cast<double>(pts) * 1e-3);
    });
    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(!trend.usable);
    CHECK(trend.unusable_reason[0] != '\0');
    // 即便不可用，样本数与窗口仍然要报出来：诊断要用
    CHECK(trend.samples_used > 100);
    CHECK(trend.window_count >= 2);
}

void t08_empty_and_frozen_media_time_are_rejected() {
    pace::DriftSeries empty;
    const pace::DriftTrend empty_trend = pace::summarize_drift(empty);
    CHECK(!empty_trend.usable);
    CHECK(empty_trend.unusable_reason[0] != '\0');
    CHECK(empty_trend.samples_used == 0);

    pace::DriftSeries frozen;
    for (int index = 0; index < 600; ++index) {
        frozen.note(0, index * 1000);  // pts 不动
    }
    const pace::DriftTrend frozen_trend = pace::summarize_drift(frozen);
    CHECK(!frozen_trend.usable);
    CHECK(frozen_trend.unusable_reason[0] != '\0');
}

// ── 环形与分段 ────────────────────────────────────────────────────

void t09_ring_overwrites_oldest_and_flags_truncation() {
    pace::DriftSeries series;
    const std::int64_t total = 3000;  // 超过 2048 容量
    for (std::int64_t index = 0; index < total; ++index) {
        series.note(index * pace::DriftSeries::kSampleIntervalNs, index);
    }
    CHECK(series.total_samples() == total);
    CHECK(series.size() == pace::DriftSeries::kCapacity);
    CHECK(series.truncated());
    // 最早留下的点 = 总数 − 容量
    const std::int64_t first_kept = total - pace::DriftSeries::kCapacity;
    CHECK(series.at(0).lateness_ns == first_kept);
    CHECK(series.at(series.size() - 1).lateness_ns == total - 1);
}

void t10_backwards_media_time_starts_a_new_segment() {
    pace::DriftSeries series;
    feed(series, 5 * kSecond, kSecond / 30, [](std::int64_t) { return 0; });
    const std::int32_t before = series.size();
    CHECK(series.segments() == 1);

    // seek 回到 0：pts 倒退
    series.note(0, 0);
    CHECK(series.segments() == 2);
    CHECK(series.size() == 1);  // 旧时间轴的点被清掉，不许拼进新回归
    CHECK(before > 0);
    // 累计计数不清：证据要的是"整场发生过什么"
    CHECK(series.observed_frames() > before);
}

void t11_window_bounds_cover_every_sample() {
    pace::DriftSeries series;
    feed(series, 60 * kSecond, kSecond / 20, [](std::int64_t pts) {
        return static_cast<std::int64_t>(static_cast<double>(pts) * 1e-3);
    });
    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.window_count == pace::DriftTrend::kWindowCount);
    std::int64_t sum = 0;
    for (std::int32_t index = 0; index < trend.window_count; ++index) {
        sum += trend.window_samples[static_cast<std::size_t>(index)];
        CHECK(trend.window_samples[static_cast<std::size_t>(index)] > 0);
    }
    // 每个样本恰好落进一个窗口
    CHECK(sum == trend.samples_used);
}

void t12_grid_gaps_are_counted_but_not_filled() {
    pace::DriftSeries series;
    for (std::int64_t pts = 0; pts <= kSecond; pts += pace::DriftSeries::kSampleIntervalNs) {
        series.note(pts, 0);
    }
    const std::int64_t samples_before = series.total_samples();

    // 500 ms 的空洞：中间 9 个格点没有帧跨过
    series.note(1500 * kMs, 0);
    for (std::int64_t pts = 1550 * kMs; pts <= 2 * kSecond; pts += pace::DriftSeries::kSampleIntervalNs) {
        series.note(pts, 0);
    }

    CHECK(series.unobserved_cells() == 9);
    // 没有补样本：总样本数 = 之前的 + 空洞后真实采到的那些
    CHECK(series.total_samples() > samples_before);
    CHECK(series.total_samples() < samples_before + 20);
}

}  // namespace

int main() {
    struct Case {
        const char* name;
        void (*fn)();
    };
    const Case cases[] = {
        {"01 抽稀后 30/60fps 样本数相同", t01_same_duration_same_sample_count_across_frame_rates},
        {"02 首帧必采、间隔内的帧被拒", t02_first_sample_is_always_accepted},
        {"03 拒采比例随帧率上升", t03_skip_ratio_matches_frame_rate},
        {"04 1000 ppm 直线算回 1000 ppm", t04_recovers_known_1000ppm_ramp},
        {"05 纯噪声不报漂移且 r2 低", t05_flat_with_noise_reports_no_drift},
        {"06 两道斜率同号同量级", t06_two_slopes_agree_on_noisy_ramp},
        {"07 8 秒片段不可用且带理由", t07_short_clip_is_not_usable},
        {"08 空序列与静止 pts 都被拒", t08_empty_and_frozen_media_time_are_rejected},
        {"09 环形覆盖保留最近 102.4 秒", t09_ring_overwrites_oldest_and_flags_truncation},
        {"10 pts 倒退开新段并清空旧点", t10_backwards_media_time_starts_a_new_segment},
        {"11 窗口覆盖全部样本", t11_window_bounds_cover_every_sample},
        {"12 空洞记格点但不补样本", t12_grid_gaps_are_counted_but_not_filled},
    };

    for (const Case& item : cases) {
        const int before = g_failed;
        item.fn();
        std::printf("%s %s\n", (g_failed == before) ? "PASS" : "FAIL", item.name);
    }
    std::printf("\n%d 项断言 / %d 项失败\n", g_checked, g_failed);
    return (g_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
