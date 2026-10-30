#pragma once

#include <array>
#include <cstdint>

namespace kivo::video::core::pace {

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
    std::array<std::int64_t, kBucketCount> bins_{};
    std::int64_t below_ = 0;
    std::int64_t above_ = 0;
    std::int64_t count_ = 0;
    std::int64_t min_ = 0;
    std::int64_t max_ = 0;
};

}  // namespace kivo::video::core::pace
