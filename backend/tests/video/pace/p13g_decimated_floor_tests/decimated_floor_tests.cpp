// decimated_floor_tests.cpp — P13-G tests: 抽稀的最小可用门槛
//
// 这一组守的是四句话：
//   ① **1 个样本不是抽稀数据。** 它的 p50/p95/p99/max 四个数完全相同，
//      长得像一份正常数据，E1/E2/E3/F1/F2′ 全套判据都放它过去。
//   ② **缺 pts 不会降级，会静默产 1 个样本。** `DecodedFrame::presentation_time_us`
//      默认 0，首帧接受、其后全部 `skipped` —— 单测把"如果发生会长成什么样"钉住。
//   ③ **`kPerFrame` 在旧判据下不可达。** `lateness.count() > 0` 蕴含
//      `sampled_samples >= 1`：两者在同一个函数体里，中间只隔一次无条件接受的调用。
//      这条用随机调用序列扫，不靠嘴说。
//   ④ **理由只有一个出处。** `decimated_unusable_reason()` 是唯一回答"为什么"的地方，
//      它被逐字钉着，谁想编第二句话就必须先改这里。
//
// 纯桌面计算：没有设备、没有时钟、没有睡眠。
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>

#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_stats.hpp"
#include "video/video_plane/core/pace/sampling_mode.hpp"

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

// ---- ① 门槛：数值一个都不能松 ----------------------------------------------

void t01_floor_is_two() {
    // 阈值是线格式契约的一部分，改它等于改判据 G1。钉死。
    CHECK(pace::kMinDecimatedSamples == 2);
}

void t02_zero_samples_is_unusable() {
    CHECK(!pace::decimated_block_is_usable(0, 0));
    CHECK(!pace::decimated_block_is_usable(0, 1'000'000'000));
    CHECK(std::strcmp(pace::decimated_unusable_reason(0, 0),
                      "no decimated sample was accepted") == 0);
}

void t03_single_sample_is_unusable_even_with_span() {
    // 1 个样本不可能有跨度（跨度是首末差），但即便调用方给了个非零跨度，
    // 门槛也必须拦住 —— 判据不能依赖"调用方永远自洽"。
    CHECK(!pace::decimated_block_is_usable(1, 0));
    CHECK(!pace::decimated_block_is_usable(1, 50 * kMs));
    CHECK(std::strcmp(pace::decimated_unusable_reason(1, 0),
                      "media time did not reach the second 50 ms grid point (single sample)") == 0);
}

void t04_two_samples_without_span_is_unusable() {
    // 两帧 pts 相同 / pts 在重复：采到两次，但媒体时间没走。
    CHECK(!pace::decimated_block_is_usable(2, 0));
    CHECK(!pace::decimated_block_is_usable(2, -50 * kMs));
    CHECK(std::strcmp(pace::decimated_unusable_reason(2, 0),
                      "media time does not advance across the accepted samples") == 0);
}

void t05_two_samples_one_cell_apart_is_usable() {
    // 结构门的下界：两点、一格。**够当抽稀数据，不够谈斜率** —— 后者归
    // `DriftTrend::kMinSamples`，两件事不能用一个阈值。
    CHECK(pace::decimated_block_is_usable(2, kStep));
    CHECK(pace::decimated_block_is_usable(160, 7'950 * kMs));

    // 可用时问理由，必须得到**空串**，而不是一句"媒体时间不推进"的假话。
    // 这是 `decimated_unusable_reason` 的完整契约：非空返回必须是真的。
    CHECK(pace::decimated_unusable_reason(2, kStep)[0] == '\0');
    CHECK(pace::decimated_unusable_reason(160, 7'950 * kMs)[0] == '\0');
}

// ---- ② 缺 pts 的真实形态：1 个样本、跨度 0 ----------------------------------

void t06_missing_pts_yields_a_single_sample() {
    // 复刻 `DecodedFrame::presentation_time_us` 默认 0 的那条流：
    // 每一帧的 media_ns 都是 0。首帧被接受，其后全部 `skipped`。
    pace::PaceStats stats;
    for (int i = 0; i < 480; ++i) {
        stats.note_presented(/*lateness_ns=*/1 * kMs, /*media_ns=*/0);
    }

    const pace::PaceSummary summary = stats.summary(16 * kMs, pace::ClockHealth::kFresh);

    // 逐帧那份是满的 —— 这就是为什么它看起来"有数据"。
    CHECK(summary.lateness.count() == 480);
    // 抽稀只有 1 个。
    CHECK(summary.sampled_samples == 1);
    CHECK(summary.sampled_lateness.count() == 1);
    CHECK(summary.drift.span_ns == 0);

    // 旧判据（`sampled_samples > 0`）会认为抽稀成立 —— 这就是缺陷本身。
    CHECK(summary.sampled_samples > 0);
    // 新门槛必须拦住它。
    CHECK(!pace::decimated_block_is_usable(summary.sampled_samples, summary.drift.span_ns));

    // 四个分位数完全相同，是它唯一的破绽。
    CHECK(summary.sampled_lateness.percentile_ns(0.50) ==
          summary.sampled_lateness.percentile_ns(0.99));
    CHECK(summary.sampled_lateness.max_ns() == summary.sampled_lateness.percentile_ns(0.50));
}

// ---- ③ `kPerFrame` 在旧判据下不可达：随机序列扫 ------------------------------

void t07_lateness_implies_at_least_one_decimated_sample() {
    // 这条链是本卡第一节的全部论证，用随机序列而非推理来验：
    //   `lateness.count() > 0` ⇒ `sampled_samples >= 1`
    //
    // 若这里出现反例，说明 `note_presented()` 里"加 lateness"与"喂序列"
    // 被拆开了 —— 那 `kPerFrame` 就重新变成可达路径，本卡的修法也要重来。
    //
    // **随机不能替代显式。** 第一版只写了下面的随机段，把
    // `if (media_ns > 0 && drift_.note(...))` 这个变异喂给它时**没红** ——
    // 因为媒体时刻从 0 单调累加，只有"只调一帧且步进恰为 0"才会碰上，
    // 概率约 0.08%，200 次试验期望命中 0.16 次。
    // 所以先写显式的最小反例，再拿随机扫别的组合。
    {
        // 一帧、媒体时刻 0（缺 pts 的形态，也是"首帧 pts 真的是 0"）：
        // 序列**必须**接受它。加了 `media_ns > 0` 的门，这里立刻红。
        pace::PaceStats one;
        one.note_presented(1 * kMs, 0);
        const pace::PaceSummary summary = one.summary(16 * kMs, pace::ClockHealth::kFresh);
        CHECK(summary.lateness.count() == 1);
        CHECK(summary.sampled_samples == 1);
    }

    std::mt19937 rng(20261030u);
    std::uniform_int_distribution<std::int64_t> lateness_dist(-40 * kMs, 40 * kMs);
    std::uniform_int_distribution<std::int64_t> media_step(0, 20 * kMs);
    std::uniform_int_distribution<int> count_dist(0, 60);

    for (int trial = 0; trial < 200; ++trial) {
        pace::PaceStats stats;
        const int frames = count_dist(rng);
        std::int64_t media = 0;
        for (int i = 0; i < frames; ++i) {
            media += media_step(rng);
            stats.note_presented(lateness_dist(rng), media);
        }

        const pace::PaceSummary summary =
            stats.summary(16 * kMs, pace::ClockHealth::kFresh);

        if (summary.lateness.count() > 0) {
            CHECK(summary.sampled_samples >= 1);
        } else {
            CHECK(summary.sampled_samples == 0);
        }
    }
}

// ---- ④ 正常片源：门槛不误伤 ------------------------------------------------

void t08_normal_1080p30_run_stays_usable() {
    // 30fps / 8 秒：每帧 +33.33 ms，媒体时间单调推进。
    pace::PaceStats stats;
    std::int64_t media = 0;
    for (int i = 0; i < 240; ++i) {
        stats.note_presented((i % 5) * 1 * kMs, media);
        media += 33'333'333;
    }

    const pace::PaceSummary summary = stats.summary(33 * kMs, pace::ClockHealth::kFresh);
    CHECK(summary.sampled_samples >= 150);
    CHECK(summary.sampled_samples <= 170);
    CHECK(summary.drift.span_ns > 7'000 * kMs);
    CHECK(pace::decimated_block_is_usable(summary.sampled_samples, summary.drift.span_ns));
}

void t09_one_second_clip_is_still_decimated() {
    // 30 帧 / 1 秒：只有 ~20 个抽稀点。**结构门该过**（它是抽稀数据），
    // **统计门该拦**（斜率不可信）。这条守的是"两个门不能混"。
    pace::PaceStats stats;
    std::int64_t media = 0;
    for (int i = 0; i < 30; ++i) {
        stats.note_presented(0, media);
        media += 33'333'333;
    }

    const pace::PaceSummary summary = stats.summary(33 * kMs, pace::ClockHealth::kFresh);
    CHECK(pace::decimated_block_is_usable(summary.sampled_samples, summary.drift.span_ns));
    CHECK(!summary.drift.usable);  // 统计门：样本 < 30 或跨度 < 30 s
}

}  // namespace

int main() {
    std::printf("P13-G decimated-floor tests\n");
    t01_floor_is_two();
    t02_zero_samples_is_unusable();
    t03_single_sample_is_unusable_even_with_span();
    t04_two_samples_without_span_is_unusable();
    t05_two_samples_one_cell_apart_is_usable();
    t06_missing_pts_yields_a_single_sample();
    t07_lateness_implies_at_least_one_decimated_sample();
    t08_normal_1080p30_run_stays_usable();
    t09_one_second_clip_is_still_decimated();
    std::printf("\n%d 项断言 / %d 项失败\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
