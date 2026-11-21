#pragma once

// ============================================================================
// Block Planner —— 播放头优先的块规划（AI 字幕翻译的调度大脑，纯函数）。
//
// 落位路径: backend/include/kivo/cinema_engine/subtitle_translation_core/
//            subtitle_translation_block_planner.hpp
//
// 设计依据: docs/design/ai-subtitle-translation-family-tree.md (v3.2)
//   - **固定网格**：块边界由 cue 序列确定性推导，同输入同输出；唯一例外是
//     落点所在块的局部起点随播放头漂移（评审 F6：跨路径可复现承诺放弃，
//     journal 本来就是实际产出真源）；
//   - **播放头优先**（DEC-15 领先窗口）：先翻播放头所在/之后的未完成块，
//     让用户在最短时间内看到当前位置的中文（评审 F15：翻译比实时快 1.5-1.7×，
//     整片批处理浪费了这个红利）；
//   - **纯函数**：`pick_next_translation_block` 的全部决策可桌面单测 ——
//     播放头重排/空洞回填/续译是全方案最易出 bug 的逻辑（评审 F18/E8），
//     不许下沉到 Kotlin 或真机才暴露。
//
// 时间轴不变性（DEC-07 v2）：本层只对 cue **分组**，不修改任何 cue 的
// start/end/text —— 单测显式断言。
//
// 零平台依赖：纯值类型与纯函数，无 IO、无时钟读取（playhead 由调用方注入）。
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_cue_assembler.hpp"

namespace kivo::cinema_engine {

/// 翻译块：cue 序列上的一个**闭开区间** [first_cue, last_cue)。
///
/// 块内 cue 满足（切块约束，评审 E4/G12 多维上限 + 时间间隙断开）：
///   - ≤25 条（实测 10 行/块回吐英文率 21%，20~25 行最优）；
///   - ≤2000 字符（源文本粗估口径）；
///   - 时间跨度 ≤90 秒；
///   - 相邻 cue 间隙 >2000ms 处强制断开（场景切换处上下文自然割裂）。
/// 单条 cue 自身超限（超长独白）→ 独占成块，不拆 cue（cue 是原子）。
struct SubtitleTranslationBlock
{
    std::size_t block_id{0};      ///< 在块列表中的下标（确定性，跨会话稳定）。
    std::size_t first_cue{0};     ///< 进入 cue 数组的起点（闭）。
    std::size_t last_cue{0};      ///< 终点（开）—— last_cue == first_cue 表示空块。
    std::int64_t start_time_ms{0}; ///< 块首 cue 的 start（空块为 0）。
    std::int64_t end_time_ms{0};   ///< 块尾 cue 的 end（空块为 0）。
};

/// 切块结果：全片固定网格。
struct SubtitleTranslationBlockPlan
{
    std::vector<SubtitleTranslationBlock> blocks;

    /// 总条数（= 输入 cue 数；块只分组不改 cue，时间轴不变性的计数面）。
    std::size_t total_cues{0};
};

/// 多维切块上限（评审 E4/G12；数值来源见设计文档 §1，调参走 decision log）。
struct SubtitleTranslationBlockLimits
{
    std::size_t max_cues{25};
    std::size_t max_chars{2000};
    std::int64_t max_span_ms{90 * 1000};
    std::int64_t scene_gap_ms{2000};   ///< 相邻 cue 间隙超过此值强制断块。
};

/// 固定网格切块。**纯函数**：同 cue 序列 + 同 limits ⇒ 同块列表。
///
/// @param cues        已装配的 cue 时间轴（只读 start/end/text；text 用于
///                    字符数估算，CPS/归一已在 cue_assembler 完成）。
/// @param limits      切块上限（默认值即 v3.2 口径）。
SubtitleTranslationBlockPlan plan_translation_blocks(
    const std::vector<struct SubtitleTranslationCue>& cues,
    const SubtitleTranslationBlockLimits& limits = {}) noexcept;

/// `pick_next_translation_block` 的返回：下一块 + 本轮起始 cue。
struct SubtitleTranslationBlockPick
{
    /// 选中的块下标；`kNoBlock` 表示全部已处理（无可翻内容）。
    std::size_t block_id;
    /// 本轮翻译从该 cue 开始（进入 cue 数组）。
    std::size_t start_cue;
    /// true = 落点块的局部起点（播放头在块中部 —— Preview Slice，评审 E8）；
    /// false = 从块首开始。
    bool preview;

    static constexpr std::size_t kNoBlock = std::numeric_limits<std::size_t>::max();
};

/// 播放头优先选块（DEC-15 领先窗口）。**纯函数**。
///
/// 选择顺序：
///   1. **包含播放头的未完成块** —— start_cue = 播放头之后的首条 cue
///      （Preview Slice：不等块前半段白翻，评审 E8）；
///   2. 播放头**之后**最近的未完成块（保持领先窗口，从块首开始）；
///   3. 播放头**之前**的未完成块（空洞回填，最低优先级，从块首开始）；
///   4. 全部完成 → kNoBlock。
///
/// @param blocks      plan_translation_blocks 的产物。
/// @param cues        原 cue 时间轴（Preview 需要块内每条 cue 的 end 判定）。
/// @param completed   与 blocks 等长的完成位图（断点续译的输入）。
/// @param playhead_ms 当前播放头（调用方注入，本层不读时钟）。
SubtitleTranslationBlockPick pick_next_translation_block(
    const std::vector<SubtitleTranslationBlock>& blocks,
    const std::vector<SubtitleTranslationCue>& cues,
    const std::vector<bool>& completed,
    std::int64_t playhead_ms) noexcept;

}  // namespace kivo::cinema_engine