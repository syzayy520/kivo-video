// media_span_tests.cpp — P13-H tests: 累计媒体跨度，与那一次口径不匹配的除法
//
// 这一组守的是三句话：
//   ① **`accepted` 的分母必须是累计跨度。** 拿窗口跨度（封顶 102.35 s）去除
//      累计样本数，商随片长线性增长：300 秒的片子会报出 58 Hz，而目标是 20 Hz。
//   ② **两者在 8 s / 35 s 上恰好相等。** 所以缺陷只在没人跑过的时长里存在 ——
//      这也是它活了四张卡的原因。
//   ③ **恒等式 `accepted + unobserved == floor(span/interval) + 1` 是精确的。**
//      它不是容差判据，是抽稀门接受规则的直接推论；两边对不上，
//      只可能是"接受数"和"媒体时间"算到了不同段上。
//
// 纯桌面计算：没有设备、没有时钟、没有睡眠。
#include <cstdint>
#include <cstdio>

#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_drift_trend.hpp"
#include "video/video_plane/core/pace/pace_stats.hpp"

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
constexpr std::int64_t kStep = pace::DriftSeries::kSampleIntervalNs;

/// 窗口跨度 = 序列里首末两点的媒体时刻差。**这不是** `media_span_ns()`：
/// 前者封顶 102.35 s，后者只增不减。P13-H 之前 `observed_hz` 用的是前者。
std::int64_t window_span_ns(const pace::DriftSeries& series) {
    if (series.size() < 2) {
        return 0;
    }
    return series.at(series.size() - 1).media_ns - series.at(0).media_ns;
}

/// 喂 `count` 帧，媒体时刻从 `start_ns` 起每帧走 `step_ns`。
void feed(pace::DriftSeries& series, std::int64_t start_ns, std::int64_t step_ns,
          int count) {
    std::int64_t media = start_ns;
    for (int i = 0; i < count; ++i) {
        series.note(media, 0);
        media += step_ns;
    }
}

// ① 没样本就没有跨度。写 0 而不是"未知"，因为它和 `accepted == 0` 成对出现，
//    判据 H4 就是拿这对守的。
void t01_empty_series_has_zero_span() {
    const pace::DriftSeries series;
    CHECK(series.media_span_ns() == 0);
    CHECK(series.total_samples() == 0);
}

// ② 一个点也没有跨度：位置的差需要两个点。
void t02_single_sample_has_zero_span() {
    pace::DriftSeries series;
    series.note(0, 0);
    CHECK(series.total_samples() == 1);
    CHECK(series.media_span_ns() == 0);
    CHECK(series.at(0).media_ns == 0);
}

// ③ 两点之间：跨度就是两点媒体时刻之差，不是格点数。
void t03_two_samples_span_is_media_delta() {
    pace::DriftSeries series;
    series.note(0, 0);
    series.note(kStep, 0);
    CHECK(series.total_samples() == 2);
    CHECK(series.media_span_ns() == kStep);

    // 一帧跨了三格：只接受一次，跨度是真实媒体时间，不是 1 格。
    pace::DriftSeries jumped;
    jumped.note(0, 0);
    jumped.note(4 * kStep, 0);
    CHECK(jumped.total_samples() == 2);
    CHECK(jumped.media_span_ns() == 4 * kStep);
    CHECK(jumped.unobserved_cells() == 3);
}

// ④ 恒等式：8 秒 30fps（33.333 ms 帧距，会落在格点之间）。
//    这条是整张卡的地基 —— 接受规则的直接推论，不是拟合出来的经验值。
void t04_identity_holds_on_a_30fps_run() {
    pace::DriftSeries series;
    feed(series, 0, 33'333'333, 241);  // ≈8.0 s

    const std::int64_t cells = series.media_span_ns() / kStep;
    CHECK(series.total_samples() + series.unobserved_cells() == cells + 1);
    CHECK(series.total_samples() == 160);
    CHECK(series.media_span_ns() > 7'900 * kMs);
    CHECK(series.media_span_ns() < 8'100 * kMs);
    // 单段未覆盖：两根尺子必须等长（判据 H3 的单元级版本）。
    CHECK(series.truncated() == false);
    CHECK(series.media_span_ns() == window_span_ns(series));
}

// ⑤ 空洞：中间跳过 1 秒没有帧，格点数照样算进去，接受数不变。
//    这就是 `unobserved_cells` 存在的理由 —— 它让"样本为什么变少"可核对。
void t05_identity_holds_across_a_gap() {
    pace::DriftSeries series;
    feed(series, 0, kStep, 10);              // 0 .. 450 ms
    feed(series, 1'500 * kMs, kStep, 10);    // 1.5 s 起，中间空 1 s
    CHECK(series.unobserved_cells() >= 20);  // 1000/50 = 20 格
    const std::int64_t cells = series.media_span_ns() / kStep;
    CHECK(series.total_samples() + series.unobserved_cells() == cells + 1);
}

// ⑥ 环形覆盖之后：窗口甲死在 102.35 s，累计继续涨。
//    这正是 P13-H 修的缺陷 —— 上一版拿窗口跨度去除累计接受数。
void t06_span_keeps_growing_after_ring_wraparound() {
    pace::DriftSeries series;
    feed(series, 0, kStep, 3000);  // 3000 × 50 ms = 149.95 s

    CHECK(series.total_samples() == 3000);
    CHECK(series.truncated() == true);
    CHECK(series.size() == pace::DriftSeries::kCapacity);

    // 累计跨度：整场 149.95 s。
    CHECK(series.media_span_ns() == 2999 * kStep);
    // 窗口跨度：只有最近 2048 点 = 2047 × 50 ms。
    CHECK(window_span_ns(series) == (pace::DriftSeries::kCapacity - 1) * kStep);

    // 恒等式在覆盖后仍然成立（两边都是累计口径）。
    const std::int64_t cells = series.media_span_ns() / kStep;
    CHECK(series.total_samples() + series.unobserved_cells() == cells + 1);

    // 同一份数据，两个分母，两个答案。只有一个在 20 Hz 附近。
    const double hz_cumulative =
        static_cast<double>(series.total_samples()) * 1e9 /
        static_cast<double>(series.media_span_ns());
    const double hz_window =
        static_cast<double>(series.total_samples()) * 1e9 /
        static_cast<double>(window_span_ns(series));
    CHECK(hz_cumulative > 19.9 && hz_cumulative < 20.1);
    CHECK(hz_window > 29.0);  // 3000 / 102.35 s = 29.3 Hz —— 旧公式的答案
}

// ⑦ seek：媒体时刻倒退 → 新段。跨度是**各段之和**，不是"新段末 − 旧段首"。
void t07_seek_sums_per_segment_spans() {
    pace::DriftSeries series;
    feed(series, 0, kStep, 101);   // 段①：0 .. 5000 ms，跨度 5000 ms
    feed(series, 0, kStep, 61);    // 段②：0 .. 3000 ms，跨度 3000 ms

    CHECK(series.segments() == 2);
    CHECK(series.total_samples() == 162);
    CHECK(series.media_span_ns() == 8'000 * kMs);

    // 「新段末 − 旧段首」会得到 3000 − 0 = 3000 ms —— 少了一整段。
    // 差这么多还"看起来合理"，正是按段累加要防的。
    CHECK(series.media_span_ns() != 3'000 * kMs);

    // 恒等式的 "+1" 是**每段各一个**：162 + 0 = 100 + 1 + 60 + 1。
    // 这就是报告层判据 H1 只在 `segments == 1` 时成立的原因。
    CHECK(series.total_samples() + series.unobserved_cells() ==
          (5'000 * kMs / kStep + 1) + (3'000 * kMs / kStep + 1));
}

// ⑧ `clear()` 关段：跨度不跨越被清掉的那一段。
//    如果有人把 `clear()` 写成"只清序列不关段"，这里会得到 7000 ms（两段首尾相连的假跨度）。
void t08_clear_closes_the_segment() {
    pace::DriftSeries series;
    feed(series, 0, kStep, 41);      // 0 .. 2000 ms
    series.clear();
    feed(series, 5'000 * kMs, kStep, 41);  // 5000 .. 7000 ms

    CHECK(series.total_samples() == 82);
    CHECK(series.segments() == 1);  // `clear()` 不改段数，只关跨度
    CHECK(series.media_span_ns() == 4'000 * kMs);  // 2000 + 2000
    CHECK(series.media_span_ns() != 7'000 * kMs);
}

// ⑨ 长片源上节奏仍然是 20 Hz —— 这是缺陷的**用户可见**形态。
//    300 秒：累计跨度口径 20.00 Hz，窗口跨度口径 58.6 Hz。
void t09_rate_stays_near_target_on_a_long_run() {
    pace::DriftSeries series;
    feed(series, 0, kStep, 6000);  // 300 s

    const double hz = static_cast<double>(series.total_samples()) * 1e9 /
                      static_cast<double>(series.media_span_ns());
    // 多出来的是首帧那一次接受：6000 / 299.95 s = 20.003 Hz。
    CHECK(hz > 20.0 && hz < 20.02);
    // 上界形式：接受数不可能超过"格点数 + 1"。这条不依赖除法，是机制本身。
    CHECK(series.total_samples() <= series.media_span_ns() / kStep + 1);
}

// ⑩ 经过 `PaceStats`：字段真的接上了，不是只活在 `DriftSeries` 里。
//    上一卡撞过三次"改了一半"（`SCHEMA_WITH_WORST` / `check_pair` / `observed_hz` 的 else），
//    所以这里从 `note_presented()` 一路走到 `PaceSummary`。
void t10_pace_summary_carries_the_cumulative_span() {
    pace::PaceStats stats;
    for (int i = 0; i < 3000; ++i) {
        stats.note_presented(0, i * kStep);
    }
    const pace::PaceSummary summary = stats.summary(33 * kMs, pace::ClockHealth::kFresh);

    CHECK(summary.sampled_samples == 3000);
    CHECK(summary.drift_media_span_ns == 2999 * kStep);
    CHECK(summary.drift.truncated == true);
    // 窗口跨度（趋势用的那个）封顶，累计跨度不封顶 —— 两者必须不同。
    CHECK(summary.drift.span_ns == (pace::DriftSeries::kCapacity - 1) * kStep);
    CHECK(summary.drift.span_ns != summary.drift_media_span_ns);
}

// ⑪ 尾部被拒的帧**不算**进跨度。
//
// 这条是被变异逼出来的：上面九个用例里最后一帧永远是被接受的（`feed()` 走整格），
// 于是"关段时用最后观察到的帧"这个变异一次都没红。与 P13-G 的 `t07` 同族 ——
// 断言想守的场景没被数据覆盖，测试自己是绿的。
// 现在显式造出"最后一帧没跨过格点"的形态。
void t11_trailing_skipped_frame_is_not_counted_in_span() {
    pace::DriftSeries series;
    series.note(0, 0);                 // 接受：首帧
    series.note(kStep, 0);             // 接受：跨过 50 ms 格点，下一个格点是 100 ms
    series.note(kStep + 20 * kMs, 0);  // 70 ms：没跨过 100 ms → 被拒

    CHECK(series.total_samples() == 2);
    CHECK(series.observed_frames() == 3);
    // 段还开着：跨度按最后**接受**的 50 ms，不是最后观察到的 70 ms。
    CHECK(series.media_span_ns() == kStep);

    // 关段之后仍然是 50 ms —— 关段路径必须和实时路径同口径。
    series.clear();
    CHECK(series.media_span_ns() == kStep);
    CHECK(series.media_span_ns() != kStep + 20 * kMs);
}

}  // namespace

int main() {
    std::printf("P13-H media-span tests\n");
    t01_empty_series_has_zero_span();
    t02_single_sample_has_zero_span();
    t03_two_samples_span_is_media_delta();
    t04_identity_holds_on_a_30fps_run();
    t05_identity_holds_across_a_gap();
    t06_span_keeps_growing_after_ring_wraparound();
    t07_seek_sums_per_segment_spans();
    t08_clear_closes_the_segment();
    t09_rate_stays_near_target_on_a_long_run();
    t10_pace_summary_carries_the_cumulative_span();
    t11_trailing_skipped_frame_is_not_counted_in_span();
    std::printf("\n%d 项断言 / %d 项失败\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
