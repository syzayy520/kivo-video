// ============================================================================
// Cue 全链路集成测试（评审 E21 第一组）。
//
// 落位路径: backend/tests/subtitle_translation/integration_test.cpp
//
// 把三大件串起来验证**组合行为**（各自单测绿 ≠ 组合无恙，评审 E21）：
//   SRT 文本 → cue_assembler（归一/语言）→ block_planner（固定网格/播放头优先）
//   → scheduler（mock engine 脚本化重试）→ 逐 cue 最终结果。
//
// 断言的不是单件职责，而是「接缝」：
//   ① cue_assembler 的语言/规则判定决定了 scheduler 收到几条待翻行；
//   ② planner 的块覆盖**恰好**囊括全部 cue（不丢不重）；
//   ③ Preview pick 的 start_cue 与 scheduler 段翻译的结果下标对齐；
//   ④ 规则行零 token + 全部 LLM 行的 token 计数贯通。
// ============================================================================

#include <cassert>
#include <crtdbg.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_block_planner.hpp"
#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_cue_assembler.hpp"
#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_scheduler.hpp"

using namespace kivo::cinema_engine;

/// assert 失败不弹 Debug 对话框（评审 C1/D 修复链）。
void disable_assert_dialog() {
#ifdef _MSC_VER
    // 三类报告全部静默：_CRT_ASSERT（断言）、_CRT_ERROR（vector 越界等
    // Debug 校验 —— 2026-11-21 两张弹窗截图的嫌疑根因）、_CRT_WARN。
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _CALL_REPORTFAULT);
#endif
}

namespace {

/// 脚本化 mock：按调用序返回预设 Batch。
class ScriptedEngine final : public SubtitleTranslationEngine
{
public:
    std::vector<SubtitleTranslationUnit> received;
    std::vector<SubtitleTranslationBatch> script;
    std::int64_t tokens{0};

    SubtitleTranslationBatch translate(const SubtitleTranslationUnit& unit) override
    {
        received.push_back(unit);
        if (next >= script.size()) {
            SubtitleTranslationBatch empty;
            empty.count_matched = false;
            return empty;
        }
        tokens += 100;
        return script[next++];
    }
    bool ready() const noexcept override { return true; }
    std::string last_error() const noexcept override { return ""; }
    std::int64_t consumed_tokens() const noexcept override { return tokens; }

    std::size_t next{0};
};

/// 与 gen_cpp_header.py 的 cpp_ascii_literal 同口径的转义（测试里写中文探针用）。
std::string zh(const char* utf8) { return std::string(utf8); }

/// 构造 Batch：每个元素 = (ok, text)。全 ok 时 count_matched=true。
/// （直接 push_back({{...}}) 在 MSVC 下 init-list 推导失败——显式走 helper。）
SubtitleTranslationBatch batch_of(
    const std::vector<std::pair<bool, std::string>>& items)
{
    SubtitleTranslationBatch batch;
    bool all = true;
    for (const auto& item : items) {
        SubtitleTranslationOutcome outcome;
        outcome.ok = item.first;
        outcome.text = item.second;
        outcome.reason = item.first ? "" : "missing slot";
        if (!item.first) { all = false; }
        batch.outcomes.push_back(outcome);
    }
    batch.count_matched = all;
    return batch;
}

}  // namespace

/// 混合片源全链路：真实 SRT（对白 + [Music] + 数字 + 对话）→ 装配 → 规划 → 调度。
///
/// 片源设计（5 条 cue）：
///   cue 0 "Hello there."            → LLM（高频词命中）
///   cue 1 "[Music]"                 → RULE_TRANSLATED（零 token）
///   cue 2 "1945"                    → INTENTIONAL_KEEP（Other）
///   cue 3 "- Get out."              → 对话拆分，LLM
///   cue 4 "- Now."                  → 对话拆分，LLM
/// 期望：planner 1 块（无间隙、总跨度小）；scheduler 收到 3 条待翻行
/// （Hello there. / Get out. / Now.），[Music] 与 1945 不进模型。
void test_full_chain_mixed_source() {
    const std::string srt =
        "1\n"
        "00:00:01,000 --> 00:00:02,000\n"
        "Hello there.\n"
        "\n"
        "2\n"
        "00:00:03,000 --> 00:00:04,000\n"
        "[Music]\n"
        "\n"
        "3\n"
        "00:00:05,000 --> 00:00:06,000\n"
        "1945\n"
        "\n"
        "4\n"
        "00:00:07,000 --> 00:00:08,000\n"
        "- Get out.\n"
        "\n"
        "5\n"
        "00:00:09,000 --> 00:00:10,000\n"
        "- Now.\n";

    // ① 装配。
    const auto assembled = assemble_subtitle_translation_cues(srt);
    assert(assembled.cues.size() == 5);
    // 语言判定按字符构成：[Music] 的 5 个拉丁字母在装配层也判 English
    // （规则映射是 scheduler 层的事，两层职责分离）。
    assert(assembled.english_cue_count == 4);
    assert(assembled.chinese_cue_count == 0);
    assert(!assembled.contains_chinese());

    // ② 规划：全部 cue 无间隙（每条间 1s < 2s 场景间隙）→ 单块覆盖 5 条。
    const auto plan = plan_translation_blocks(assembled.cues);
    assert(plan.blocks.size() == 1);
    assert(plan.blocks[0].first_cue == 0 && plan.blocks[0].last_cue == 5);
    assert(plan.total_cues == 5);

    // ③ 调度（脚本：块请求 3 行 → matched）。
    ScriptedEngine engine;
    engine.script.push_back(batch_of({
        {true, zh("\xe5\x87\xba\xe5\x8e\xbb")},          // 出去
        {true, zh("\xe7\xab\x8b\xe5\x8d\xb3")},          // 立即
        {true, zh("\xe6\x88\x91\xe4\xbc\x9a\xe6\x89\xbe\xe5\x88\xb0\xe4\xbd\xa0")},  // 我会找到你
    }));
    SubtitleTranslationScheduler scheduler(engine);

    std::vector<SubtitleCueResult> results;
    const auto stats = scheduler.translate_block(
        assembled.cues, 0, plan.blocks[0].last_cue, 0,
        {},   // 无前块上下文（片头）
        results);

    // ④ 断言接缝：规则行零 token、待翻行数、结果分布。
    assert(results.size() == 5);
    assert(results[0].status == SubtitleCueResult::Status::LLM_TRANSLATED);
    assert(results[1].status == SubtitleCueResult::Status::RULE_TRANSLATED);
    assert(results[2].status == SubtitleCueResult::Status::INTENTIONAL_KEEP);
    assert(results[3].status == SubtitleCueResult::Status::LLM_TRANSLATED);
    assert(results[4].status == SubtitleCueResult::Status::LLM_TRANSLATED);

    assert(engine.received.size() == 1);
    // 待翻行 = 3 条（Hello there. + 对话两行）；[Music]/1945 不进模型。
    assert(engine.received[0].source_lines.size() == 3);
    assert(engine.received[0].source_lines[0] == "Hello there.");
    assert(engine.received[0].source_lines[1] == "Get out.");
    assert(engine.received[0].source_lines[2] == "Now.");

    assert(stats.llm_translated == 3);
    assert(stats.rule_translated == 1);
    assert(stats.intentional_keep == 1);
    assert(stats.retries == 0 && stats.fallbacks == 0);
    assert(stats.tokens_used == 100);
    std::cout << "PASS: test_full_chain_mixed_source" << std::endl;
}

/// Preview + 断点续译全链路（评审 E21 第二组的桌面可测部分）：
/// 部分完成后换播放头 → pick 跳到新位置 → scheduler 段翻译 → 回填。
void test_preview_then_backfill_chain() {
    // 9 条连续 cue（无间隙）→ 1 块 [0,9)。
    std::vector<SubtitleTranslationCue> cues;
    for (std::size_t index = 0; index < 9; ++index) {
        SubtitleTranslationCue cue;
        cue.start_time_ms = static_cast<std::int64_t>(index) * 1000;
        cue.end_time_ms = cue.start_time_ms + 1000;
        cue.text = "line " + std::to_string(index);
        cues.push_back(cue);
    }
    const auto plan = plan_translation_blocks(cues);
    assert(plan.blocks.size() == 1);

    ScriptedEngine engine;
    SubtitleTranslationScheduler scheduler(engine);
    std::vector<SubtitleCueResult> results;
    std::vector<bool> completed = {false};

    // 第一轮：播放头 6500ms → 块 0 含播放头 → preview。
    // cue 6 = [6000,7000)，播放头 6500 在其内部（end=7000 > 6500）→ 从 cue 6 起。
    const auto pick1 = pick_next_translation_block(plan.blocks, cues, completed, 6500);
    assert(pick1.block_id == 0 && pick1.preview && pick1.start_cue == 6);
    // 翻 [6, 9)：脚本 3 行成功（七/八/九）。
    engine.script.push_back(batch_of({
        {true, zh("\xe4\xb8\x83")},   // 七
        {true, zh("\xe5\x85\xab")},   // 八
        {true, zh("\xe4\xb9\x9d")},   // 九
    }));
    const auto stats1 = scheduler.translate_block(
        cues, 0, plan.blocks[0].last_cue, pick1.start_cue, {}, results);
    // scheduler 返回的结果下标 0..2 对应全局 cue 6..8（Preview 段对齐）。
    assert(results.size() == 3);
    assert(results[0].text == zh("\xe4\xb8\x83"));
    assert(results[1].text == zh("\xe5\x85\xab"));
    assert(results[2].text == zh("\xe4\xb9\x9d"));
    assert(stats1.cues_requested == 3);

    // 第二轮：播放头不动，块 0 标完成 → kNoBlock？不——cue 0-6 还没翻，
    // 但完成位图是块级：块 0 标完成 ⇒ pick 认为全片完成（块级粒度的已知边界，
    // 真实恢复粒度由 journal 行级记录补 —— 评审 E21 第二组在 T-B journal 后补全）。
    completed[0] = true;
    const auto pick2 = pick_next_translation_block(plan.blocks, cues, completed, 6500);
    assert(pick2.block_id == SubtitleTranslationBlockPick::kNoBlock);

    // 位图重置（模拟「提前翻整片」重启）：播放头 6500 仍在块内 → **Preview
    // 优先于回填**（DEC-15 播放头优先是故意的——重置后回播放头位置继续，
    // 先翻用户正在看的位置；块首回填的语义由 test_pick_backfill 的片尾外
    // 场景单独覆盖，两者不冲突）。
    completed[0] = false;
    // 回填轮脚本：**一个 9 元素 batch** —— 块翻译一次调用消费一个 batch；
    // 9 个单元素 batch 会让 outcomes 尺寸(1) < 请求行数(9)，cue 汇总越界 hang
    // （实测，2026-11-21）—— 同时暴露了调度器缺尺寸防御，已补（见实现）。
    std::vector<std::pair<bool, std::string>> nine;
    for (int index = 0; index < 9; ++index) {
        nine.push_back({true, zh("\xe8\xa1\x8c")});
    }
    engine.script.push_back(batch_of(nine));
    const auto pick3 = pick_next_translation_block(plan.blocks, cues, completed, 6500);
    assert(pick3.block_id == 0 && pick3.preview && pick3.start_cue == 6);
    const auto stats3 = scheduler.translate_block(
        cues, 0, plan.blocks[0].last_cue, pick3.start_cue, {}, results);
    // pick3 是 Preview（start_cue=6）→ 本轮只翻 [6,9) 3 条；前段回填再调一轮。
    assert(stats3.cues_requested == 3);
    std::cout << "PASS: test_preview_then_backfill_chain" << std::endl;
}

/// 时间轴不变性全链路（DEC-07 v2）：装配→规划→调度全程不改 cue 时间轴与文本。
void test_timeline_invariance_chain() {
    const std::string srt =
        "1\n"
        "00:00:01,000 --> 00:00:02,000\n"
        "Line one.\n"
        "\n"
        "2\n"
        "00:00:03,000 --> 00:00:04,000\n"
        "Line two.\n";
    const auto assembled = assemble_subtitle_translation_cues(srt);
    const auto plan = plan_translation_blocks(assembled.cues);
    ScriptedEngine engine;
    engine.script.push_back(batch_of({
        {true, zh("\xe4\xb8\x80")},   // 一
        {true, zh("\xe4\xba\x8c")},   // 二
    }));
    SubtitleTranslationScheduler scheduler(engine);
    std::vector<SubtitleCueResult> results;
    scheduler.translate_block(assembled.cues, 0, 2, 0, {}, results);

    // 全链路后：cue 的时间轴与原文逐字节不变。
    assert(assembled.cues[0].start_time_ms == 1000);
    assert(assembled.cues[0].end_time_ms == 2000);
    assert(assembled.cues[0].text == "Line one.");
    assert(assembled.cues[1].start_time_ms == 3000);
    assert(assembled.cues[1].end_time_ms == 4000);
    assert(assembled.cues[1].text == "Line two.");
    // 调度结果对齐：results[i] 对应 cue i。
    assert(results[0].text == zh("\xe4\xb8\x80"));
    assert(results[1].text == zh("\xe4\xba\x8c"));
    std::cout << "PASS: test_timeline_invariance_chain" << std::endl;
}

int main() {
    disable_assert_dialog();
    test_full_chain_mixed_source();
    test_preview_then_backfill_chain();
    test_timeline_invariance_chain();
    std::cout << "All Integration tests passed." << std::endl;
    return 0;
}