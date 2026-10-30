// quantile_tests.cpp — P13-N tests: 分位数必须自己说出精度
//
// 这一组守五句话：
//   ① **桶中心不是量出来的值。** 一份真实分布里 `p50` 恒为 125000，而真中位数是
//      73 000 —— 差 71%。测不到这条，读者就会把两个 p50 当成"两份测量打架"。
//   ② **桶号与值必须同源。** `percentile_ns` 与 `percentile_bucket` 各写一遍累加
//      循环，迟早会在秩边界上分家。n09 拿"秩恰好等于前 k 桶累计"钉这一条。
//   ③ **可能范围必须真的包含那个值。** 桶内时 `[low, high)` 含 `p50`；
//      钳制之后范围变窄但不能变空 —— 空区间在 JSON 里长得像"范围很窄、很准"。
//   ④ **溢出与端点返回的是精确值。** 那时"范围"退化成点本身，这是更准，
//      不是缺数据。把它也当成"量化过"会让人以为还有 0.25 ms 的不确定度。
//   ⑤ **整批的精确参照与桶中心同秩口径。** 两套秩算出来的中位数，
//      放在同一张表里就没法比了 —— 而这张卡存在的全部理由就是让它们能比。
//
// 纯桌面计算：没有设备、没有时钟、没有睡眠。
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "video/video_plane/core/pace/pace_drift_series.hpp"
#include "video/video_plane/core/pace/pace_drift_trend.hpp"
#include "video/video_plane/core/pace/pace_histogram.hpp"

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

/// 桶 512 覆盖 [0, 250 000)，中心 125 000 —— 真机上绝大多数样本落在这里。
constexpr std::int32_t kTheBucket = 512;

// ① 那份真机分布的形状：绝大多数样本远低于桶中心。
//
// 1300 个 73 µs + 700 个 200 µs = 2000 个样本，全在桶 512 里。
// 排序后第 1000 个（p50 的秩）落在 73 µs 那一堆 —— 真中位数就是 73 000，
// 而 `percentile_ns(0.50)` 给的是桶中心 125 000，**偏高 71%**。
// 这正是 P13-J 在真机上量到的那个偏移（`74-p13j-design.md` §四）。
pace::SignedHistogram the_real_machine_shape() {
    pace::SignedHistogram histogram;
    for (int i = 0; i < 1300; ++i) {
        histogram.add(73'000);
    }
    for (int i = 0; i < 700; ++i) {
        histogram.add(200'000);
    }
    return histogram;
}

void n01_the_bucket_center_is_not_the_median() {
    const pace::SignedHistogram histogram = the_real_machine_shape();
    CHECK(histogram.count() == 2000);
    // 桶中心：这是 `percentile_ns` 给的那个数。
    CHECK(histogram.percentile_ns(0.50) == 125'000);

    // 真实的中位数（真值 73 000）根本不在 125 000 附近 —— 不在这个函数里算，
    // 因为直方图本来就答不出真值。这就是本卡要说的话：
    // **125 000 不是量出来的，是桶的几何中心。**
    CHECK(histogram.percentile_ns(0.50) != 73'000);
    CHECK(histogram.min_ns() == 73'000);
    CHECK(histogram.max_ns() == 200'000);
}

// ② 桶号、桶内样本数、范围三者与 `percentile_ns` 出自同一次定位。
void n02_the_bucket_and_the_value_agree() {
    const pace::SignedHistogram histogram = the_real_machine_shape();
    const pace::PercentileBucket at = histogram.percentile_bucket(0.50);

    CHECK(at.located());
    CHECK(at.in_bucket());
    CHECK(at.index == kTheBucket);
    // 整个分布都在这一个桶里 —— 所以"桶内样本数"就是总样本数。
    // 这一格是整个 P13-N 的论据：2000 个样本挤在一个桶里，
    // p50 的位置误差可以接近半桶宽 125 000。
    CHECK(at.samples == 2000);
    CHECK(at.samples == histogram.count());

    // 桶号必须能和值对上：桶 512 的中心 125 000 正是 `percentile_ns` 返回的数。
    CHECK(pace::SignedHistogram::bucket_center_ns(at.index) == histogram.percentile_ns(0.50));
    CHECK(pace::SignedHistogram::bucket_low_ns(at.index) == 0);
    CHECK(pace::SignedHistogram::bucket_high_ns(at.index) == 250'000);
    // 中心 = 下界 + 半桶宽。三者是同一套映射的三种说法。
    CHECK(pace::SignedHistogram::bucket_center_ns(at.index) ==
          pace::SignedHistogram::bucket_low_ns(at.index) +
              pace::SignedHistogram::kBucketWidthNs / 2);
}

// ③ 可能范围必须包含那个值，且被样本范围钳窄。
void n03_the_range_contains_the_value_and_is_clamped_by_the_samples() {
    const pace::SignedHistogram histogram = the_real_machine_shape();
    const pace::SignedHistogram::PercentileRange range = histogram.percentile_range(0.50);
    const std::int64_t p50 = histogram.percentile_ns(0.50);

    CHECK(range.bounded);
    CHECK(range.low_ns <= p50);
    CHECK(p50 < range.high_ns);

    // 桶界是 [0, 250 000)，但样本只到 73 000 ~ 200 000 ——
    // 真值不可能是 0（没有样本），也不可能是 249 999（最大样本是 200 000）。
    // 所以范围被钳成 [73 000, 200 001)。这一条是"钳制"这件事唯一能被看见的地方：
    // 只报桶界 [0, 250 000) 也"包含"了 p50，但那等于说"真值可能是 0"。
    CHECK(range.low_ns == 73'000);
    CHECK(range.high_ns == 200'001);  // 开界 = max + 1
    // 真中位数 73 000 落在范围里 —— 这就是"两个 p50 不打架"的形式化说法。
    CHECK(73'000 >= range.low_ns && 73'000 < range.high_ns);
}

// ④ 溢出区与端点：`percentile_ns` 返回精确极值，范围退化成点。
void n04_overflow_and_endpoints_are_exact_not_quantized() {
    pace::SignedHistogram histogram;
    // 全部高于 kHighNs（+128 ms）→ 落在上溢区。
    for (int i = 0; i < 10; ++i) {
        histogram.add(500 * kMs);
    }

    const pace::PercentileBucket at = histogram.percentile_bucket(0.50);
    CHECK(at.located());
    // 上溢：桶号是 kBucketCount，不是某个真实桶。
    CHECK(at.index == pace::SignedHistogram::kBucketCount);
    CHECK(!at.in_bucket());
    CHECK(at.exact);
    CHECK(at.samples == 10);

    // 值就是精确最大值 —— 不含 0.25 ms 的量化误差。
    const std::int64_t p50 = histogram.percentile_ns(0.50);
    CHECK(p50 == 500 * kMs);

    const pace::SignedHistogram::PercentileRange range = histogram.percentile_range(0.50);
    CHECK(!range.bounded);
    CHECK(range.low_ns == p50);
    CHECK(range.high_ns == p50);

    // 端点同样走精确分支：q = 0 → min，q = 1 → max。
    const pace::PercentileBucket at_zero = histogram.percentile_bucket(0.0);
    const pace::PercentileBucket at_one = histogram.percentile_bucket(1.0);
    CHECK(at_zero.exact && !at_zero.in_bucket());
    CHECK(at_one.exact && !at_one.in_bucket());
    CHECK(histogram.percentile_ns(0.0) == histogram.min_ns());
    CHECK(histogram.percentile_ns(1.0) == histogram.max_ns());
    // 端点范围必须和值相等 —— 否则报告里会写出 "p50 = 500000000，
    // 可能范围 [0, 250000)" 这种自相矛盾的一对。
    CHECK(histogram.percentile_range(0.0).low_ns == histogram.min_ns());
    CHECK(histogram.percentile_range(1.0).high_ns == histogram.max_ns());
}

// ⑤ 空样本：不是"落在桶 0"，是"什么都没量到"。
void n05_empty_histogram_has_no_bucket() {
    const pace::SignedHistogram histogram;
    const pace::PercentileBucket at = histogram.percentile_bucket(0.50);

    CHECK(!at.located());
    CHECK(!at.in_bucket());
    CHECK(at.index == pace::PercentileBucket::kNoSamples);
    CHECK(at.samples == 0);
    // 空样本不许说"精确"：说精确等于说"我量到了一个值"。
    CHECK(!at.exact);

    const pace::SignedHistogram::PercentileRange range = histogram.percentile_range(0.50);
    CHECK(!range.bounded);
    CHECK(range.low_ns == 0);
    CHECK(range.high_ns == 0);
}

// ⑥ 秩边界：秩恰好等于前 k 桶累计时，落在第 k 桶而不是第 k+1 桶。
//
// 这是 `locate()` 里 `rank <= accumulated` 那个 `<=` 的唯一证据。
// 改成 `<` 时，任何"秩正好压在桶边界上"的分布都会把 p50 挪到下一个桶 ——
// 而桶宽 250 µs，挪一格就是一次 66% 的偏差。
void n06_the_rank_on_a_bucket_boundary_stays_in_that_bucket() {
    pace::SignedHistogram histogram;
    // 100 个样本全在桶 0（[-128 ms, -127.75 ms)）。
    // 桶 0 的下界是 -128 000 000，所以 -127'900'000 落在桶 0。
    for (int i = 0; i < 100; ++i) {
        histogram.add(-127'900'000);
    }
    const pace::PercentileBucket at = histogram.percentile_bucket(0.50);
    CHECK(at.index == 0);
    CHECK(at.samples == 100);

    // 现在让秩正好压在一个桶边界上：50 个在桶 0、50 个在桶 1。
    // p50 的秩 = ceil(0.5 × 100) = 50 → 前 1 个桶累计正好 50 → 落在桶 0。
    // 若判据写成 `rank < accumulated`，第 50 名会被推进桶 1。
    pace::SignedHistogram split;
    for (int i = 0; i < 50; ++i) {
        split.add(-127'900'000);  // 桶 0
    }
    for (int i = 0; i < 50; ++i) {
        split.add(-127'600'000);  // 桶 1：[-127.75, -127.5) ms
    }
    const pace::PercentileBucket boundary = split.percentile_bucket(0.50);
    CHECK(boundary.index == 0);
    CHECK(boundary.samples == 50);
    CHECK(pace::SignedHistogram::bucket_center_ns(boundary.index) ==
          split.percentile_ns(0.50));
}

// ⑦ 整批的精确参照：`median_of_window_medians_ns` 与 `p50` 同秩口径。
//
// 8 个窗口中位数，秩取 (n−1)/2 = 3（0 基），即第 4 个。
// 若写成 `n/2`，拿到的是第 5 个 —— 在两个数不同的构造下立刻能分开。
void n07_the_window_median_of_medians_uses_the_same_rank() {
    pace::DriftSeries series;
    // 8 个窗口 × 200 个点，每窗 lateness 各不相同。
    //
    // **故意用递减输入**：第 w 窗的值是 (7−w) ms，于是窗口数组是
    // [7, 6, 5, 4, 3, 2, 1, 0]。排不排序给出的中位数不同 ——
    // 递增输入下"忘了排序"和"排了序"给同一个数，那条变异会活过去
    // （P13-M 的 C6 就是这么漏的：max 与累加在只有一个洞时相等）。
    for (int i = 0; i < 1600; ++i) {
        const std::int64_t media_ns = static_cast<std::int64_t>(i) * 50 * kMs;
        const std::int64_t window = static_cast<std::int64_t>(i) / 200;
        series.note(media_ns, (7 - window) * kMs);
    }
    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.window_count == 8);
    CHECK(trend.window_median_exact);
    // 递减输入确实递减 —— 否则上面那句"故意"是空话。
    CHECK(trend.window_median_ns[0] == 7 * kMs);
    CHECK(trend.window_median_ns[7] == 0);

    // 排序后第 (8−1)/2 = 3 个 = 3 ms。
    // 秩写成 n/2 = 4 → 第 5 个 = 4 ms；不排序直接取 medians[3] = 4 ms。
    CHECK(trend.median_of_window_medians_ns == 3 * kMs);

    // 同一批 8 个值交给直方图，p50 必须落在**同一个元素所在的桶**里 ——
    // 这才是"秩口径一致"能验证的东西。
    //
    // 拿 `percentile_ns(0.50)` 的返回值来比是错的：那个数是桶中心 3.125 ms，
    // 而真值是 3 ms，两者**本来就该不同**（这正是本卡的论点，不是判据的漏洞）。
    // 第一版就是这么写的，第一次运行就红了 —— 红得对，错的是断言不是实现。
    pace::SignedHistogram as_samples;
    for (int w = 0; w < 8; ++w) {
        as_samples.add(static_cast<std::int64_t>(w) * kMs);
    }
    const pace::PercentileBucket at = as_samples.percentile_bucket(0.50);
    CHECK(at.index == (3 * kMs + 128'000'000) / pace::SignedHistogram::kBucketWidthNs);
    // 这一桶里只有 **1** 个样本（8 个值每 250 µs 落一个桶）。
    // 断言的是"桶内计数"而不是"累计计数"：两者在这里是 1 vs 4，
    // 写成 `accumulated` 会立刻红。
    CHECK(at.samples == 1);
    // 真值必须落在那一段里 —— 秩口径一致的形式化说法。
    const pace::SignedHistogram::PercentileRange range = as_samples.percentile_range(0.50);
    CHECK(trend.median_of_window_medians_ns >= range.low_ns);
    CHECK(trend.median_of_window_medians_ns < range.high_ns);
    // 顺便把这个偏移本身钉住：桶中心 3.125 ms 比真值 3 ms 高 125 µs，
    // 正是半个桶宽。P13-J 在真机上量到的是 52 µs（71%），量级一致。
    CHECK(as_samples.percentile_ns(0.50) == 3'125'000);
    CHECK(as_samples.percentile_ns(0.50) - trend.median_of_window_medians_ns ==
          pace::SignedHistogram::kBucketWidthNs / 2);
}

// ⑧ 整批的精确参照必须落在样本范围内 —— 中位数不可能超出极值。
void n08_the_reference_stays_inside_the_samples() {
    pace::DriftSeries series;
    std::int64_t lowest = 0;
    std::int64_t highest = 0;
    for (int i = 0; i < 800; ++i) {
        const std::int64_t media_ns = static_cast<std::int64_t>(i) * 50 * kMs;
        // 锯齿：−400 µs 到 +399 µs，来回摆。
        const std::int64_t lateness = (i % 800) * 1000 - 400'000;
        if (i == 0 || lateness < lowest) {
            lowest = lateness;
        }
        if (i == 0 || lateness > highest) {
            highest = lateness;
        }
        series.note(media_ns, lateness);
    }
    const pace::DriftTrend trend = pace::summarize_drift(series);
    CHECK(trend.window_count == 8);
    CHECK(trend.median_of_window_medians_ns >= lowest);
    CHECK(trend.median_of_window_medians_ns <= highest);
}

// ⑨ 那份真机形状进不了"整批参照"这条路 —— 但整场 p50 照样是桶中心。
//
// 两个数在同一份证据里并存，而它们差 71%。这条用例的作用是把这个差
// **写成断言**：以后谁把桶宽改小、或把 `percentile_ns` 换成精确选值，
// 这里会立刻红，逼他去看 P13-N 说过什么。
void n09_the_gap_between_the_two_p50s_is_real_and_documented() {
    const pace::SignedHistogram histogram = the_real_machine_shape();
    const std::int64_t bucket_p50 = histogram.percentile_ns(0.50);
    const std::int64_t true_median = 73'000;  // 由构造保证

    CHECK(bucket_p50 == 125'000);
    // 偏差 52 000 ns = 71%（以真值为基准）。用绝对值比较，免得写下浮点。
    CHECK(bucket_p50 - true_median == 52'000);
    // 而真中位数确实落在那个桶里 —— 所以它们不是"两份测量打架"，
    // 是同一批点的两种精度。
    CHECK(true_median >= histogram.percentile_range(0.50).low_ns);
    CHECK(true_median < histogram.percentile_range(0.50).high_ns);
}

void n10_payload_stays_small() {
    // 24 B：int32 + 4 B 填充 + int64 + bool + 7 B 填充。
    CHECK(sizeof(pace::PercentileBucket) <= 24);
    // `DriftTrend` 加了一个 int64（P13-N），384 → 392。
    // P13-O 又整体嵌进了一个 `HalvesAgreement`（9 个 double + 2 个 bool），
    // 实测 528 —— 这条断言原先停在 448，从 P13-O 落地那天起就是红的（P13-P 发现）。
    CHECK(sizeof(pace::DriftTrend) <= 528);
}

// ⑪ 秩正好等于下溢计数时，落回 `min_ns` 而不是掉进桶循环。
//
// 这是 `locate()` 里 `rank <= below_` 那个 `<=` 的唯一证据。
// 改成 `<` 时，这一帧会越过所有桶、落到上溢分支，返回 `max_ns` ——
// 一份"全部样本都在 −200 ms"的分布会报出 p50 = 125 000。
// 那是从"极早"翻到"正常"，不是差一点。
void n11_a_rank_equal_to_the_below_count_stays_below() {
    pace::SignedHistogram histogram;
    // 1 个在 kLowNs（−128 ms）以下 → below_ = 1。
    histogram.add(-200 * kMs);
    // 1 个在桶 512（0~250 µs）。
    histogram.add(0);
    CHECK(histogram.below_count() == 1);
    CHECK(histogram.count() == 2);

    // 2 个样本的 p50 秩 = ceil(0.5 × 2) = 1，正好等于 below_ = 1。
    const pace::PercentileBucket at = histogram.percentile_bucket(0.50);
    CHECK(at.index == pace::PercentileBucket::kBelowRange);
    CHECK(at.samples == 1);
    CHECK(at.exact);
    CHECK(!at.in_bucket());

    // 命中下溢分支 ⇒ 返回精确最小值。
    CHECK(histogram.percentile_ns(0.50) == -200 * kMs);
    // 若判据写成 `<`，这里会变成桶 512 的中心 125 000 —— 差了 325 ms。
    CHECK(histogram.percentile_ns(0.50) != 125'000);
}



}  // namespace

int main() {
    n01_the_bucket_center_is_not_the_median();
    n02_the_bucket_and_the_value_agree();
    n03_the_range_contains_the_value_and_is_clamped_by_the_samples();
    n04_overflow_and_endpoints_are_exact_not_quantized();
    n05_empty_histogram_has_no_bucket();
    n06_the_rank_on_a_bucket_boundary_stays_in_that_bucket();
    n07_the_window_median_of_medians_uses_the_same_rank();
    n08_the_reference_stays_inside_the_samples();
    n09_the_gap_between_the_two_p50s_is_real_and_documented();
    n10_payload_stays_small();
    n11_a_rank_equal_to_the_below_count_stays_below();

    std::printf("p13n quantile tests: %d checks, %d failed\n", g_checked, g_failed);
    return g_failed == 0 ? 0 : 1;
}
