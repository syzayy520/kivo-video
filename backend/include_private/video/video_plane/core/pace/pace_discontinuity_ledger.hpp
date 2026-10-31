#pragma once

#include <cstdint>

#include "video/video_plane/core/pace/pace_discontinuity.hpp"

namespace kivo::video::core::pace {

/// 钟的不连续那本账（P14-Y）。
///
/// **它只做记账，不做判定**：分类由调用方（`ClockPositionTracker`）按分支决定，
/// 阈值留在 `PacePolicy` 与 tracker 的常量里。这样单测可以直接喂一笔"假事件"，
/// 不必构造时钟 —— 而"账对不对"与"分类对不对"也能分开验。
///
/// 一句话职责：`note()` 收一笔，`offset_ns()` 回答"这些台阶一共吃掉多少领先量"。
class DiscontinuityLedger {
public:
    /// 记一笔。`step_ns` 是"位置实际推进 − 墙钟推进"，累计进 `offset_ns`。
    ///
    /// **一帧最多一笔**由调用方保证：同一帧既被拒又被钳时只记优先级最高的那一个，
    /// 否则同一段墙时会被算两次（账直接翻倍）。
    void note(DiscontinuityKind kind, std::int64_t at_ns, std::int64_t wall_delta_ns,
              std::int64_t raw_delta_ns, std::int64_t step_ns, std::int64_t lead_before_ns,
              std::int64_t lead_after_ns) noexcept;

    /// 累计偏移（与领先量同号）。门每帧都要读它，所以是 O(1) 的一个整数。
    [[nodiscard]] std::int64_t offset_ns() const noexcept { return offset_ns_; }

    /// 出版用快照：分类计数 + 累计 + 环（升序展开）。
    [[nodiscard]] DiscontinuityTrace trace() const noexcept;

    /// seek / 换片源：整本账清零（与 `RebaseTrace` 同寿命，只被 `reset()` 清）。
    void reset() noexcept;

private:
    std::int64_t events_ = 0;
    std::int64_t extrapolated_ = 0;
    std::int64_t rejected_ = 0;
    std::int64_t clamped_ = 0;
    std::int64_t jumped_ = 0;
    std::int64_t stepped_ = 0;
    std::int64_t offset_ns_ = 0;
    std::int64_t sampled_offset_ns_ = 0;
    std::int64_t first_at_ns_ = 0;
    DiscontinuitySample max_{};
    DiscontinuitySample recent_[8]{};
    std::int32_t head_ = 0;   ///< 下一笔写到哪
    std::int32_t count_ = 0;  ///< 环里已有几笔（≤ 8）
};

}  // namespace kivo::video::core::pace
