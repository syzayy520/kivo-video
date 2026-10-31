#pragma once

#include <array>
#include <cstdint>

namespace kivo::video::core::pace {

/// gap 分档边界（纳秒），**左闭右开**，最后一档溢出。
///
/// 8/16/33/66/133/266/532/1000 ms 是按"帧时长"选的点：
/// 一个 30 fps 帧是 33 ms，所以 33 那一档是"正好一帧"，
/// 66 是"丢一帧"，133 是"丢三帧"，266 以上才是用户能看见的卡。
inline constexpr std::int32_t kPresentGapBucketCount = 9;
inline constexpr std::array<std::int64_t, kPresentGapBucketCount - 1> kPresentGapEdgesNs{
    8'000'000,      //    8 ms
    16'000'000,     //   16 ms
    33'000'000,     //   33 ms ≈ 一帧
    66'000'000,     //   66 ms ≈ 两帧
    133'000'000,    //  133 ms
    266'000'000,    //  266 ms
    532'000'000,    //  532 ms
    1'000'000'000,  // 1000 ms
};

/// 一次"最长停顿"的构成。
///
/// 设备走的是**合并管线**：送包与收帧在同一个线程里轮流跑（`run_merged()`），
/// 一圈是 `submit(批量) → pump(等帧 + 节拍 + 交帧)`。所以：
///
/// ```
/// gap        = submit + pump + unaccounted
/// pump  ⊇      pace + release（+ 等帧）
/// ```
///
/// `unaccounted` 是**圈与圈之间**的时间：循环开销、被抢占、别的线程挤占。
/// 它不是"误差"，是本卡要找的那个量。
///
/// **为什么只看最差的那一次**：平均值会把 234 ms 抹成 32 ms，而 32 ms 正好是
/// 正常节拍等待 —— 一份"看起来完全健康"的证据。而用户能看见的只有最差那一次。
///
/// **为什么三段必须是同一次迭代的**：P14-I 设计卡 §一 试过拿三段各自的峰值相加，
/// 得到 42 ms 去解释 253 ms 的停顿 —— 峰值可以发生在三个不同时刻，那个加法不成立。
struct PresentGapFacts {
    std::int64_t iterations = 0;      ///< 圈数（= 帧数，含 EOS 那一圈）
    std::int64_t long_gaps = 0;       ///< gap ≥ 100 ms 的圈数
    std::int64_t max_gap_ns = 0;      ///< 最长的一圈

    /// **最长那一圈**的分段耗时。没有样本时全为 0。
    ///
    /// `worst_pump_ns ⊇ worst_pace_ns + worst_release_ns`（嵌套，不能相加两次）。
    std::int64_t worst_submit_ns = 0;
    std::int64_t worst_pump_ns = 0;
    std::int64_t worst_pace_ns = 0;     ///< pump 内：`arrive()`（时钟查询 + 按节拍睡）
    std::int64_t worst_release_ns = 0;  ///< pump 内：平台交帧
    /// `max_gap_ns − submit − pump`，钳到 ≥ 0。**"看不见的时间"**。
    std::int64_t worst_unaccounted_ns = 0;

    std::array<std::int64_t, kPresentGapBucketCount> buckets{};
};

/// 接收线程的停顿累加器。只在那个线程里写。
class PresentGapTracker {
public:
    /// 记一圈。
    ///
    /// `gap_ns` 是相邻两圈起点之差；三个分段耗时是**这一圈**的实测值。
    /// 首次调用（还没有上一圈）只建基线，不产生样本。
    void note(std::int64_t gap_ns,
              std::int64_t submit_ns,
              std::int64_t pump_ns,
              std::int64_t pace_ns,
              std::int64_t release_ns) noexcept;

    void reset() noexcept;

    [[nodiscard]] PresentGapFacts snapshot() const noexcept;

private:
    PresentGapFacts facts_{};
};

}  // namespace kivo::video::core::pace
