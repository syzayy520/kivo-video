#pragma once

#include <cstdint>

namespace kivo::video::core::pace {

/// 一次漂移复位的现场（P14-X）。
///
/// **为什么需要它**：P14-W 在 4K30 的一轮里看到 `drift_max_correction_ns = 104.71 ms`
/// —— 是迟滞带（5 ms）的 21 倍，而同一批的另外两轮只有 5.2 / 5.8 ms。
/// 算术给出一个唯一自洽的解释（`350.16 − 49×5 = 105.16`）：
/// **启动期盲区（首帧后 1 s）里累积的量，在盲区结束的第一次读上被一次还清。**
/// 但那只是算术 —— 证据里没有逐次修正的时刻，所以它停在"最自洽的解释"。
///
/// 这个结构就是给它一个时刻：每一笔修正都记下"什么时候、修了多少、修之前领先多少"。
struct RebaseSample {
    /// 距**首帧读数**的墙钟毫秒数。用相对量而不是绝对 `mono`：
    /// 绝对单调时钟在不同会话里差着开机时长，列出来没法读。
    std::int64_t at_ns = 0;

    /// 这一笔修了多少（正值，单位 ns）。
    std::int64_t amount_ns = 0;

    /// 修之前那一眼看到的领先量。**恒 ≥ `amount_ns`** —— 修正只还超出部分，不会多于看到的值。
    /// 万一出现 `amount > lead_before`，说明账算错了，判据 X2 守这条。
    std::int64_t lead_before_ns = 0;
};

/// 最近若干次复位的轨迹 + 首末与最大的一笔。
///
/// **不做无界累积**：只需要回答"最大那一笔发生在什么时候"，
/// 留最近 8 笔就够看出形状（迟滞带的锯齿约每 1 s 一笔，8 笔 = 8 s）。
/// 无界列表会让一份两小时的报告里塞进上万个样本，而没人为那个精度付钱。
struct RebaseTrace {
    /// 总共复位几次。与 `PaceSummary::drift_rebases` 必须相等（判据 X1）。
    std::int64_t rebases = 0;

    /// 环里留下了几笔（`min(rebases, 8)`）。
    std::int64_t sampled = 0;

    /// 第一笔的时刻。它可能已经被环挤掉，所以单独留一格 ——
    /// "第一笔在多少秒"正是启动期假设要问的那个数。
    std::int64_t first_at_ns = 0;

    /// 最大的一笔（可能已被环挤掉，所以单独留）。`rebases == 0` 时全 0 且不可读。
    RebaseSample max{};

    /// 最近 8 笔，按时间升序。`samples_used` 之后的内容无效。
    RebaseSample recent[8]{};
    std::int64_t samples_used = 0;
};

}  // namespace kivo::video::core::pace
