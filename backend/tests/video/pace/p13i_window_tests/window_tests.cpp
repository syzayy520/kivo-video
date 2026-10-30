// window_tests.cpp — P13-I tests: 窗口的身份（覆盖 ≠ 跨段；趋势自带分布）
//
// 这一组守的是三句话：
//   ① **覆盖与跨段是两件不同的事。** 环形容量是按**段**算的，不是按整场。
//      上一版 `truncated()` 拿整场累计数去比 2048，于是 seek 之后必然谎报
//      "覆盖发生过"—— 而当前段一个点都没被覆盖。判据是 `segment_samples() > size()`。
//   ② **窗口里剩几个点是能算出来的。** `size() == min(segment_samples(), kCapacity)`，
//      在任何时刻（空的、没满的、刚满的、覆盖过的、刚 clear 的）都成立。
//   ③ **趋势自带的那批点，自带它们的分布。** 未覆盖且单段时，窗口分布与整场累计
//      分布必须逐桶相同；一旦不同，`truncated` 或 `segments` 必有一个能解释。
//
// 纯桌面计算：没有设备、没有时钟、没有睡眠。
#include <cstdint>
#include <cstdio>

#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_drift_trend.hpp"
#include "video/video_plane/core/pace/pace_drift_window.hpp"
#include "video/video_plane/core/pace/pace_histogram.hpp"
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

/// 喂 `count` 帧，媒体时刻从 `start_ns` 起每帧走一格（必被接受）。
void feed(pace::DriftSeries& series, std::int64_t start_ns, int count,
          std::int64_t lateness_ns = 0) {
    std::int64_t media = start_ns;
    for (int i = 0; i < count; ++i) {
        series.note(media, lateness_ns);
        media += kStep;
    }
}

/// 两份分布是不是同一份数据。逐桶比、连溢出计数与极值一起比 ——
/// 只比 `count()` 等于什么都没比（同样的样本数可以落在完全不同的桶里）。
bool same_distribution(const pace::SignedHistogram& a, const pace::SignedHistogram& b) {
    if (a.count() != b.count() || a.below_count() != b.below_count() ||
        a.above_count() != b.above_count() || a.min_ns() != b.min_ns() ||
        a.max_ns() != b.max_ns()) {
        return false;
    }
    return a.bins() == b.bins();
}

// ① 单段覆盖：段内接受数超过容量 → `truncated` 为真，窗口停在 2048。
//    这是"正常"的覆盖，上一版也判得对 —— 放在这里是为了给 ② 当对照。
void t01_single_segment_wraparound_is_truncation() {
    pace::DriftSeries series;
    feed(series, 0, 3000);

    CHECK(series.total_samples() == 3000);
    CHECK(series.segment_samples() == 3000);
    CHECK(series.size() == pace::DriftSeries::kCapacity);
    CHECK(series.truncated() == true);
    CHECK(series.segments() == 1);
}

// ② **本卡的核心**：seek 之后累计数越过容量，但当前段一个点都没被覆盖。
//
//    上一版 `truncated()` 是 `total_ > kCapacity`：1200 + 1800 = 3000 > 2048，
//    于是它说"覆盖发生过"。而序列里躺着的是第二段的整整 1800 个点 ——
//    一个都没少。那份 JSON 会同时写着 `truncated: true` 与
//    `window_sample_count: 1800`，自相矛盾：覆盖过的环形序列必然满 2048。
//
//    真实原因不是覆盖，是**换过时间轴**（`segments == 2`）。
//    两个原因必须由两个字段回答。
void t02_seek_past_capacity_is_not_truncation() {
    pace::DriftSeries series;
    feed(series, 0, 1200);         // 段①：0 .. 59.95 s
    feed(series, 0, 1800);         // 段②：媒体时刻倒退 → 新段

    CHECK(series.segments() == 2);
    CHECK(series.total_samples() == 3000);      // 整场累计，越过了容量
    CHECK(series.segment_samples() == 1800);    // 本段只有 1800
    CHECK(series.size() == 1800);               // 窗口 = 本段全部，一个没丢

    // 旧判据会在这里给出 true —— 本卡修的就是这一行。
    CHECK(series.truncated() == false);

    // 窗口比累计少的 1200 个点，差额由 `segments` 解释，不由 `truncated` 解释。
    CHECK(series.total_samples() - series.size() == 1200);
}

// ③ 恒等式：`size() == min(segment_samples(), kCapacity)`。
//    四种形态各走一遍 —— 空、未满、刚满、覆盖后。
//
//    这条不是"顺手加的断言"：它是**报告层 `window_sample_count` 的唯一算法**。
//    校验器会拿 JSON 里的 `segment_accepted` / `capacity` 自己算一遍 min 去比，
//    这里守的是那个 min 在 C++ 侧真的成立。
void t03_window_size_identity_holds_in_every_shape() {
    pace::DriftSeries empty;
    CHECK(empty.size() == 0);
    CHECK(empty.segment_samples() == 0);
    CHECK(empty.truncated() == false);

    pace::DriftSeries partial;
    feed(partial, 0, 100);
    CHECK(partial.size() == 100);
    CHECK(partial.size() == partial.segment_samples());

    pace::DriftSeries exact;
    feed(exact, 0, pace::DriftSeries::kCapacity);
    CHECK(exact.size() == pace::DriftSeries::kCapacity);
    CHECK(exact.segment_samples() == pace::DriftSeries::kCapacity);
    // 刚好装满不算覆盖：没有点被"覆盖掉"，只是没地方写了。
    CHECK(exact.truncated() == false);

    pace::DriftSeries wrapped;
    feed(wrapped, 0, pace::DriftSeries::kCapacity + 1);
    CHECK(wrapped.size() == pace::DriftSeries::kCapacity);
    CHECK(wrapped.truncated() == true);
}

// ④ seek 之后**本段内**照样可能覆盖：判据看的是段，不是整场。
//    段① 只有 100 点，段② 3000 点 → 覆盖真的发生了，`segment_samples()` 说 3000。
void t04_truncation_is_judged_within_the_current_segment() {
    pace::DriftSeries series;
    feed(series, 0, 100);
    feed(series, 0, 3000);  // 倒退 → 新段

    CHECK(series.segments() == 2);
    CHECK(series.total_samples() == 3100);
    CHECK(series.segment_samples() == 3000);
    CHECK(series.size() == pace::DriftSeries::kCapacity);
    CHECK(series.truncated() == true);
}

// ⑤ `clear()` 之后的那一瞬间：活段是空的，两个数一起归零。
//
//    不归零会同时出现 `size() == 0` 与 `truncated() == true`——
//    一个"没有点的序列里有东西被覆盖了"的自相矛盾。
//    `media_span_ns()` 仍然报已关段的跨度：它回答的是"整场跨了多久"，
//    与本段的样本数不同口径，两者都该如实。
void t05_clear_zeroes_the_segment_count_but_keeps_the_span() {
    pace::DriftSeries series;
    feed(series, 0, 41);  // 0 .. 2000 ms
    CHECK(series.media_span_ns() == 2'000 * kMs);

    series.clear();
    CHECK(series.size() == 0);
    CHECK(series.segment_samples() == 0);
    CHECK(series.truncated() == false);
    CHECK(series.total_samples() == 41);          // 累计不清
    CHECK(series.media_span_ns() == 2'000 * kMs); // 已关段的跨度不清
}

// ⑥ `summarize_window()` 数的是序列里那几个点，不多不少。
//    极值走的是直方图自己的 min/max（精确，与桶无关），所以可以直接对序列比。
void t06_window_summary_counts_exactly_the_series() {
    pace::DriftSeries series;
    std::int64_t expected_max = -1'000'000'000;
    std::int64_t expected_min = 1'000'000'000;
    for (int i = 0; i < 200; ++i) {
        const std::int64_t lateness = (i % 7) * 1'000'000 - 3'000'000;  // -3 .. +3 ms
        series.note(static_cast<std::int64_t>(i) * kStep, lateness);
        if (lateness > expected_max) {
            expected_max = lateness;
        }
        if (lateness < expected_min) {
            expected_min = lateness;
        }
    }

    const pace::DriftWindow window = pace::summarize_window(series);
    CHECK(window.samples == series.size());
    CHECK(window.lateness.count() == series.size());
    CHECK(window.lateness.max_ns() == expected_max);
    CHECK(window.lateness.min_ns() == expected_min);
}

// ⑦ 未覆盖且单段：窗口分布与整场累计分布**逐桶相同**。
//    这是判据 I4 的单元级版本 —— 两个数本来就该是同一批点。
void t07_untruncated_single_segment_matches_the_cumulative_histogram() {
    pace::DriftSeries series;
    pace::SignedHistogram cumulative;
    for (int i = 0; i < 400; ++i) {
        const std::int64_t lateness = (i * 37 % 401 - 200) * 1'000;  // -200 .. +200 µs
        series.note(static_cast<std::int64_t>(i) * kStep, lateness);
        cumulative.add(lateness);
    }

    CHECK(series.truncated() == false);
    CHECK(series.segments() == 1);

    const pace::DriftWindow window = pace::summarize_window(series);
    CHECK(same_distribution(window.lateness, cumulative));
}

// ⑧ 覆盖之后：两份分布必然不同，但**窗口是累计的子集**，所以 max 不会更大。
//
//    前半句是 I4 的另一半：不同的时候必须能指出原因（这里 `truncated == true`）。
//    后半句"子集"抓的是另一类错：把被拒的帧、或者别的口径的样本掺进窗口分布，
//    掺进来之后桶对不上，而 max 有可能反而更大。
void t08_truncated_window_is_a_subset_of_the_cumulative_one() {
    pace::DriftSeries series;
    pace::SignedHistogram cumulative;
    for (int i = 0; i < 3000; ++i) {
        // 开头 500 个点埋一个很差的区间。**500 < 3000 - 2048 = 952**，
        // 所以它整段都会被环形覆盖掉 —— 这个数不能随手写：写成 1000 就有 48 个
        // 坏点留在窗口里，`max` 两边相等，下面那条"严格小于"会变成假红（本轮真撞过）。
        const std::int64_t lateness =
            (i < 500) ? (50 * kMs + i * 1'000) : ((i * 37 % 401) - 200) * 1'000;
        series.note(static_cast<std::int64_t>(i) * kStep, lateness);
        cumulative.add(lateness);
    }

    CHECK(series.truncated() == true);
    const pace::DriftWindow window = pace::summarize_window(series);
    CHECK(window.samples == pace::DriftSeries::kCapacity);
    CHECK(cumulative.count() == 3000);

    // 同一批样本，两种口径，两份不同的分布 —— 但窗口那份是子集。
    CHECK(!same_distribution(window.lateness, cumulative));
    CHECK(window.lateness.max_ns() <= cumulative.max_ns());
    // 被覆盖掉的那一段本来更差，所以这里是严格的"小于"。
    CHECK(window.lateness.max_ns() < cumulative.max_ns());
}

// ⑨ seek：窗口分布只覆盖最后一段。
//    段① 的 lateness 恒为 0、段② 恒为 +5 ms。窗口的 min 必须是 5 ms ——
//    若窗口分布把两段混在一起，min 会是 0。
void t09_window_covers_only_the_last_segment() {
    pace::DriftSeries series;
    feed(series, 0, 200, 0);          // 段①：全部 0
    feed(series, 0, 300, 5 * kMs);    // 段②：全部 +5 ms

    CHECK(series.segments() == 2);
    CHECK(series.total_samples() == 500);
    CHECK(series.segment_samples() == 300);

    const pace::DriftWindow window = pace::summarize_window(series);
    CHECK(window.samples == 300);
    CHECK(window.lateness.min_ns() == 5 * kMs);
    CHECK(window.lateness.max_ns() == 5 * kMs);
    // 累计口径则是两段混在一起（`sampled_lateness.min_ns()` 会是 0）——
    // 那正是 `t10` 通过 `PaceStats` 核对的事。
}

// ⑩ 一路走到 `PaceSummary`：字段真的接上了，不是只活在 `DriftSeries` 里。
//
//    上一卡撞过四次"改了一半"，所以这里从 `note_presented()` 走到 summary，
//    同时核对累计/窗口两个口径在同一份数据上确实是两批点。
void t10_pace_summary_carries_both_calibres() {
    pace::PaceStats stats;
    std::int64_t media_ns = 0;
    for (int i = 0; i < 200; ++i) {          // 段①：lateness 0
        stats.note_presented(0, media_ns);
        media_ns += kStep;
    }
    media_ns = 0;                            // 媒体时刻倒退 → seek
    for (int i = 0; i < 300; ++i) {          // 段②：lateness +5 ms
        stats.note_presented(5 * kMs, media_ns);
        media_ns += kStep;
    }

    const pace::PaceSummary summary =
        stats.summary(33 * kMs, pace::ClockHealth::kFresh);

    CHECK(summary.sampled_samples == 500);                 // 整场累计
    CHECK(summary.drift_segment_samples == 300);           // 本段
    CHECK(summary.window_lateness.samples == 300);         // 窗口 = 本段（未覆盖）
    CHECK(summary.drift.segments == 2);
    CHECK(summary.drift.truncated == false);

    // 累计分布混了两段：min 是段① 的 0。
    CHECK(summary.sampled_lateness.min_ns() == 0);
    CHECK(summary.sampled_lateness.count() == 500);
    // 窗口分布只有段②：min 是 5 ms。两个数并排摆着，谁也不会把它们当同一批。
    CHECK(summary.window_lateness.lateness.min_ns() == 5 * kMs);
    CHECK(summary.window_lateness.lateness.count() == 300);

    // 恒等式在 `PaceSummary` 上也要成立（报告层的 `window_sample_count` 就是它）。
    CHECK(summary.window_lateness.samples == summary.drift_segment_samples);
}

// ⑪ 空序列：分布是空的，`samples` 是 0，而不是"量到零"。
//    与 `not_measured ⇒ quantiles 为空对象` 是同一条规矩在分布层的版本。
void t11_empty_window_has_no_samples() {
    const pace::DriftSeries empty;
    const pace::DriftWindow window = pace::summarize_window(empty);
    CHECK(window.samples == 0);
    CHECK(window.lateness.count() == 0);
    CHECK(window.lateness.max_ns() == 0);
}

}  // namespace

int main() {
    std::printf("P13-I window tests\n");
    t01_single_segment_wraparound_is_truncation();
    t02_seek_past_capacity_is_not_truncation();
    t03_window_size_identity_holds_in_every_shape();
    t04_truncation_is_judged_within_the_current_segment();
    t05_clear_zeroes_the_segment_count_but_keeps_the_span();
    t06_window_summary_counts_exactly_the_series();
    t07_untruncated_single_segment_matches_the_cumulative_histogram();
    t08_truncated_window_is_a_subset_of_the_cumulative_one();
    t09_window_covers_only_the_last_segment();
    t10_pace_summary_carries_both_calibres();
    t11_empty_window_has_no_samples();
    std::printf("\n%d 项断言 / %d 项失败\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
