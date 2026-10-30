#pragma once

#include <cstdint>

#include "video/video_plane/core/audio/clock/audio_clock_source.hpp"

namespace kivo::video::core::audio {

/// 给时钟位置注入一次**已知台阶**的装饰器（P13-AB）。
///
/// 为什么需要它：P13-S 到 P13-AA 一共跑了 20 多轮负载实验去撞"时钟跑偏"这个
/// 间歇事件，命中率约 1/3（`105-p13z-status.md` §二、`106-p13aa-status.md` §四）。
/// 靠概率撞出来的证据，永远说不清"是门没用"还是"这次没撞上"——
/// 漂移门从装上到现在一直卡在这一句上。
///
/// 这个装饰器把那件事变成**可以按需复现的**：包住真实时钟，在媒体时间走到
/// `arm_after_ns` 之后，把从此以后的位置都加上 `offset_ns`。之后的所有行为
/// （节拍器怎么反应、门怎么复位、`playback_rate` 变成多少）都还是真的，
/// 只有"这个时钟比真实时间超前 x ms"这一段是给定的。
///
/// 为什么是持续偏移而不是一次性脉冲：真实的时钟跑偏（`elapsed` 估计偏大）
/// 不会下一帧自己走回来。做成脉冲的话，位置回落会被 tracker 的倒退钳制顶住，
/// 观测到的形状就变成"一串钳制"而不是"一次台阶后怎么收敛"。
///
/// **只在探针路径上使用**：`kivo.clock_offset_ms` 非 0 才会被创建
/// （`jni_session_handle.cxx`）。不带参数时它根本不存在，生产路径上没有任何分支
/// 需要为它让路 —— 这是它与"往 tracker 里塞一个 debug 开关"的关键差别：
/// 注入点在 `AudioClockSource` 这层接缝上，而这一层本来就是为注入而设计的。
///
/// 证据里必须留下 `clock_injection` 三格（`sync/18`），否则一份被注入过的证据
/// 和一份真实现场长得一模一样。
class ClockOffsetSource final : public AudioClockSource {
public:
    /// @param inner        被包住的真实时钟（生命周期须长于本对象）
    /// @param offset_ns    要注入的台阶（正 = 时钟位置超前，即"表快了"）
    /// @param arm_after_ns 从**首次读到的位置**算起，走够这么多媒体时间才注入
    /// @param rate_ppm     **速率**误差（P13-AD）：正 = 钟比墙**快**（位置超前），
    ///                     注入之后位置按 `1 + ppm/1e6` 的倍率推进。
    ///                     这是真实事件的形状（P13-R/U/V/Z 的 8 次全是渐变），
    ///                     与台阶（瞬时）是两种不同的输入。
    ///                     符号与 `offset_ns`、`clock_ahead`、`playback_rate` 一致：
    ///                     正 = 快，谁都不用再记第二套约定。
    ClockOffsetSource(AudioClockSource& inner, std::int64_t offset_ns,
                      std::int64_t arm_after_ns, std::int32_t rate_ppm = 0) noexcept;

    [[nodiscard]] AudioClockSnapshot now() override;

    /// seek：转发给内层，并把"起点"作废 —— 重设基准之后原来的
    /// `arm_after` 参照系已经没有意义了。
    void rebase(std::int64_t media_position_ns) noexcept override;

    [[nodiscard]] bool applied() const noexcept { return applied_; }
    [[nodiscard]] std::int64_t offset_ns() const noexcept { return offset_ns_; }
    [[nodiscard]] std::int64_t arm_after_ns() const noexcept { return arm_after_ns_; }
    [[nodiscard]] std::int32_t rate_ppm() const noexcept { return rate_ppm_; }
    /// 注入发生时的位置（-1 = 还没发生）。出版它是为了让"注入过"这件事带时间戳，
    /// 而不是只有一个布尔。
    [[nodiscard]] std::int64_t armed_at_position_ns() const noexcept { return armed_at_position_ns_; }

private:
    AudioClockSource& inner_;
    const std::int64_t offset_ns_;
    const std::int64_t arm_after_ns_;
    /// 速率的百万分比。正 = 钟比墙快（P13-AD）。
    const std::int32_t rate_ppm_;
    bool has_origin_ = false;
    std::int64_t origin_position_ns_ = 0;
    std::int64_t armed_at_position_ns_ = -1;
    bool applied_ = false;
    /// 注入起点：速率倍率从这一帧的位置开始算，之前的样本不受影响。
    std::int64_t rate_origin_position_ns_ = 0;
};

}  // namespace kivo::video::core::audio
