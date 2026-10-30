#pragma once

#include <cstdint>

#include "video/video_plane/core/pace/pace_action.hpp"

namespace kivo::video::core::pace {

/// 节拍阈值。默认值**照抄** `TASKS.md` P12-C5 的卡片，不在这里"顺手调优"。
///
/// 调参必须另开卡并给出对照数据 —— 否则同一份代码的两个版本会产生
/// 两套不可比的 G6 数字，而 G6 是"丢帧率 < 0.5%"这种几乎零余量的指标，
/// 阈值一动，历史证据立刻失效。
struct PacePolicy {
    /// 早于到期多少以内不再睡。2ms 来自卡片（`|now - due| <= 2ms` 直接呈现）。
    std::int64_t early_margin_ns = 2'000'000;

    /// 迟到多少判定为"落后"。100ms 来自卡片。
    std::int64_t late_drop_ns = 100'000'000;

    /// 连续落后多少帧触发追帧。10 帧来自卡片。
    std::int32_t catch_up_run = 10;

    /// 是否允许为了追上时钟而丢帧。
    ///
    /// 关掉它（`false`）时永远不会丢，迟到的帧照样呈现 —— 用于做"只睡不丢"的对照组，
    /// 也用于"宁可慢也不能少一帧"的场景。默认开，因为 G6 的判据是丢帧率，
    /// 默认关掉等于把判据偷偷改成"永不丢帧"。
    bool allow_drop = true;

    // ---- 以下三项卡片没写，是实现必须补的护栏（见 17-p12c5-design.md §3.1）----

    /// 时钟位置增量与墙钟增量之差超过此值即判为跳变并重锚。
    /// 取 1s：正常抖动在毫秒级，换采样率 / seek / 设备重建在百毫秒到秒级。
    std::int64_t jump_limit_ns = 1'000'000'000;

    /// **累积**漂移上限（P13-S）：位置增量与墙钟增量之差累加到这么多就按墙钟推进。
    ///
    /// `jump_limit_ns` 只看**单次**跳变（1 s），于是"每次只快 1.5%"这种**速率**误差
    /// 永远不会触发它：16.7 ms 的帧距里多走 250 µs，跑一万帧也只是每帧 250 µs。
    /// 真机满载实测：8 秒累积 118.79 ms（1.49%），`jump_rebases` 全程 0 —— 门根本没响。
    ///
    /// 取 45 ms 而不是别的数：它就是 `kAudibleThresholdNs`（人耳能听出的音画偏差），
    /// 与 P13-L 的额度判读同一把尺子。超过它还没纠，播十分钟就是 9 秒。
    std::int64_t drift_limit_ns = 45'000'000;

    /// 复位目标比门限再低这么多（P13-AC）。
    ///
    /// 为什么需要迟滞带：P13-AB 的确定性实验里，`drift_rebases` 是 5/6/10，
    /// 而总修正量只有 35.2 ms —— 第一个复位吃掉了绝大部分，其余全是
    /// "残差刚好等于门限、下一帧钟再往前挤一点又超了"的微调。
    /// 门限同时当"触发阈值"和"收敛目标"，稳态就只能贴着阈值走。
    ///
    /// 复位到 `limit − band` 之后，要再攒 band 才重新触发，微调自然消失。
    /// 取 5 ms：一个 60 fps 帧的三分之一不到，肉眼无感，又足以挡住抖动。
    std::int64_t drift_band_ns = 5'000'000;

    /// 漂移门总开关（P13-AD）。
    ///
    /// 关掉它 = 位置完全按时钟走，门一次都不触发。**只用于对照组**：没有对照组，
    /// 一份"门没触发"的证据就分不清"门在正确的时机保持沉默"和"门根本没接上"。
    /// 它会出版到证据里（`drift_gate.enabled`），关着的一轮不会被误读成正常轮。
    bool drift_gate_enabled = true;


    /// `stale` 持续超过此值就放弃该时钟，改用自生成时间轴。
    /// 取 500ms：音频 burst 4ms、缓冲 32ms（C1 实测），500ms 已经是 15 个缓冲深度。
    std::int64_t stale_extrapolate_limit_ns = 500'000'000;

    /// 单次睡眠上限。睡眠期间无法响应停止请求，所以切块。
    /// 取 250ms：远大于所有正常帧间隔（≤ 200ms），又短到停止不会卡顿可见。
    std::int64_t max_single_wait_ns = 250'000'000;

    /// 帧时长估计的合法区间。低于 1ms（1000fps）或高于 200ms（5fps）的 pts 差值
    /// 一律不采纳 —— 那是坏包或时间基切换，不是帧率。
    std::int64_t min_frame_duration_ns = 1'000'000;
    std::int64_t max_frame_duration_ns = 200'000'000;

    /// 首帧还没有任何 pts 差值可用时的兜底（25fps）。
    std::int64_t default_frame_duration_ns = 40'000'000;

    /// 阈值自身是否自洽。不自洽（比如 early_margin > late_drop）时构造即拒绝，
    /// 免得运行起来每帧都在做"既早又晚"的判定。
    [[nodiscard]] bool sane() const noexcept;
};

/// 判定输入。全部是事实，不含判断。
struct PaceInput {
    /// `now - due_at`
    std::int64_t lateness_ns = 0;

    /// 连续被判"落后"的帧数（`kDrop` 累加，非落后清零）。
    std::int32_t consecutive_late = 0;

    /// 当前是否已在追帧模式
    bool catching_up = false;
};

/// 判定输出：动作 + 状态迁移意图。
///
/// 把"要不要进/出追帧"单独给出来，而不是让判定函数改状态：
/// 这样 `decide()` 是**纯函数**，可以直接对着表格做断言，不必构造会话。
struct PaceVerdict {
    PaceAction action = PaceAction::kPresent;
    bool enter_catch_up = false;
    bool leave_catch_up = false;
};

/// 纯函数：`(迟到量, 连迟帧数, 是否追帧中) → 动作 + 迁移`。
///
/// 判定顺序是**先早后晚**：
/// 早到（`lateness < -early_margin`）优先，因为冻结的时钟会让 lateness 瞬间变得很负，
/// 此时若先判"晚"，就会因为一个错误的基准丢帧。
[[nodiscard]] PaceVerdict decide(const PaceInput& input, const PacePolicy& policy) noexcept;

}  // namespace kivo::video::core::pace
