#pragma once

#include <cstdint>

namespace kivo::video::core::pace {

/// 这一帧该被怎么处理。
///
/// 只有四种，且互斥。动作是**结论**，不是过程 —— 过程在 `FramePacer::arrive()` 里。
enum class PaceAction : std::int32_t {
    kWait = 0,     ///< 早于到期时刻，睡到 `due_at` 再呈现
    kPresent = 1,  ///< 已在窗口内，立即呈现
    kDrop = 2,     ///< 落后超过阈值，丢弃该帧（不上屏），立即取下一帧
    kCatchUp = 3,  ///< 连续落后过多，进入追帧：丢弃且不等待，直到追上
};

[[nodiscard]] const char* pace_action_name(PaceAction action) noexcept;

/// 时钟可信度。三级，只降不升（同一次会话内一旦 free_run 就不再回 fresh 的假设失效，
/// 但会随下一次读取恢复 —— 这里记录的是**当前这一帧**的判断，不是会话终态）。
enum class ClockHealth : std::int32_t {
    kFresh = 0,         ///< 设备直接给的位置，未经修补
    kExtrapolated = 1,  ///< 本轮 `stale`，用墙钟增量补了一小段（< 500ms）
    kFreeRun = 2,       ///< 陈旧过久，不再信它，改用自生成时间轴
};

[[nodiscard]] const char* clock_health_name(ClockHealth health) noexcept;

/// 一次节拍判定的完整结果。纯数据，不含行为。
///
/// `reason` 只在 `kDrop` / `kCatchUp` 时非空 —— 丢帧没有原因就无法归因，
/// 而"丢了多少帧、为什么丢"正是 G6 要回答的问题。
struct PaceDecision {
    PaceAction action = PaceAction::kPresent;

    /// 绝对值：单调时间轴上的到期时刻（ns）。
    /// 只有 `kFreeRun` 时它来自自生成时间轴，其余来自 `clock`。
    std::int64_t due_at_ns = 0;

    /// 判定瞬间的真实迟到量：`now - due_at`。负表示早到，正表示迟到。
    std::int64_t lateness_ns = 0;

    /// `kWait` 时实际睡的时长；其余动作恒为 0。
    std::int64_t wait_ns = 0;

    /// 本帧使用的帧时长估计（由观察到的 pts 差值得出，不是容器声明）。
    std::int64_t frame_duration_ns = 0;

    ClockHealth health = ClockHealth::kFresh;

    /// 空串表示"无特殊原因"。直接用静态字面量，避免热路径上碰 `std::string`。
    const char* reason = "";
};

}  // namespace kivo::video::core::pace
