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
    /// 默认 200 ms 跳变阈值：
    /// 远大于任何合理查询间隔（一个视频帧 16–42 ms），
    /// 又远小于任何真实 seek 跳变（秒级）。P12-F 做 seek 时由上层重建时钟，不走这条路径。
    static constexpr std::int64_t kDefaultJumpThresholdNs = 200'000'000;

    /// 一次判定的结果。
    struct Decision {
        std::int64_t position_ns = 0;
        bool rejected_backward = false;  ///< 候选值倒退，已丢弃
        bool clamped_jump = false;       ///< 候选值跳变过大，已钳制
    };

    explicit MonotonicClockGuard(
        std::int64_t jump_threshold_ns = kDefaultJumpThresholdNs) noexcept;

    /// 喂入一个候选位置，得到单调化后的位置。
    [[nodiscard]] Decision observe(std::int64_t candidate_ns) noexcept;

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
