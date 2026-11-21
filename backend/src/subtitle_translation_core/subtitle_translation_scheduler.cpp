// ============================================================================
// 翻译调度器实现。契约见 subtitle_translation_scheduler.hpp。
//
// 落位路径: backend/src/subtitle_translation_core/subtitle_translation_scheduler.cpp
// 手写源文件（注释含中文）→ CMake 该 target 显式 /utf-8。
// ============================================================================

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_scheduler.hpp"

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_prompt.hpp"

#include <algorithm>

namespace kivo::cinema_engine {

namespace {

// 规则映射表（评审 A12/G26）：整 cue 精确匹配（trim + 大小写不敏感），零 token。
// 用 struct 而非 string_view[][2]：二维数组的 rule[1] 会 decay 成「第二行」，
// 语义陷阱实测抓出过（out 被赋成 [applause] 而非 [音乐]，2026-11-21）。
struct SubtitleRulePair
{
    std::string_view from;
    std::string_view to;
};

constexpr SubtitleRulePair kRuleTable[] = {
    {"[music]", "\xe9\x9f\xb3\xe4\xb9\x90"},       // [音乐]
    {"[applause]", "\xe6\x8e\x8c\xe5\xa3\xb0"},    // [掌声]
    {"[laughter]", "\xe7\xac\x91\xe5\xa3\xb0"},    // [笑声]
    {"[music plays]", "\xe9\x9f\xb3\xe4\xb9\x90"}, // [音乐]
};

bool equals_ci(std::string_view a, std::string_view b) noexcept
{
    if (a.size() != b.size()) { return false; }
    for (std::size_t index = 0; index < a.size(); ++index) {
        const char ca = a[index] >= 'A' && a[index] <= 'Z'
                            ? static_cast<char>(a[index] - 'A' + 'a')
                            : a[index];
        const char cb = b[index] >= 'A' && b[index] <= 'Z'
                            ? static_cast<char>(b[index] - 'A' + 'a')
                            : b[index];
        if (ca != cb) { return false; }
    }
    return true;
}

/// cue 是否走规则映射（整 cue 精确匹配查表）。命中返回译文。
bool rule_translate(std::string_view text, std::string& out) noexcept
{
    std::string_view trimmed = text;
    while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) {
        trimmed.remove_prefix(1);
    }
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t')) {
        trimmed.remove_suffix(1);
    }
    for (const auto& rule : kRuleTable) {
        if (equals_ci(trimmed, rule.from)) {
            out = std::string(rule.to);
            return true;
        }
    }
    return false;
}

/// 待翻行：行原文 + 归属（段内 cue 下标 + cue 内行号）。
struct PendingLine
{
    std::size_t cue_slot{0};      ///< 段内 cue 下标（out_results 下标）。
    std::size_t line_index{0};    ///< cue 内行号（对话 cue 多行时 0..n-1）。
    std::string source;           ///< 行原文。
};

/// cue 的行拆分：对话 cue 取 dialogue_units，普通 cue 取 [text]。
std::vector<std::string> cue_lines(const SubtitleTranslationCue& cue)
{
    if (!cue.dialogue_units.empty()) { return cue.dialogue_units; }
    return {cue.text};
}

/// 行译文按 cue 形态合并回展示文本：对话 cue 恢复 "- 译文" 逐行；普通单行直取。
std::string join_cue_lines(const SubtitleTranslationCue& cue,
                           const std::vector<std::string>& translations,
                           std::size_t begin_line)
{
    if (cue.dialogue_units.empty()) { return translations[begin_line]; }
    std::string out;
    for (std::size_t index = 0; index < cue.dialogue_units.size(); ++index) {
        out.append("- ");
        out.append(translations[begin_line + index]);
        if (index + 1 < cue.dialogue_units.size()) { out.push_back('\n'); }
    }
    return out;
}

}  // namespace

SubtitleTranslationScheduler::SubtitleTranslationScheduler(
    SubtitleTranslationEngine& engine) noexcept
    : engine_(engine), config_(Config())
{
}

SubtitleTranslationScheduler::SubtitleTranslationScheduler(
    SubtitleTranslationEngine& engine, Config config) noexcept
    : engine_(engine), config_(config)
{
}

SubtitleTranslationBlockStats SubtitleTranslationScheduler::translate_block(
    const std::vector<SubtitleTranslationCue>& cues,
    std::size_t first_cue, std::size_t last_cue, std::size_t start_cue,
    const std::vector<std::string>& prev_context,
    std::vector<SubtitleCueResult>& out_results) noexcept
{
    SubtitleTranslationBlockStats stats;
    stats.cues_requested = last_cue - start_cue;
    out_results.assign(stats.cues_requested, SubtitleCueResult{});
    const std::int64_t tokens_before = engine_.consumed_tokens();

    // ---- 第一轮：规则映射 + 合法保留，剩余进模型待翻池 ----
    std::vector<PendingLine> pending;
    for (std::size_t slot = 0; start_cue + slot < last_cue; ++slot) {
        const auto& cue = cues[start_cue + slot];
        std::string mapped;
        if (rule_translate(cue.text, mapped)) {
            out_results[slot] = {SubtitleCueResult::Status::RULE_TRANSLATED,
                                 mapped, ""};
            ++stats.rule_translated;
            continue;
        }
        if (detect_subtitle_cue_language(cue.text) == SubtitleCueLanguage::Other) {
            // 纯数字/符号/专名（INTENTIONAL_KEEP，评审 G13）：保留原文不走模型。
            out_results[slot] = {SubtitleCueResult::Status::INTENTIONAL_KEEP,
                                 cue.text, ""};
            ++stats.intentional_keep;
            continue;
        }
        const auto lines = cue_lines(cue);
        for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
            pending.push_back({slot, line_index, lines[line_index]});
        }
    }

    if (pending.empty()) {
        stats.tokens_used = engine_.consumed_tokens() - tokens_before;
        return stats;
    }

    // ---- 第二轮：块翻译（context 进 user 段，DEC-06/DEC-19）----
    SubtitleTranslationUnit unit;
    unit.use_fewshot = config_.use_fewshot;
    unit.context_lines = prev_context;
    for (const auto& line : pending) { unit.source_lines.push_back(line.source); }

    SubtitleTranslationBatch batch = engine_.translate(unit);

    auto slot_ok = [&](std::size_t line_position) {
        return line_position < batch.outcomes.size() && batch.outcomes[line_position].ok &&
               !batch.outcomes[line_position].text.empty();
    };

    // ---- 第三轮：缺号聚合重试（**必须改变输入** —— 缺失行重组小编号块，
    //      评审 E29 重试不变量；温度 0 下同输入重试无意义）。----
    std::vector<std::size_t> still_missing;
    if (!batch.count_matched) {
        for (std::size_t position = 0; position < pending.size(); ++position) {
            if (!slot_ok(position)) { still_missing.push_back(position); }
        }
    }
    if (!still_missing.empty() && !engine_.last_error().empty()) {
        // 引擎级失败（模型未加载等）：不重试，直接判失败。
    } else if (!still_missing.empty()) {
        ++stats.retries;
        SubtitleTranslationUnit retry_unit;
        retry_unit.use_fewshot = false;   // 小块不再示范（改变输入的一部分）
        for (const std::size_t position : still_missing) {
            retry_unit.source_lines.push_back(pending[position].source);
        }
        const SubtitleTranslationBatch retry = engine_.translate(retry_unit);
        std::size_t filled = 0;
        for (const std::size_t position : still_missing) {
            if (filled < retry.outcomes.size() && retry.outcomes[filled].ok &&
                !retry.outcomes[filled].text.empty()) {
                batch.outcomes[position] = retry.outcomes[filled];
                batch.count_matched = true;   // 只要补齐了就不再走逐行
            }
            ++filled;
        }
        // 重算 still_missing（重试后仍缺的行才走逐行）。
        still_missing.clear();
        for (std::size_t position = 0; position < pending.size(); ++position) {
            if (!slot_ok(position)) { still_missing.push_back(position); }
        }
    }

    // ---- 第四轮：逐行回退（独立单行请求；失败的行 FAILED_FALLBACK 原文）----
    for (const std::size_t position : still_missing) {
        ++stats.line_fallbacks;
        SubtitleTranslationUnit single;
        single.use_fewshot = true;   // 单行 few-shot 是实测涨 BLEU 的口径
        single.source_lines.push_back(pending[position].source);
        const SubtitleTranslationBatch single_batch = engine_.translate(single);
        if (!single_batch.outcomes.empty() && single_batch.outcomes[0].ok &&
            !single_batch.outcomes[0].text.empty()) {
            batch.outcomes[position] = single_batch.outcomes[0];
        } else {
            SubtitleTranslationOutcome failed;
            failed.ok = false;
            failed.text.clear();
            failed.reason = "line fallback exhausted";
            batch.outcomes[position] = failed;
        }
    }

    // ---- cue 级汇总（评审 E6/G6：Cue=提交单位；任一行失败整 cue 原文回退）----
    std::size_t lines_seen = 0;
    for (std::size_t slot = 0; start_cue + slot < last_cue; ++slot) {
        const auto& cue = cues[start_cue + slot];
        if (out_results[slot].status != SubtitleCueResult::Status::PENDING) {
            continue;   // 规则/保留已定
        }
        const auto lines = cue_lines(cue);
        bool all_ok = true;
        std::string first_reason;
        std::vector<std::string> translations;
        for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
            // 【尺寸防御】引擎返回的 outcomes 不足以覆盖全部行时
            // （实现 bug 或脚本/数据异常），按失败处理而不是越界 ——
            // G9 宁缺勿错：实测 9 行请求配 1 行 outcomes 直接越界 hang。
            const std::size_t outcome_at = lines_seen + line_index;
            if (outcome_at >= batch.outcomes.size()) {
                all_ok = false;
                if (first_reason.empty()) {
                    first_reason = "engine returned fewer outcomes than lines";
                }
                translations.push_back("");
                continue;
            }
            const auto& outcome = batch.outcomes[outcome_at];
            if (!outcome.ok || outcome.text.empty()) {
                all_ok = false;
                if (first_reason.empty()) { first_reason = outcome.reason; }
            }
            translations.push_back(outcome.text);
        }
        lines_seen += lines.size();

        if (all_ok) {
            out_results[slot] = {SubtitleCueResult::Status::LLM_TRANSLATED,
                                 join_cue_lines(cue, translations, 0), ""};
            ++stats.llm_translated;
        } else {
            // 整 cue 原文回退：半中文半英文比全英文更糟（评审 E6）。
            out_results[slot] = {SubtitleCueResult::Status::FAILED_FALLBACK,
                                 cue.text,
                                 first_reason.empty() ? "block misaligned" : first_reason};
            ++stats.fallbacks;
        }
    }

    stats.tokens_used = engine_.consumed_tokens() - tokens_before;
    return stats;
}

std::int64_t SubtitleTranslationScheduler::consumed_tokens() const noexcept
{
    return engine_.consumed_tokens();
}

}  // namespace kivo::cinema_engine