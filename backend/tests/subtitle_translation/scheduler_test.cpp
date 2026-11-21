// ============================================================================
// 翻译调度器测试。契约见 subtitle_translation_scheduler.hpp。
//
// 落位路径: backend/tests/subtitle_translation/scheduler_test.cpp
//
// 覆盖（设计 v3.2 §2 + 评审 E29/E6/G13/A6）：
//   正常块 / 缺号聚合重试（重试不变量：Unit 只含缺失行）/ 重试仍败逐行回退 /
//   失败行 FAILED_FALLBACK（reason 非空）/ 规则映射零 token / INTENTIONAL_KEEP /
//   上下文贯通（DEC-19/N1）/ Preview 段 / tokens 统计 / 对话 cue 合并回填。
// ============================================================================

#include <cassert>
#include <crtdbg.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_scheduler.hpp"

using namespace kivo::cinema_engine;

/// assert 失败不弹 Debug 对话框（弹窗会卡住 ctest 60s 超时并打扰同机使用）。
void disable_assert_dialog() {
#ifdef _MSC_VER
    // 三类报告全部静默：_CRT_ASSERT（断言）、_CRT_ERROR（vector 越界等
    // Debug 校验 —— 2026-11-21 两张弹窗截图的嫌疑根因）、_CRT_WARN。
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _set_error_mode(_OUT_TO_STDERR);
    // Debug CRT 的 assert() 失败在报告之后还会调 abort() —— abort 本身在
    // /MDd 下默认弹「abort() has been called」对话框（_CALL_REPORTFAULT），
    // 关掉它让失败直接退进程、由 ctest 捕获（2026-11-21 弹窗截图第二轮）。
    _set_abort_behavior(0, _CALL_REPORTFAULT);
#endif
}

namespace {

/// 脚本化 mock：按调用序返回预设 Batch，记录收到的 Unit（断言重试不变量与上下文贯通）。
class MockEngine final : public SubtitleTranslationEngine
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
    std::string last_error() const noexcept override { return last_error_; }
    std::int64_t consumed_tokens() const noexcept override { return tokens; }

    std::size_t next{0};
    std::string last_error_;
};

/// 构造一个 Batch：每个元素 = (ok, text)。全 ok 时 count_matched=true。
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

SubtitleTranslationCue cue_at(std::int64_t start, const std::string& text)
{
    SubtitleTranslationCue cue;
    cue.start_time_ms = start;
    cue.end_time_ms = start + 1000;
    cue.text = text;
    return cue;
}

}  // namespace

/// 正常块：一次成功 → 全 LLM_TRANSLATED，retries/fallbacks=0。
void test_scheduler_normal_block() {
    MockEngine engine;
    engine.script.push_back(batch_of({
        {true, "\xe4\xbd\xa0\xe5\xa5\xbd"},          // 你好
        {true, "\xe4\xbd\xa0\xe5\xa5\xbd\xe5\x90\x97"},  // 你好吗
    }));
    SubtitleTranslationScheduler scheduler(engine);

    std::vector<SubtitleTranslationCue> cues = {
        cue_at(1000, "Hello."), cue_at(2000, "How are you?"),
    };
    std::vector<SubtitleCueResult> results;
    const auto stats = scheduler.translate_block(cues, 0, 2, 0, {}, results);

    assert(results.size() == 2);
    assert(results[0].status == SubtitleCueResult::Status::LLM_TRANSLATED);
    assert(results[1].status == SubtitleCueResult::Status::LLM_TRANSLATED);
    assert(stats.retries == 0 && stats.fallbacks == 0);
    assert(stats.llm_translated == 2);
    assert(stats.tokens_used == 100);
    std::cout << "PASS: test_scheduler_normal_block" << std::endl;
}

/// 缺号聚合重试：第一块 matched=false（2 行缺 1）→ 重试 Unit **只含缺失行**
/// （重试不变量：改变输入；评审 E29）→ 补齐后全 LLM。
void test_scheduler_missing_slot_retry() {
    MockEngine engine;
    // 第一次：2 行只回了 1 行（第 2 槽缺失）。
    engine.script.push_back(batch_of({
        {true, "\xe7\xac\xac\xe4\xb8\x80\xe8\xa1\x8c"},       // 第一行
        {false, ""},                                          // 缺失
    }));
    // 重试：缺失行单独成块成功。
    engine.script.push_back(batch_of({
        {true, "\xe7\xac\xac\xe4\xba\x8c\xe8\xa1\x8c"},       // 第二行
    }));
    SubtitleTranslationScheduler scheduler(engine);

    std::vector<SubtitleTranslationCue> cues = {
        cue_at(1000, "First line."), cue_at(2000, "Second line."),
    };
    std::vector<SubtitleCueResult> results;
    const auto stats = scheduler.translate_block(cues, 0, 2, 0, {}, results);

    assert(stats.retries == 1);
    assert(results[0].status == SubtitleCueResult::Status::LLM_TRANSLATED);
    assert(results[1].status == SubtitleCueResult::Status::LLM_TRANSLATED);

    // 重试不变量：第二次调用的 source_lines **只含缺失行**（第二行原文）。
    assert(engine.received.size() == 2);
    assert(engine.received[1].source_lines.size() == 1);
    assert(engine.received[1].source_lines[0] == "Second line.");
    std::cout << "PASS: test_scheduler_missing_slot_retry" << std::endl;
}

/// 重试仍失败 → 逐行回退；成功行 LLM、失败行 FAILED_FALLBACK（reason 非空）。
void test_scheduler_line_fallback() {
    MockEngine engine;
    engine.script.push_back(batch_of({
        {false, ""},   // 块翻译全缺
        {false, ""},
    }));
    engine.script.push_back(batch_of({
        {false, ""},   // 聚合重试仍失败
    }));
    // 逐行回退（2 行各自一次）。
    engine.script.push_back(batch_of({
        {true, "\xe8\xa1\x8c\xe4\xb8\x80"},   // 行一
    }));
    engine.script.push_back(batch_of({
        {false, ""},                           // 行二穷尽失败
    }));
    SubtitleTranslationScheduler scheduler(engine);

    std::vector<SubtitleTranslationCue> cues = {
        cue_at(1000, "Line one."), cue_at(2000, "Line two."),
    };
    std::vector<SubtitleCueResult> results;
    const auto stats = scheduler.translate_block(cues, 0, 2, 0, {}, results);

    assert(stats.retries == 1);
    assert(stats.line_fallbacks == 2);
    assert(results[0].status == SubtitleCueResult::Status::LLM_TRANSLATED);
    assert(results[1].status == SubtitleCueResult::Status::FAILED_FALLBACK);
    assert(results[1].text == "Line two.");    // 失败回退显示原文（DEC-05）
    assert(!results[1].reason.empty());        // G9：reason 必须非空
    std::cout << "PASS: test_scheduler_line_fallback" << std::endl;
}

/// 规则映射：[Music] → [音乐]，零 token（engine 不被调用）。
void test_scheduler_rule_table() {
    MockEngine engine;
    SubtitleTranslationScheduler scheduler(engine);

    std::vector<SubtitleTranslationCue> cues = {
        cue_at(1000, "[Music]"),
        cue_at(2000, "[Applause]"),
    };
    std::vector<SubtitleCueResult> results;
    const auto stats = scheduler.translate_block(cues, 0, 2, 0, {}, results);

    assert(results[0].status == SubtitleCueResult::Status::RULE_TRANSLATED);
    assert(results[0].text == "\xe9\x9f\xb3\xe4\xb9\x90");        // 音乐
    assert(results[1].text == "\xe6\x8e\x8c\xe5\xa3\xb0");        // 掌声
    assert(stats.rule_translated == 2);
    assert(engine.received.empty());   // 零 token：规则映射不走模型
    assert(stats.tokens_used == 0);
    std::cout << "PASS: test_scheduler_rule_table" << std::endl;
}

/// INTENTIONAL_KEEP：纯数字/符号 cue 保留原文不走模型。
void test_scheduler_intentional_keep() {
    MockEngine engine;
    SubtitleTranslationScheduler scheduler(engine);

    std::vector<SubtitleTranslationCue> cues = {
        cue_at(1000, "..."), cue_at(2000, "1945"),
    };
    std::vector<SubtitleCueResult> results;
    const auto stats = scheduler.translate_block(cues, 0, 2, 0, {}, results);

    assert(results[0].status == SubtitleCueResult::Status::INTENTIONAL_KEEP);
    assert(results[0].text == "...");
    assert(results[1].status == SubtitleCueResult::Status::INTENTIONAL_KEEP);
    assert(engine.received.empty());
    std::cout << "PASS: test_scheduler_intentional_keep" << std::endl;
}

/// 上下文贯通（DEC-19/N1）：prev_context 进 Unit.context_lines，
/// 且成功块与重试块**同带同 context**——前缀 KV 红线由引擎层用 context 参数保证。
void test_scheduler_context_passthrough() {
    MockEngine engine;
    engine.script.push_back(batch_of({
        {true, "\xe8\xaf\x91\xe6\x96\x87"},   // 译文
    }));
    SubtitleTranslationScheduler scheduler(engine);

    std::vector<SubtitleTranslationCue> cues = {cue_at(1000, "Hello.")};
    const std::vector<std::string> prev = {"Earlier line one.", "Earlier line two."};
    std::vector<SubtitleCueResult> results;
    scheduler.translate_block(cues, 0, 1, 0, prev, results);

    assert(engine.received.size() == 1);
    assert(engine.received[0].context_lines.size() == 2);
    assert(engine.received[0].context_lines[0] == "Earlier line one.");
    assert(engine.received[0].context_lines[1] == "Earlier line two.");
    std::cout << "PASS: test_scheduler_context_passthrough" << std::endl;
}

/// Preview 段：start_cue > first_cue → 只翻 [start, last)，结果下标对齐段内。
void test_scheduler_preview_segment() {
    MockEngine engine;
    engine.script.push_back(batch_of({
        {true, "\xe8\xaf\x91\xe4\xba\x8c"},   // 译二
        {true, "\xe8\xaf\x91\xe4\xb8\x89"},   // 译三
    }));
    SubtitleTranslationScheduler scheduler(engine);

    std::vector<SubtitleTranslationCue> cues = {
        cue_at(1000, "One."), cue_at(2000, "Two."), cue_at(3000, "Three."),
    };
    std::vector<SubtitleCueResult> results;
    // first=0 但 start=1（Preview：cue 0 已翻过，本轮从 cue 1 起）。
    const auto stats = scheduler.translate_block(cues, 0, 3, 1, {}, results);

    assert(results.size() == 2);   // 只覆盖 [1,3)
    assert(results[0].text == "\xe8\xaf\x91\xe4\xba\x8c");
    assert(results[1].text == "\xe8\xaf\x91\xe4\xb8\x89");
    assert(stats.cues_requested == 2);
    std::cout << "PASS: test_scheduler_preview_segment" << std::endl;
}

/// 对话 cue：拆多行进 Unit，译文恢复 "- 译文" 逐行形态（评审 A4a 合并语义）。
void test_scheduler_dialogue_roundtrip() {
    MockEngine engine;
    engine.script.push_back(batch_of({
        {true, "\xe4\xbd\xa0\xe5\xa5\xbd"},        // 你好
        {true, "\xe4\xbd\xa0\xe5\xa5\xbd\xe5\x90\x97"},  // 你好吗
    }));
    SubtitleTranslationScheduler scheduler(engine);

    SubtitleTranslationCue dialogue = cue_at(1000, "- Hello.\n- How are you?");
    dialogue.dialogue_units = {"Hello.", "How are you?"};
    std::vector<SubtitleTranslationCue> cues = {dialogue};

    std::vector<SubtitleCueResult> results;
    const auto stats = scheduler.translate_block(cues, 0, 1, 0, {}, results);

    assert(results.size() == 1);
    assert(results[0].status == SubtitleCueResult::Status::LLM_TRANSLATED);
    // 恢复对话形态：两行、每行 "- " 前缀。
    const std::string expected = std::string("- \xe4\xbd\xa0\xe5\xa5\xbd") + "\n" +
                                 std::string("- \xe4\xbd\xa0\xe5\xa5\xbd\xe5\x90\x97");
    assert(results[0].text == expected);
    assert(engine.received[0].source_lines.size() == 2);
    assert(engine.received[0].source_lines[0] == "Hello.");
    std::cout << "PASS: test_scheduler_dialogue_roundtrip" << std::endl;
}

int main() {
    disable_assert_dialog();
    test_scheduler_normal_block();
    test_scheduler_missing_slot_retry();
    test_scheduler_line_fallback();
    test_scheduler_rule_table();
    test_scheduler_intentional_keep();
    test_scheduler_context_passthrough();
    test_scheduler_preview_segment();
    test_scheduler_dialogue_roundtrip();

    std::cout << "All Scheduler tests passed." << std::endl;
    return 0;
}