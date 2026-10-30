// segment_tests.cpp — P13-M tests: 段边界由**锚点重建**决定，不由 pts 的走向推断
//
// 这一组守四句话：
//   ① **pts 前进推不出"没换时间轴"。** 向前 seek 与真卡顿在 pts 上长得一模一样，
//      所以段边界不能由 pts 推断 —— 只能由那个确实知道的地方（`FramePacer::reset()`）
//      在事件发生时记下来。做不到这条，P13-L 那份缺陷证据就会重现：
//      30 s 处跳到 40 s，报 `segments = 1`、`span = 112.367 s`。
//   ② **pts 倒退仍然切段。** 兜底路径不许因为有了主判据就被拆掉 ——
//      倒退是充分条件，它永远成立。
//   ③ **切段不作废"整场"计数。** `total_samples` / `presented` / 各直方图
//      回答的是"整场发生过什么"，一个都不许动。
//   ④ **段内的空洞不切段，但要出版。** 卡顿与向前 seek 无法区分，切了会丢掉
//      卡顿之前的真数据；不出版则 `span_ns` 含着一个洞而没人知道。
//
// 纯桌面：没有设备、没有真时钟、没有真睡眠。
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"
#include "video/video_plane/core/pace/frame_pacer.hpp"
#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_drift_trend.hpp"
#include "video/video_plane/core/pace/pace_policy.hpp"
#include "video/video_plane/core/pace/pace_stats.hpp"

namespace core = kivo::video::core;
namespace pace = kivo::video::core::pace;
namespace audio = kivo::video::core::audio;

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
constexpr std::int64_t kCell = pace::DriftSeries::kSampleIntervalNs;

// ---- 假时间轴（只在 m12 用得到）--------------------------------------------

std::int64_t g_now_ns = 0;

[[nodiscard]] std::int64_t fake_now() { return g_now_ns; }

void fake_wait(std::int64_t deadline_ns) { g_now_ns = deadline_ns; }

class FakeClock final : public audio::AudioClockSource {
public:
    [[nodiscard]] audio::AudioClockSnapshot now() override {
        audio::AudioClockSnapshot snapshot;
        // 位置恒为 0：这一组测的是"段边界记在哪"，不是"迟到多少"。
        // 让时钟停着，各行的时间轴就只由 pts 决定，断言里不必带容差。
        snapshot.position_ns = 0;
        snapshot.source = audio::AudioClockSourceKind::kSystem;
        snapshot.stale = false;
        snapshot.monotonic_rejected = false;
        return snapshot;
    }
};

struct Harness {
    FakeClock clock;
    pace::FramePacer pacer;

    Harness()
        : pacer(pace::FramePacer::Setup{&clock, pace::PacePolicy{}, &fake_now, &fake_wait,
                                        nullptr}) {}
};

/// 喂一个点，返回是否进了序列。
bool feed(pace::DriftSeries& series, std::int64_t media_ns, std::int64_t lateness_ns = 0) {
    return series.note(media_ns, lateness_ns);
}

// ---- 用例 -------------------------------------------------------------------

// ① 空会话上 `break_segment()` 什么也不做。
//
//    每个会话的 `start()` 都会走一次 `FramePacer::reset()`。若那里把段数 +1，
//    每一份证据都会从 `segments = 2` 起步 —— 一个恒真的告警，读者只会学会忽略它。
void m01_breaking_an_empty_session_is_a_no_op() {
    pace::DriftSeries series;
    series.break_segment();
    CHECK(series.segments() == 1);
    CHECK(series.size() == 0);
    CHECK(series.total_samples() == 0);
    CHECK(series.media_span_ns() == 0);
}

// ③ 切段之后：环形与段内计数归零，整场计数一个不动。
void m02_breaking_keeps_the_session_counters() {
    pace::DriftSeries series;
    feed(series, 0);
    feed(series, 50 * kMs);
    feed(series, 100 * kMs);
    CHECK(series.size() == 3);
    CHECK(series.segment_samples() == 3);
    CHECK(series.total_samples() == 3);
    CHECK(series.segments() == 1);
    CHECK(series.media_span_ns() == 100 * kMs);

    series.break_segment();

    CHECK(series.size() == 0);
    CHECK(series.segment_samples() == 0);
    // 整场计数与已结束各段的跨度都不清。
    CHECK(series.total_samples() == 3);
    CHECK(series.observed_frames() == 3);
    CHECK(series.segments() == 2);
    CHECK(series.media_span_ns() == 100 * kMs);
}

// ① 本卡的主断言：切段之后，一个**前进**的 pts 也能起新段。
//
//    没有 `break_segment()` 的话，500 ms 会被当成"跨过了 9 个格点"，
//    以 segments 仍为 1 的方式拼进上一段 —— P13-L 那份缺陷证据就是这个形状。
void m03_a_forward_point_starts_a_new_segment_after_a_break() {
    pace::DriftSeries series;
    feed(series, 0);
    feed(series, 50 * kMs);
    feed(series, 100 * kMs);

    series.break_segment();
    CHECK(feed(series, 500 * kMs));

    CHECK(series.segments() == 2);
    CHECK(series.size() == 1);
    CHECK(series.segment_samples() == 1);
    CHECK(series.at(0).media_ns == 500 * kMs);
    // 新段的第一个点不构成"空洞"：洞是段内的概念，跨段的那段空白由
    // `media_span_ns()` 两边各算各的（见 m07）。
    CHECK(series.largest_gap_cells() == 0);
    CHECK(series.contiguous());
}

// ④ 不切段的向前跳 = 段内空洞。**段数不变，但要把洞出版。**
//
//    这条刻意断言 `segments == 1`：卡顿与向前 seek 在 pts 上无法区分，
//    拿它切段会把卡顿之前的真数据全部作废。所以这里的正确行为是"忍着"，
//    代价是 `span_ns` 里含着一个十秒的洞 —— 由 `contiguous` 把它说出来。
void m04_a_forward_jump_without_a_break_is_a_gap_not_a_segment() {
    pace::DriftSeries series;
    feed(series, 0);
    feed(series, 50 * kMs);
    feed(series, 100 * kMs);
    // 从 100 ms 直接跳到 10.1 s：next_due 是 150 ms，(10100−150)/50 = 199 格。
    feed(series, 10'100 * kMs);

    CHECK(series.segments() == 1);
    CHECK(series.largest_gap_cells() == 199);
    CHECK(!series.contiguous());
    CHECK(series.unobserved_cells() == 199);
    // 跨度把洞算进去了 —— 这正是要出版 `largest_gap_cells` 的理由。
    CHECK(series.media_span_ns() == 10'100 * kMs);
}

// ④ 边界的数就是常量本身：20 格算连续，21 格不算。
void m05_the_contiguity_boundary_is_twenty_cells() {
    constexpr std::int64_t kAt = pace::DriftSeries::kMaxContiguousGapCells;

    pace::DriftSeries exact;
    feed(exact, 0);
    // next_due = 50 ms；要对齐到 kAt 格，落点 = 50 + kAt×50。
    feed(exact, kCell * (1 + kAt));
    CHECK(exact.largest_gap_cells() == kAt);
    CHECK(exact.contiguous());

    pace::DriftSeries over;
    feed(over, 0);
    feed(over, kCell * (1 + kAt) + kCell);
    CHECK(over.largest_gap_cells() == kAt + 1);
    CHECK(!over.contiguous());
}

// ④ 上一段的洞不许继承到下一段 —— 否则切段之后仍然报"不连续"，
//    而那个洞根本不在被拟合的这段里。
void m06_a_break_clears_the_gap_reading() {
    pace::DriftSeries series;
    feed(series, 0);
    feed(series, 10'100 * kMs);
    CHECK(!series.contiguous());

    series.break_segment();
    CHECK(series.largest_gap_cells() == 0);
    CHECK(series.contiguous());

    feed(series, 20'000 * kMs);
    CHECK(series.largest_gap_cells() == 0);
    CHECK(series.contiguous());
}


//
//    这就是 P13-L 那份缺陷证据的算术：第一段 0 → 30 s，第二段 40 → 130 s。
//    "末 − 首" 会给出 130 s，而实际走过的只有 30 + 90 = 120 s。
void m07_media_span_sums_segments_instead_of_spanning_the_hole() {
    pace::DriftSeries series;
    feed(series, 0);
    feed(series, 30'000 * kMs);
    CHECK(series.media_span_ns() == 30'000 * kMs);

    series.break_segment();
    feed(series, 40'000 * kMs);
    feed(series, 130'000 * kMs);

    CHECK(series.media_span_ns() == 120'000 * kMs);
    CHECK(series.media_span_ns() != 130'000 * kMs);
    // 窗口跨度只覆盖最后一段。
    CHECK(series.at(series.size() - 1).media_ns - series.at(0).media_ns == 90'000 * kMs);
}

// ① 连切两次而中间没有样本，段数只涨一次。
//    "有东西可关"才是切段发生的判据，不是"调用过 break_segment"。
void m08_consecutive_breaks_count_once() {
    pace::DriftSeries series;
    feed(series, 0);
    series.break_segment();
    series.break_segment();
    series.break_segment();
    CHECK(series.segments() == 2);
}

// ② 兜底：pts 倒退仍然自己切段（没有 pacer 的调用方也能正常工作）。
void m09_a_backward_point_still_breaks_by_itself() {
    pace::DriftSeries series;
    feed(series, 0);
    feed(series, 50 * kMs);
    feed(series, 100 * kMs);
    feed(series, 150 * kMs);

    feed(series, 20 * kMs);  // 倒退

    CHECK(series.segments() == 2);
    CHECK(series.size() == 1);
    CHECK(series.at(0).media_ns == 20 * kMs);
    CHECK(series.total_samples() == 5);
}

// ① 切段后的第一帧即便距上一次接受不足一格也必须采。
//    否则新时间轴要等 50 ms 才有第一个点，而 seek 之后头几帧恰好是
//    "新锚点稳不稳"最该看的地方。
void m10_the_first_frame_after_a_break_is_always_accepted() {
    pace::DriftSeries series;
    feed(series, 0);
    feed(series, 50 * kMs);
    CHECK(series.size() == 2);

    series.break_segment();
    // 距上一次接受只有 10 ms，正常会被抽稀门拒掉。
    CHECK(feed(series, 60 * kMs));
    CHECK(series.size() == 1);
    CHECK(series.skipped_frames() == 0);
}

// ③ `PaceStats::note_anchor_reset()` 只切段，不碰任何累计计数。
void m11_pace_stats_forwards_the_break_without_touching_counters() {
    pace::PaceStats stats;
    stats.note_presented(0, 0);
    stats.note_presented(0, 50 * kMs);
    // 制造一个 200 格的洞：next_due 是 100 ms，(10100−100)/50 = 200，
    // 走的是"段内空洞"那条路，段数不变。
    stats.note_presented(0, 10'100 * kMs);

    pace::PaceSummary before = stats.summary(33'333'333, pace::ClockHealth::kFresh);
    CHECK(before.drift.segments == 1);
    CHECK(before.drift_segment_largest_gap_cells == 200);
    CHECK(!before.drift_segment_contiguous);
    CHECK(before.sampled_samples == 3);

    stats.note_anchor_reset();
    stats.note_presented(0, 5'000 * kMs);

    const pace::PaceSummary summary = stats.summary(33'333'333, pace::ClockHealth::kFresh);
    CHECK(summary.drift.segments == 2);
    CHECK(summary.window_lateness.samples == 1);
    // "整场"的计数一个都不许动。
    CHECK(summary.presented == 4);
    CHECK(summary.sampled_samples == 4);
    CHECK(summary.drift_observed_frames == 4);
    // 段内的洞跟着切段归零 —— 上一段的洞不属于这一段。
    CHECK(summary.drift_segment_largest_gap_cells == 0);
    CHECK(summary.drift_segment_contiguous);
}

// ① 整张卡的那一行：`FramePacer::reset()` 必须切段。
//
//    真机上 `DecodeSession::begin_run()` 在 seek 时调它，所以这条同时守住
//    "报告层看到的 `segments` 到底是什么"。删掉 `reset()` 里那一行，
//    这条会红 —— 而其余十一条全部照旧绿，因为它测的是 pacer 的接线，
//    不是序列自己的行为。
void m12_frame_pacer_reset_breaks_the_segment() {
    g_now_ns = 0;
    Harness h;

    const pace::PaceDecision first = h.pacer.arrive(0);
    h.pacer.note_presented(first, g_now_ns);
    const pace::PaceDecision second = h.pacer.arrive(50 * kMs);
    h.pacer.note_presented(second, g_now_ns);

    CHECK(h.pacer.summary().drift.segments == 1);
    CHECK(h.pacer.summary().window_lateness.samples == 2);

    // 锚点重建。真机上这一步就是 seek。
    h.pacer.reset();
    // 向后 5 秒的大跳 —— 注意它是**前进**的：没有 `reset()` 里那一行，
    // 它会被当成"跨过了 99 个格点"，以 segments 仍为 1 的方式拼进上一段。
    const pace::PaceDecision third = h.pacer.arrive(5'000 * kMs);
    h.pacer.note_presented(third, g_now_ns);

    const pace::PaceSummary summary = h.pacer.summary();
    CHECK(summary.drift.segments == 2);
    CHECK(summary.window_lateness.samples == 1);
    // 段数涨了，整场计数不动。
    CHECK(summary.presented == 3);
    CHECK(summary.sampled_samples == 3);
}

// ④ 趋势在 `usable == false` 时**仍然带着**段内空洞。
//
//    一条短到算不出斜率的证据同样需要知道"这段里有个多宽的洞"，
//    否则不可用的原因会只剩 "fewer than 30 samples"，而真实原因可能是
//    那些点被一个大洞摊在一百秒里 —— 那两句话导向的修法完全不同。
//    （这条钉的是"不可用 ≠ 什么都不知道"；`n == 0` 那种提前返回无法用
//    真实数据构造出非零空洞，所以不另设用例。）
void m13_short_series_still_reports_its_gap() {
    pace::DriftSeries series;
    feed(series, 0);
    feed(series, 50 * kMs);
    feed(series, 100 * kMs);
    feed(series, 10'100 * kMs);  // 同上：next_due 是 150 ms，(10100−150)/50 = 199 格

    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(!trend.usable);
    CHECK(trend.unusable_reason != nullptr && trend.unusable_reason[0] != '\0');
    CHECK(trend.segment_largest_gap_cells == 199);
    CHECK(!trend.segment_contiguous);
    CHECK(trend.segments == 1);
}

// ④ 段内最宽空洞是 **max**，不是累加。
//
//    这条单独存在是因为一个容易写出的"更自然"版本：把每次跳过的格点累加起来。
//    两者在只有一个洞时给出同一个数，于是所有单洞用例都放它过去 ——
//    而回归的可信度取决于**最宽**的那个洞：十个 100 ms 的小洞不该让一条直线
//    失去意义，一个 10 s 的大洞该。累加会把前者也算成后者。
void m14_the_gap_is_a_maximum_not_a_sum() {
    pace::DriftSeries series;
    feed(series, 0);          // next_due = 50 ms
    feed(series, 300 * kMs);  // (300−50)/50 = 5 格
    feed(series, 500 * kMs);  // next_due=350，(500−350)/50 = 3 格

    CHECK(series.largest_gap_cells() == 5);
    CHECK(series.largest_gap_cells() != 8);
    CHECK(series.unobserved_cells() == 8);  // 累计那份是 5+3
    CHECK(series.contiguous());             // 5 格远低于阈值，不该被 8 影响
}

}  // namespace

int main() {
    m01_breaking_an_empty_session_is_a_no_op();
    m02_breaking_keeps_the_session_counters();
    m03_a_forward_point_starts_a_new_segment_after_a_break();
    m04_a_forward_jump_without_a_break_is_a_gap_not_a_segment();
    m05_the_contiguity_boundary_is_twenty_cells();
    m06_a_break_clears_the_gap_reading();
    m07_media_span_sums_segments_instead_of_spanning_the_hole();
    m08_consecutive_breaks_count_once();
    m09_a_backward_point_still_breaks_by_itself();
    m10_the_first_frame_after_a_break_is_always_accepted();
    m11_pace_stats_forwards_the_break_without_touching_counters();
    m12_frame_pacer_reset_breaks_the_segment();
    m13_short_series_still_reports_its_gap();
    m14_the_gap_is_a_maximum_not_a_sum();

    std::printf("p13m segment tests: %d checks, %d failed\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
