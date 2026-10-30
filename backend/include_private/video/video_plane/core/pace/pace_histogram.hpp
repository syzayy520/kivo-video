#pragma once

#include <array>
#include <cstdint>

namespace kivo::video::core::pace {

/// 第 q 分位数**落在哪个桶**，以及那个桶里有多少样本（P13-N）。
///
/// 存在的理由：`percentile_ns()` 返回桶中心，而桶中心**不是量出来的值**。
/// 真机上 2013/2048 个样本挤在 0~250 µs 这一桶里，于是 `p50` 恒为 125000 ——
/// 同一份证据里窗口精确中位数却是 74 835，两个数差 66%，读者只会认定
/// "两份测量打架"。它们不打架：真值确实在那块钱里，只是桶中心没本事说出来。
///
/// 光把 `p50` 改成"124999"没有用，那不是更准，是假装更准。要治的是
/// **读者不知道这个数是量出来的还是算出来的** —— 所以出版桶号与桶内样本数，
/// 让"p50 = 125000"旁边永远站着一句"它可能的范围是 [0, 250000)，那里面 2013 个样本"。
struct PercentileBucket {
    /// 桶号 ∈ [0, `kBucketCount`)。**−1** = 落在下溢区（< `kLowNs`），
    /// **`kBucketCount`** = 落在上溢区（≥ `kHighNs`），**−2** = 根本没有样本。
    ///
    /// 不用单个 −1 兼任"下溢"与"空"：那两种情况下 `percentile_ns` 返回的东西
    /// 完全不同（`min_` vs `0`），一个数代表两件事，读者没法判断该信哪个。
    /// 桶号 ∈ [0, `kBucketCount`)。**−1** = 落在下溢区（< `kLowNs`），
    /// **`kBucketCount`** = 落在上溢区（≥ `kHighNs`），**−2** = 根本没有样本。
    ///
    /// 不用单个 −1 兼任"下溢"与"空"：那两种情况下 `percentile_ns` 返回的东西
    /// 完全不同（`min_` vs `0`），一个数代表两件事，读者没法判断该信哪个。
    static constexpr std::int32_t kNoSamples = -2;
    static constexpr std::int32_t kBelowRange = -1;

    std::int32_t index = kNoSamples;
    /// 该桶里的样本数（溢出桶给溢出计数）。空样本时为 0。
    std::int64_t samples = 0;
    /// `true` ⇒ `percentile_ns(q)` 返回的是**精确极值**（`min_` / `max_`），
    /// 不是桶中心；此时"可能的范围"就是这个点本身，没有量化误差。
    bool exact = false;

    /// 有桶号（落在桶里或溢出区，只是不是"没有样本"）。
    [[nodiscard]] bool located() const noexcept { return index != kNoSamples; }
    /// 落在**桶里**（不是溢出区）。只有这样才谈得上量化的上下界。
    ///
    /// 靠 `exact` 区分而不是拿 `index` 去和桶数比：溢出时
    /// `percentile_ns` 返回精确极值，桶内时返回桶中心 —— 那个分支本来就在这里
    /// 已经判过一次，再判第二次就是两个真相的起点。
    [[nodiscard]] bool in_bucket() const noexcept { return located() && !exact; }
};

/// 有符号定宽直方图：热路径零分配、零分支外溢。
///
/// 为什么不用 `std::vector` 存全样本：每帧一个样本，30fps × 1 小时 = 108,000 个，
/// 存得下但不该存 —— 采样器一变长，内存就跟着涨，而 p99 只需要分布。
/// 桶宽 0.25ms 对"1 帧时长（16.7ms @60fps）"这个判据足够。
class SignedHistogram {
public:
    static constexpr std::int64_t kBucketWidthNs = 250'000;  // 0.25 ms
    static constexpr std::int32_t kBucketCount = 1024;
    static constexpr std::int64_t kLowNs = -128'000'000;     // -128 ms
    static constexpr std::int64_t kHighNs = kLowNs + kBucketWidthNs * kBucketCount;  // +128 ms

    void add(std::int64_t value_ns) noexcept;
    void clear() noexcept;

    [[nodiscard]] std::int64_t count() const noexcept { return count_; }
    [[nodiscard]] std::int64_t below_count() const noexcept { return below_; }
    [[nodiscard]] std::int64_t above_count() const noexcept { return above_; }

    /// 精确极值（与桶无关，溢出也准）。
    [[nodiscard]] std::int64_t min_ns() const noexcept { return count_ > 0 ? min_ : 0; }
    [[nodiscard]] std::int64_t max_ns() const noexcept { return count_ > 0 ? max_ : 0; }

    /// 分位数。q ∈ (0,1]。空样本返回 0。
    ///
    /// 落在溢出桶时返回精确的极值而不是桶中心 —— 宁可报"至少 200ms"，
    /// 也不要编一个 ±128ms 的假数。
    [[nodiscard]] std::int64_t percentile_ns(double q) const noexcept;

    /// 第 q 分位数**落在哪个桶**、那个桶里有多少样本（P13-N）。
    ///
    /// 与 `percentile_ns(q)` **共用同一次定位**（内部都走 `locate()`）。
    /// 这一点是这张卡的结构性要求，不是实现细节：另写一个"找中位数桶"的
    /// 循环，就会在边界上（`rank <= below_`、桶中间正好跨过秩）与
    /// `percentile_ns` 分家，然后证据里出现"p50 = 125000，落在桶 511"这种
    /// 自相矛盾的一对 —— P13-M 的 `segments` 正是这么坏的。
    [[nodiscard]] PercentileBucket percentile_bucket(double q) const noexcept;

    /// 第 q 分位数的**可能范围** `[low, high)`（P13-N）。
    ///
    /// 与 `percentile_ns(q)` 同源（都走 `locate()`），但回答的是另一个问题：
    /// 那个数**可能是多少**。桶内时范围就是那个桶与 `[min, max]` 的交集
    /// （`percentile_ns` 会把桶中心钳进样本范围，钳过之后可能范围就窄了）；
    /// 溢出或端点时 `percentile_ns` 返回的是精确极值，范围退化成 `low == high`。
    ///
    /// 为什么放在 core 而不是报告层自己算：报告层（`av_sync_report.cxx`）
    /// **只在 Android 侧编译**，桌面构建看不到它 —— 写在那里的逻辑没有单测，
    /// 只能靠真机证据 + Python 复算。而"钳制之后范围变窄"这件事恰好有两处
    /// 容易写反（`max` 该配 `low` 还是 `high`），必须能在桌面上钉住。
    struct PercentileRange {
        std::int64_t low_ns = 0;
        /// **开界**：真值严格小于它。`low == high` 表示这是个精确值。
        std::int64_t high_ns = 0;
        /// `true` ⇒ 范围宽于一个点，`low < high`。报告层不出版这一格，
        /// 它由 `low == high` 已经能读出来 —— 存它只是为了让单测读得顺。
        bool bounded = false;
    };
    [[nodiscard]] PercentileRange percentile_range(double q) const noexcept;

    /// 桶 `index` 覆盖的区间下界（闭）。
    ///
    /// 与 `bucket_center_ns()` 是同一套映射的三种说法，都写在一个地方：
    /// 中心给 `percentile_ns`，上下界给"这个数可能的范围"（P13-N）。
    [[nodiscard]] static std::int64_t bucket_low_ns(std::int32_t index) noexcept {
        return kLowNs + static_cast<std::int64_t>(index) * kBucketWidthNs;
    }
    /// 桶 `index` 覆盖的区间上界（**开**：真值严格小于它）。
    [[nodiscard]] static std::int64_t bucket_high_ns(std::int32_t index) noexcept {
        return bucket_low_ns(index) + kBucketWidthNs;
    }

    /// `|value| > threshold_ns` 的样本数。
    ///
    /// 用来回答"有多少帧的抖动超过一帧时长"（G6 的判据），
    /// 这个数**不能**从分位数反推，必须单独数。
    [[nodiscard]] std::int64_t count_abs_beyond(std::int64_t threshold_ns) const noexcept;

    /// 桶 i 的代表值（桶中心）。
    ///
    /// 证据导出（`kivo.p12c.sync/1`）与独立复算脚本**必须**用这一套映射，
    /// 否则同一份 JSON 会被算出两个 p99 —— 而"可复算"的全部意义就是
    /// 换一个人、换一份代码，算出来的数一字不差。`percentile_ns()` 内部也是这个式子。
    [[nodiscard]] static std::int64_t bucket_center_ns(std::int32_t index) noexcept {
        return kLowNs + static_cast<std::int64_t>(index) * kBucketWidthNs + kBucketWidthNs / 2;
    }

    /// 各桶计数，下标即桶序号。
    ///
    /// 只读暴露是刻意的：C8 要求"p99 能被第三方从证据重算"，
    /// 光有汇总量无法自证。导出"分布 + 溢出计数 + 精确极值"三样，
    /// 复算方就能完整重放 `percentile_ns()`，而不需要相信任何手填数字。
    [[nodiscard]] const std::array<std::int64_t, kBucketCount>& bins() const noexcept {
        return bins_;
    }

private:
    /// 第 q 分位数的秩落在哪一格。**`percentile_ns` 与 `percentile_bucket`
    /// 唯一的定位来源** —— 两个函数各写一遍累加循环，迟早在边界上分家。
    struct Located {
        /// 桶号；`PercentileBucket::kBelowRange` = 秩落在下溢区，
        /// `kBucketCount` = 落在上溢区。
        std::int32_t bucket = PercentileBucket::kBelowRange;
        /// 该桶（或溢出区）里的样本数。
        std::int64_t samples = 0;
    };
    [[nodiscard]] Located locate(double q) const noexcept;

    std::array<std::int64_t, kBucketCount> bins_{};
    std::int64_t below_ = 0;
    std::int64_t above_ = 0;
    std::int64_t count_ = 0;
    std::int64_t min_ = 0;
    std::int64_t max_ = 0;
};

}  // namespace kivo::video::core::pace
