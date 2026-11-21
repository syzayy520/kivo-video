#pragma once

// ============================================================================
// 翻译调度器 —— 重试状态机 + 五结果统计（AI 字幕翻译的执行层，纯逻辑）。
//
// 落位路径: backend/include/kivo/cinema_engine/subtitle_translation_core/
//            subtitle_translation_scheduler.hpp
//
// 设计依据: docs/design/ai-subtitle-translation-family-tree.md (v3.2 §2)
//   - **重试不变量**（评审 E29）：温度 0 下同输入必得同输出 —— 任何重试
//     必须改变输入（缺号聚合重编小编号块 / 逐行回退），否则不得发起；
//   - **Cue = 提交单位，Unit 行 = 执行单位**（评审 E6/G6）：cue 内任一行
//     失败 → 整 cue 回退原文（避免半中文半英文）；
//   - **五结果状态**（评审 G13）：LLM_TRANSLATED / RULE_TRANSLATED /
//     INTENTIONAL_KEEP / FAILED_FALLBACK / PENDING；
//   - **BlockStats**（评审 E6/A6）：验收要的数据必须有出口
//     （tokens/retries/fallbacks/rules —— timing 由 T-B 引擎层补充）。
//
// 纯逻辑：Engine 通过构造注入（接口 subtitle_translation_engine.hpp），
// 测试用 mock 注入脚本化结果 —— 全部重试/回退路径桌面单测覆盖。
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_engine.hpp"
#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_cue_assembler.hpp"

namespace kivo::cinema_engine {

/// 单条 cue 的最终翻译结果（五结果状态，评审 G13）。
struct SubtitleCueResult
{
    enum class Status
    {
        LLM_TRANSLATED,    ///< 模型翻译成功
        RULE_TRANSLATED,   ///< 规则映射成功（[Music] → [音乐]，零 token）
        INTENTIONAL_KEEP,  ///< 合法保留原文（纯数字/符号/专名 cue）
        FAILED_FALLBACK,   ///< 翻译失败，显示原文（用户可见，完成卡计数）
        PENDING            ///< 尚未处理
    };

    Status status{Status::PENDING};
    std::string text;    ///< 最终展示文本（FAILED_FALLBACK = 原文）。
    std::string reason;  ///< 非成功态的原因（FAILED_FALLBACK 必须非空，G9）。
};

/// 单块调度统计（验收与排障的数据出口，评审 E6/A6）。
struct SubtitleTranslationBlockStats
{
    std::size_t cues_requested{0};    ///< 本轮请求的 cue 数（含规则/保留）。
    std::size_t llm_translated{0};
    std::size_t rule_translated{0};
    std::size_t intentional_keep{0};
    std::size_t fallbacks{0};         ///< FAILED_FALLBACK 的 cue 数。
    std::size_t retries{0};           ///< 缺号聚合重试次数（0 或 1 —— 每块至多一次）。
    std::size_t line_fallbacks{0};    ///< 逐行回退触发的行数。
    std::int64_t tokens_used{0};      ///< 本轮 consumed_tokens 差值。
};

/// 翻译调度器：构造注入 Engine，按块驱动重试状态机。
class SubtitleTranslationScheduler
{
public:
    struct Config
    {
        bool use_fewshot{true};       ///< 块模式带 few-shot 示范（DEC-03）。
        std::size_t context_cues{2};  ///< 前块原文上下文条数（DEC-14，T-A 消融定去留）。
    };

    explicit SubtitleTranslationScheduler(SubtitleTranslationEngine& engine,
                                          Config config = {}) noexcept;

    /// 翻译一个**连续段** [first_cue, last_cue)（cue 列表的区间）。
    ///
    /// 播放头优先的 Preview 语义由调用方拆段驱动：
    ///   - 本轮翻 [start_cue, last_cue)（start_cue ≥ first_cue）；
    ///   - 前段 [first_cue, start_cue) 留待回填轮再调本函数；
    ///   - out_results 长度 = last_cue - first_cue，下标 i 对应 cue
    ///     (start_cue + i) —— 调用方负责映射回全局 cue 位置。
    ///
    /// @param cues          已装配的 cue 列表（含对话拆分的 dialogue_units）。
    /// @param first_cue     段起点（块首，用于上下文溯源时取 prev 段）。
    /// @param last_cue      段终点（开）。
    /// @param start_cue     本轮实际开始（≥ first_cue；Preview 播放头优先）。
    /// @param prev_context  前块原文末 N 条（进 user 段，DEC-06）。
    /// @param out_results   输出：长度 = last_cue - start_cue。
    /// @return              本段统计。
    SubtitleTranslationBlockStats translate_block(
        const std::vector<SubtitleTranslationCue>& cues,
        std::size_t first_cue, std::size_t last_cue, std::size_t start_cue,
        const std::vector<std::string>& prev_context,
        std::vector<SubtitleCueResult>& out_results) noexcept;

    std::int64_t consumed_tokens() const noexcept;

private:
    SubtitleTranslationEngine& engine_;
    Config config_;
};

}  // namespace kivo::cinema_engine