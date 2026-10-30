// worst_tests.cpp — P13-F tests: 最差那一下在哪儿
//
// 这一组守的是五句话：
//   ① **没有样本 = 没有位置**，不是"位置在第 0 个" —— 后者是一个看起来
//      完全正常的假观测，会被读进表格。
//   ② **有符号最大值，不是绝对值最大值。** 漂移可以为负；取绝对值会让
//      `worst` 与直方图的 `max_ns()` 分家（判据 F1 抓这个）。
//   ③ **`index` 与 `lateness_ns` 必须来自同一个点。** 两个字段各自维护时
//      最容易出现的错就是把 A 点的位置配上 B 点的值。
//   ④ **环形覆盖与 `clear()` 之后极值仍在**，且 `of` 是累计数。
//      扫序列的实现前两条都能过，到这一条才露出来。
//   ⑤ **`index` 是 0 基、与直方图 `count()` 同步。** 差一位的错在真机上
//      看起来像"最差发生在第 1 个样本"，而它其实在开头。
//
// 纯桌面计算：没有设备、没有时钟、没有睡眠。
#include <cstdint>
#include <cstdio>

#include "video/video_plane/core/pace/pace_drift_series.hpp"
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

// ---- ① 空序列：没有位置，不是位置 0 ----------------------------------------
void t01_empty_has_no_position() {
    const pace::DriftSeries series;
    const pace::DriftExtreme worst = series.max_point();

    CHECK(worst.valid == false);
    // `of` 也不能被读成一个合法样本数。
    CHECK(worst.of == 0);
}

// ---- ② 单点：位置就是 0，of 是 1 -------------------------------------------
void t02_single_point() {
    pace::DriftSeries series;
    CHECK(series.note(0, 7 * kMs) == true);

    const pace::DriftExtreme worst = series.max_point();
    CHECK(worst.valid == true);
    CHECK(worst.index == 0);
    CHECK(worst.of == 1);
    CHECK(worst.media_ns == 0);
    CHECK(worst.lateness_ns == 7 * kMs);
}

// ---- ③ 最大值在中段：index 逐字对齐（防止差一位） ---------------------------
void t03_max_in_the_middle() {
    pace::DriftSeries series;
    // 5 个点，第 3 个（0 基 index = 2）最大。
    for (int i = 0; i < 5; ++i) {
        const std::int64_t lateness = (i == 2) ? 9 * kMs : static_cast<std::int64_t>(i) * kMs;
        CHECK(series.note(static_cast<std::int64_t>(i) * kStep, lateness) == true);
    }

    const pace::DriftExtreme worst = series.max_point();
    CHECK(worst.valid == true);
    CHECK(worst.index == 2);
    CHECK(worst.of == 5);
    CHECK(worst.media_ns == 2 * kStep);
    CHECK(worst.lateness_ns == 9 * kMs);
}

// ---- ② 全负：取"最接近零"的那个，不是绝对值最大的 -------------------------
//
// 这是本组最重要的一条。视频 pts 超前音频时漂移为负，
// 一条 −90 ms 的样本比 −1 ms 更"异常"，但 `max` 的语义是**有符号最大值** ——
// 直方图那边就是这么算的。若这里取绝对值，两个字段会对同一条证据给出两个答案。
void t04_all_negative_is_signed_max() {
    pace::DriftSeries series;
    CHECK(series.note(0, -90 * kMs) == true);
    CHECK(series.note(kStep, -1 * kMs) == true);
    CHECK(series.note(2 * kStep, -50 * kMs) == true);

    const pace::DriftExtreme worst = series.max_point();
    CHECK(worst.valid == true);
    // 有符号最大 = −1 ms（在第 1 个样本）。
    CHECK(worst.lateness_ns == -1 * kMs);
    CHECK(worst.index == 1);
    // 绝对值最大会是 −90 ms（第 0 个）—— 明确排除。
    CHECK(worst.index != 0);
}

// ---- 并列：取时间上最早的那个，遍历方向变了结论也不变 ---------------------
void t05_tie_takes_earliest() {
    pace::DriftSeries series;
    CHECK(series.note(0, 3 * kMs) == true);          // 第一个 3 ms
    CHECK(series.note(kStep, 5 * kMs) == true);      // 最大
    CHECK(series.note(2 * kStep, 5 * kMs) == true);  // 并列，更晚

    const pace::DriftExtreme worst = series.max_point();
    CHECK(worst.lateness_ns == 5 * kMs);
    CHECK(worst.index == 1);
    CHECK(worst.media_ns == kStep);
}

// ---- 抽稀门拒掉的帧不参与极值 ---------------------------------------------
//
// `note()` 返回 false 时这一帧**没进样本**，直方图也没加它。
// 若极值在返回 false 的分支里也更新，`worst` 会指到一个直方图里根本不存在的值，
// 判据 F1（worst == max）当场红 —— 而红的地方离病根很远。
void t06_rejected_frames_do_not_count() {
    pace::DriftSeries series;
    CHECK(series.note(0, 1 * kMs) == true);
    // 10 ms 后那一帧没跨格点 → 被拒，它的 88 ms 不该成为极值。
    CHECK(series.note(10 * kMs, 88 * kMs) == false);

    const pace::DriftExtreme worst = series.max_point();
    CHECK(worst.valid == true);
    CHECK(worst.of == 1);
    CHECK(worst.lateness_ns == 1 * kMs);
}

// ---- ④ 环形覆盖：极值仍在，且 of 是累计数 ---------------------------------
//
// 扫序列的实现在这一条上必红：覆盖后序列里只剩最近 2048 点，
// 而最大值发生在第 0 个 —— 它已经不在了。
void t07_survives_ring_overwrite() {
    pace::DriftSeries series;
    const std::int64_t capacity = pace::DriftSeries::kCapacity;
    const std::int64_t total = capacity + 60;

    for (std::int64_t i = 0; i < total; ++i) {
        // 第 0 个是 120 ms（全序列最大），其余都是 1 ms。
        const std::int64_t lateness = (i == 0) ? 120 * kMs : 1 * kMs;
        CHECK(series.note(i * kStep, lateness) == true);
    }

    CHECK(series.truncated() == true);
    const pace::DriftExtreme worst = series.max_point();
    CHECK(worst.valid == true);
    // 那个点早被覆盖了，但极值是累计的 —— 仍然指得到。
    CHECK(worst.lateness_ns == 120 * kMs);
    CHECK(worst.index == 0);
    // `of` 是累计接受数，不是窗口内 size（2048）。
    CHECK(worst.of == total);
    CHECK(worst.of != capacity);
}

// ---- ④ `clear()` 不清极值（与 total_ / segments_ 同理） --------------------
void t08_clear_keeps_extreme() {
    pace::DriftSeries series;
    CHECK(series.note(100 * kMs, 40 * kMs) == true);
    series.clear();

    CHECK(series.size() == 0);
    const pace::DriftExtreme worst = series.max_point();
    CHECK(worst.valid == true);
    CHECK(worst.lateness_ns == 40 * kMs);
    // total_ 不清，所以 of 仍是 1。
    CHECK(worst.of == 1);
}

// ---- ③ 位置与值必须来自同一个点 -------------------------------------------
//
// 构造一列"媒体时刻递增、lateness 递减"的点：若实现把 max 值配上了
// 最后一个点的 media_ns，两个字段都"合法"，但配对是错的。
void t09_position_and_value_pair_up() {
    pace::DriftSeries series;
    for (int i = 0; i < 6; ++i) {
        const std::int64_t lateness = (5 - i) * 10 * kMs;  // 50,40,30,20,10,0
        CHECK(series.note(static_cast<std::int64_t>(i) * kStep, lateness) == true);
    }

    const pace::DriftExtreme worst = series.max_point();
    CHECK(worst.lateness_ns == 50 * kMs);
    CHECK(worst.index == 0);
    CHECK(worst.media_ns == 0);  // 最大值那个点的媒体时刻，不是末点的
}

// ---- ⑤ PaceStats 逐帧口径：index 0 基、与直方图同步 ------------------------
void t10_pace_stats_frame_worst_index_is_zero_based() {
    pace::PaceStats stats;
    // 第 0 个样本就是最大 —— 差一位的实现会报 index = 1。
    stats.note_presented(8 * kMs, 0);
    stats.note_presented(1 * kMs, kStep);
    stats.note_presented(2 * kMs, 2 * kStep);

    const pace::PaceSummary summary = stats.summary(16 * kMs, pace::ClockHealth::kFresh);
    CHECK(summary.frame_worst.valid == true);
    CHECK(summary.frame_worst.index == 0);
    CHECK(summary.frame_worst.of == 3);
    CHECK(summary.frame_worst.media_ns == 0);
}

// ---- ⑤ 逐帧口径与直方图同口径（F1 的单元版） ------------------------------
void t11_pace_stats_frame_worst_matches_histogram_max() {
    pace::PaceStats stats;
    const std::int64_t values[] = {3 * kMs, -20 * kMs, 11 * kMs, -1 * kMs, 6 * kMs};
    for (int i = 0; i < 5; ++i) {
        stats.note_presented(values[i], static_cast<std::int64_t>(i) * kStep);
    }

    const pace::PaceSummary summary = stats.summary(16 * kMs, pace::ClockHealth::kFresh);
    // 逐位相等 —— 不是"差不多"。
    CHECK(summary.frame_worst.lateness_ns == summary.lateness.max_ns());
    CHECK(summary.frame_worst.lateness_ns == 11 * kMs);
    CHECK(summary.frame_worst.index == 2);
    CHECK(summary.frame_worst.of == summary.lateness.count());
}

// ---- 抽稀口径同样与它的直方图同口径 ---------------------------------------
void t12_pace_stats_sampled_worst_matches_histogram_max() {
    pace::PaceStats stats;
    for (int i = 0; i < 4; ++i) {
        const std::int64_t lateness = (i == 2) ? 17 * kMs : 1 * kMs;
        stats.note_presented(lateness, static_cast<std::int64_t>(i) * kStep);
    }

    const pace::PaceSummary summary = stats.summary(16 * kMs, pace::ClockHealth::kFresh);
    CHECK(summary.sampled_worst.valid == true);
    CHECK(summary.sampled_worst.lateness_ns == summary.sampled_lateness.max_ns());
    CHECK(summary.sampled_worst.lateness_ns == 17 * kMs);
    CHECK(summary.sampled_worst.index == 2);
    CHECK(summary.sampled_worst.of == summary.sampled_lateness.count());
}

}  // namespace

int main() {
    std::printf("P13-F worst-point tests\n");
    t01_empty_has_no_position();
    t02_single_point();
    t03_max_in_the_middle();
    t04_all_negative_is_signed_max();
    t05_tie_takes_earliest();
    t06_rejected_frames_do_not_count();
    t07_survives_ring_overwrite();
    t08_clear_keeps_extreme();
    t09_position_and_value_pair_up();
    t10_pace_stats_frame_worst_index_is_zero_based();
    t11_pace_stats_frame_worst_matches_histogram_max();
    t12_pace_stats_sampled_worst_matches_histogram_max();
    std::printf("\n%d 项断言 / %d 项失败\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
