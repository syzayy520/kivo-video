#pragma once

#include <array>
#include <cstdint>

namespace kivo::video::aaudio {

/// 残差分档的边界（纳秒），**左闭右开**，最后一档是溢出档。
///
/// 为什么是 1/2/4/8/16/32/64/128/256/512 ms 这种等比的：
/// 我们要回答的问题不是"平均误差多少"，而是"尾部有多长" ——
/// 即"超过 200 ms（守卫的钳制阈值）的那一类，到底离阈值有多远、有多少"。
/// 等比档在尾部的相对分辨率恒定，判"是 250 ms 还是 8 s"不会因为档太粗而看不出来。
inline constexpr std::int32_t kClockTailBucketCount = 11;
inline constexpr std::array<std::int64_t, kClockTailBucketCount - 1> kClockTailEdgesNs{
    1'000'000,    //  1 ms
    2'000'000,    //  2 ms
    4'000'000,    //  4 ms
    8'000'000,    //  8 ms
    16'000'000,   // 16 ms
    32'000'000,   // 32 ms
    64'000'000,   // 64 ms
    128'000'000,  // 128 ms
    256'000'000,  // 256 ms
    512'000'000,  // 512 ms
};

/// 一次运行里"设备候选位置步长"与"墙钟步长"之差的完整事实。
///
/// 定义：`residual = (candidate − last_candidate) − (mono − last_mono)`。
///   · 残差 ≈ 0   → 设备位置和墙钟同步前进，正常。
///   · 残差 < 0   → 设备位置走慢了（陈旧读数、重采样取整），守卫整条丢掉（`monotonic_rejects`）。
///   · 残差 > 200ms → 守卫钳制（`jumps_clamped`），一次放行一小步。
///
/// **为什么要有这个结构**：`monotonic_rejects` / `jumps_clamped` 是事件计数。
/// 一个 83 分钟只有 1 次的计数，说不出"多久一次"（泊松区间 [0.03, 5.6]）。
/// 而残差分布每次观测都产生一个样本 —— 用尾部形状去估"越过 200 ms 的概率"，
/// 比等第二个事件高效一个数量级。这是本卡换口径的全部理由。
///
/// 它不判对错：`beyond_threshold()` 只是"落在阈值之上的样本数"，
/// 是不是"异常"由证据层按注入速率判定。
struct ClockTailFacts {
    std::int64_t samples = 0;   ///< 参与统计的观测数（首次与 rebase 后的首个观测不算）
    std::int64_t forward = 0;   ///< 残差 ≥ 0 的观测数（含恰好 0）
    std::int64_t backward = 0;  ///< 残差 < 0 的观测数

    /// 最大正残差（设备跑在前面）。0 表示没出现过正残差。
    std::int64_t max_forward_ns = 0;
    /// 最大负残差的**幅度**（设备落后多少）。正值。
    std::int64_t max_backward_ns = 0;

    /// `|residual|` 的分档计数，边界见 `kClockTailEdgesNs`。
    std::array<std::int64_t, kClockTailBucketCount> buckets{};

    // ── 观测间隔（P14-G 烟测撞出来的东西）──
    //
    // 守卫判跳变用的是「候选位置距上一轮最大位置」，**它不知道过了多久**。
    // 200 ms 阈值的前提是"查询间隔远小于 200 ms"（一个视频帧 16–42 ms）。
    // 一旦查询循环被调度卡住 287 ms，设备位置正常前进 287 ms 也会被当成跳变 ——
    // 于是出现一次**残差为零的钳制**。烟测第一轮就抓到一次（残差 −9.7 µs、
    // 墙钟步长 287 ms、修正 200 ms 被吞）。
    //
    // 所以间隔必须单独出版：它是"钳制是否冤枉"的唯一判据，
    ///而不看间隔就永远分不清"设备跳了"和"我们没看"。
    std::int64_t intervals = 0;       ///< 采到的间隔数（`mono_step > 0`，含候选倒退的那些）
    std::int64_t long_gaps = 0;       ///< 间隔 ≥ `clamp_threshold_ns` 的次数
    std::int64_t max_mono_step_ns = 0; ///< 最长的一次观测间隔

    // ── 候选步长：**真正预测钳制的那个量**（P14-G 首轮 30 分钟撞出来的）──
    //
    // 守卫的条件是 `candidate − 上一轮最大位置 > 200 ms`，即**候选步长**越阈值。
    // 而 `候选步长 = 观测间隔 + 残差`。所以：
    //   · 只有长间隔、没有残差   → 也会钳制（烟测：间隔 287 ms、残差 −9.7 µs）；
    //   · 只有残差、没有长间隔   → 也会钳制（设备真跳）；
    //   · **两个都不大，加起来越阈值** → 照样钳制（首轮 30 分钟里的第三次：
    //     间隔不到 200、残差 48 ms、和越线）。
    // 第三种让"残差越阈值 OR 间隔越阈值"这条判据漏掉了一次钳制。
    // 于是：直接记候选步长，判据就不会再漏。
    std::int64_t long_candidate_steps = 0;   ///< 候选步长 ≥ `clamp_threshold_ns` 的次数
    std::int64_t max_candidate_step_ns = 0;  ///< 最长的一次候选步长

    /// 第一个间隔的墙钟时刻。事件窗口里的 `mono_ns` 是绝对时刻，
    /// 减去它才能说"这件事发生在会话的第几秒"。
    std::int64_t first_mono_ns = 0;

    /// 判定"钳制"用的阈值。出版它，读者才能自己复算"有没有越过"，
    /// 不必去翻代码里的常量。
    std::int64_t clamp_threshold_ns = 200'000'000;

    /// 落在 `clamp_threshold_ns` 及之上的样本数（`note()` 时逐次判定的真值）。
    ///
    /// **为什么不从 `buckets` 求和**：档边界是 128 / 256 ms，而阈值是 200 ms，
    /// 那一刻**落在同一档里**。按档的下界去求和，会把 200–256 ms 的事件漏掉 ——
    /// 而那正好是钳制类最典型的幅度（实测 225.5 ms）。一个为了好看而少算的
    /// 计数器，比没有更糟。
    [[nodiscard]] std::int64_t beyond_threshold() const noexcept { return beyond; }

    /// 判定时逐次累加的那个数。
    std::int64_t beyond = 0;
};

/// 残差尾部累加器。**非线程安全** —— 只在时钟源那个线程里用。
class ClockTailTracker {
public:
    /// 喂一次观测。第一次（或 `rebase()` 之后第一次）只建立基线，不产生样本。
    ///
    /// 为什么必须重设基线：seek 会把候选位置整体挪几秒到几分钟，
    /// 旧基线配新位置，残差就是一整个 seek 距离 —— 那会把尾部统计
    /// 按"每次 seek 一个大事件"污染成一片假异常。
    void note(std::int64_t candidate_ns, std::int64_t mono_ns) noexcept;

    /// 丢弃基线但**保留计数**：seek / 换片源时用。
    void drop_baseline() noexcept;

    /// 连计数一起清。只在"这是一次新的运行"时用。
    void reset() noexcept;

    [[nodiscard]] ClockTailFacts snapshot() const noexcept;

private:
    bool primed_ = false;
    std::int64_t last_candidate_ns_ = 0;
    std::int64_t last_mono_ns_ = 0;
    ClockTailFacts facts_{};
};

}  // namespace kivo::video::aaudio
