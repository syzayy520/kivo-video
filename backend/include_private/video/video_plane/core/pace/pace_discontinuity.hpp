#pragma once

#include <cstdint>

namespace kivo::video::core::pace {

/// 钟的一次"不连续"（P14-Y）。
///
/// **与漂移的分界**：漂移是"每帧都多一点、攒起来"（1.5% 的快钟每帧只有 250 µs）；
/// 不连续是"某一刻位置跳了一段"（钟停了一下又补回来，或一次重锚）。
/// 前者该由门还掉，后者还不得 —— P14-X 那笔 146.75 ms 就是被当漂移还掉的台阶。
///
/// 五种事件的差别只在**为什么**，记账规则是同一条：
/// `step = 位置实际推进 − 墙钟推进`。跳变与外推两条分支本来按墙推进，`step` 自然是 0。
enum class DiscontinuityKind : std::int32_t {
    kNone = 0,
    /// 停更一个 burst，位置按墙补。**只记现场**：位置本来就跟着墙走，不产生台阶。
    kExtrapolated = 1,
    /// 源拒了回退的候选值：位置停住、墙照走。这一段的 `step` 是负的。
    kMonotonicRejected = 2,
    /// 修补后的位置倒退，被第二道守卫钳住。
    kBackwardClamped = 3,
    /// 一次跳超过 `jump_limit_ns`：该分支已按墙推进并重锚，`step` 恒 0。
    kJumpRebased = 4,
    /// 正常分支里的台阶：单帧与墙的偏离超过 `kStepDriftNs`（30 ms）。
    /// **这一类没有自己的标志位** —— 它是"停更之后补回来"的那一帧，
    /// 只有比对位置增量与墙增量才看得见（P14-X 缺的 113 ms 就藏在这里）。
    kPositionStep = 5,
};

/// 事件名进证据。写在这里而不是另开一个 `.cxx`：四个字面量不值得一个编译单元。
inline const char* discontinuity_kind_name(DiscontinuityKind kind) noexcept {
    switch (kind) {
        case DiscontinuityKind::kExtrapolated:
            return "extrapolated";
        case DiscontinuityKind::kMonotonicRejected:
            return "monotonic_rejected";
        case DiscontinuityKind::kBackwardClamped:
            return "backward_clamped";
        case DiscontinuityKind::kJumpRebased:
            return "jump_rebased";
        case DiscontinuityKind::kPositionStep:
            return "position_step";
        case DiscontinuityKind::kNone:
            break;
    }
    return "none";
}

/// 一笔不连续的现场。
///
/// **为什么要 `wall_delta` 与 `raw_delta` 两个原始量**：只记 `step` 的话，
/// 读者无法判断这是"钟停了一下"（`raw = 0`）还是"钟补了一刀"（`raw` 很大而 `wall` 很小）。
/// 这两个数就是那 113 ms 缺口唯一能复算的入口。
struct DiscontinuitySample {
    /// 距首帧读数。用相对量：绝对单调钟在不同会话里差着开机时长，列出来没法读。
    std::int64_t at_ns = 0;
    DiscontinuityKind kind = DiscontinuityKind::kNone;
    /// 这一帧墙钟走了多少。
    std::int64_t wall_delta_ns = 0;
    /// 设备**原始**位置这一帧走了多少（可为负：回退）。
    std::int64_t raw_delta_ns = 0;
    /// 位置实际推进 − 墙钟推进。进 `offset` 的就是它。
    std::int64_t step_ns = 0;
    /// 这一帧之前 / 之后的**原始**领先量（未减去 `offset`）。
    /// 成对出版：只有 `before` 的话，"修了多少"得靠 `before + step` 反推，
    /// 而两处算同一个数正是 P14-X 踩过的坑。
    std::int64_t lead_before_ns = 0;
    std::int64_t lead_after_ns = 0;
};

/// 最近若干次不连续 + 分类计数 + 累计偏移。
///
/// **定长 8 笔**，与 `RebaseTrace` 同一个理由：两小时的会话会有上万个样本，
/// 而"台阶一共吃掉多少、最大那笔长什么样"只需要累计量与三个现场。
struct DiscontinuityTrace {
    /// 事件总数。等于五个分类计数之和（判据 Y2）。
    std::int64_t events = 0;
    std::int64_t extrapolated = 0;
    std::int64_t rejected = 0;
    std::int64_t clamped = 0;
    std::int64_t jumped = 0;
    std::int64_t stepped = 0;
    /// 被跳变分支**丢掉**的最大那一笔（带符号：正 = 钟往前跳，负 = 往回跳）。
    ///
    /// 为什么单独一格：跳变走的是 `position += wall_delta`，`step` 恒 0，所以它
    /// 不进 `offset_ns` —— 于是"这一轮到底丢了多少"在顶层看不见，只能翻 `recent`。
    /// P14-AE 量到 `lead = 0` 有两种来历（真对齐 / 被丢掉），这一格就是把后者说出来的地方。
    std::int64_t jump_max_ns = 0;
    /// 累计偏移：`Σ step`。门用它把台阶从漂移里摘出去。
    std::int64_t offset_ns = 0;
    /// 环里这几笔的 `Σ step`。环只留 8 笔，所以它只在 `events ≤ 8` 时等于 `offset_ns` ——
    /// 判据因此拆成两条（Y1），而不是让"环"和"总账"假装是同一个数。
    std::int64_t sampled_offset_ns = 0;
    /// 环里留下几笔（`min(events, 8)`）。
    std::int64_t sampled = 0;
    std::int64_t first_at_ns = 0;
    /// `|step|` 最大的那一笔。
    DiscontinuitySample max{};
    /// 按时间升序展开的最近若干笔。
    DiscontinuitySample recent[8]{};
    std::int32_t samples_used = 0;
};

}  // namespace kivo::video::core::pace
