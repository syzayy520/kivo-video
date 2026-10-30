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
