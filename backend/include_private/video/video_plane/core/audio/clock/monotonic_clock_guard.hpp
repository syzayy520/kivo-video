#pragma once

#include <cstdint>

namespace kivo::video::core::audio {

/// 单调时钟守卫：把任意候选位置变成一个**永不倒退**的输出。
///
/// 为什么需要它：`getTimestamp` 的帧位置在正常运行时理论上单调，
/// 但设备侧重采样、配置切换、API 实现差异都可能让它回退一次。
/// 一次回退就够让同步器把一帧判成"音频已经走过"，随后触发一次无谓追帧 ——
/// 而这种抖动在统计上表现为 p99 冒尖，极难归因。
///
/// 纯逻辑、零平台依赖，因此"倒退/跳变"两类异常可以在桌面机器上断言到底。
class MonotonicClockGuard {
public:
    /// 默认 200 ms 跳变阈值：**超出"这段时间本可以前进的量"多少才算跳变**。
    ///
    /// 它不是一个绝对的位置阈值 —— 判据是
    /// `advance > kDefaultJumpThresholdNs + elapsed`，`elapsed` 是距上一次观测的墙钟增量。
    /// 这一点是 P14-G 用真机 121 分钟证出来的：片尾等 EOS 的一次 383 ms 间隔里，
    /// 位置也正常前进了 383 ms（残差 ≈ 0），旧逻辑却把它当成跳变钳掉。
    /// 阈值只能和"过了多久"一起用，单独用就是把量纲搞错了。
    static constexpr std::int64_t kDefaultJumpThresholdNs = 200'000'000;

    /// `elapsed` 的上限（1 小时）。只为防 `threshold + elapsed` 溢出 ——
    /// 超过一小时的间隔已经不属于"播放中"，任何取值都不影响结论。
    static constexpr std::int64_t kMaxElapsedNs = 3'600'000'000'000;

    /// 一次判定的结果。
    struct Decision {
        std::int64_t position_ns = 0;
        bool rejected_backward = false;  ///< 候选值倒退，已丢弃
        bool clamped_jump = false;       ///< 候选值跳变过大，已钳制
    };

    explicit MonotonicClockGuard(
        std::int64_t jump_threshold_ns = kDefaultJumpThresholdNs) noexcept;

    /// 喂入一个候选位置，得到单调化后的位置。
    ///
    /// `elapsed_ns`：距**上一次 observe** 的墙钟增量（不知道就传 0）。
    /// 传 0 是最保守的一侧：按"没过去时间"判，跳变阈值全额生效。
    ///
    /// 为什么这个量必须由调用方给：守卫是纯逻辑、不碰时钟，这是它能被桌面断言到底的前提。
    /// 而"过了多久"只有持有墙钟的那一层知道。
    [[nodiscard]] Decision observe(std::int64_t candidate_ns,
                                   std::int64_t elapsed_ns = 0) noexcept;

    /// 重置到指定位置（换片源、seek 后重建时钟时调用）。
    /// 计数器不清零 —— 统计属于"本次运行"，重建不该把历史异常抹掉。
    void reset(std::int64_t position_ns = 0) noexcept;

    [[nodiscard]] std::int64_t max_position_ns() const noexcept { return max_ns_; }
    [[nodiscard]] std::int64_t backward_rejects() const noexcept { return backward_rejects_; }
    [[nodiscard]] std::int64_t jump_clamps() const noexcept { return jump_clamps_; }

private:
    std::int64_t jump_threshold_ns_;
    std::int64_t max_ns_ = 0;
    bool primed_ = false;  ///< 未经首次观测时不能用 0 当基准（首帧可能就是 0）

    std::int64_t backward_rejects_ = 0;
    std::int64_t jump_clamps_ = 0;
};

}  // namespace kivo::video::core::audio
