// ============================================================================
// Block Planner 测试。契约见 subtitle_translation_block_planner.hpp。
//
// 落位路径: backend/tests/subtitle_translation/block_planner_test.cpp
//
// 覆盖（设计 v3.2 §4 + 评审 E4/E8/F6/F15）：
//   固定网格切块（间隙断开 / 多维上限 / 单条超限独占 / 时间轴不变性）+
//   播放头优先选块（Preview Slice / 领先窗口 / 空洞回填 / 全完成 kNoBlock /
//   已完成位图容错）。
// ============================================================================

#include <cassert>
#include <crtdbg.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_block_planner.hpp"

using namespace kivo::cinema_engine;

/// assert 失败不弹 Debug 对话框（弹窗会卡住 ctest 60s 超时、还会打扰
/// 同机使用的人——2026-11-21 实测弹窗截图）。失败改写 stderr，
/// ctest --output-on-failure 能完整捕获。
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

/// 造 cue：start 从 base 开始每条 span_ms，end = start + dur。
SubtitleTranslationCue make_cue(std::int64_t start_ms, std::int64_t dur_ms,
                                const std::string& text = "x")
{
    SubtitleTranslationCue cue;
    cue.start_time_ms = start_ms;
    cue.end_time_ms = start_ms + dur_ms;
    cue.text = text;
    return cue;
}

std::vector<SubtitleTranslationCue> make_sequential(std::size_t count,
                                                    std::int64_t dur_ms = 1000)
{
    std::vector<SubtitleTranslationCue> cues;
    for (std::size_t index = 0; index < count; ++index) {
        cues.push_back(make_cue(static_cast<std::int64_t>(index) * dur_ms, dur_ms));
    }
    return cues;
}

}  // namespace

/// 空输入 → 空块列表 + total_cues=0。
void test_plan_empty() {
    const auto plan = plan_translation_blocks({});
    assert(plan.blocks.empty());
    assert(plan.total_cues == 0);
    std::cout << "PASS: test_plan_empty" << std::endl;
}

/// 单 cue → 单块；块时间轴 = cue 时间轴（时间轴不变性）。
void test_plan_single_cue() {
    const std::vector<SubtitleTranslationCue> cues = {make_cue(5000, 2000, "hello")};
    const auto plan = plan_translation_blocks(cues);
    assert(plan.blocks.size() == 1);
    assert(plan.total_cues == 1);
    assert(plan.blocks[0].first_cue == 0);
    assert(plan.blocks[0].last_cue == 1);
    assert(plan.blocks[0].start_time_ms == 5000);
    assert(plan.blocks[0].end_time_ms == 7000);
    // 时间轴不变性（DEC-07 v2）：切块不改 cue。
    assert(cues[0].start_time_ms == 5000);
    assert(cues[0].end_time_ms == 7000);
    assert(cues[0].text == "hello");
    std::cout << "PASS: test_plan_single_cue" << std::endl;
}

/// 相邻 cue（无间隙）连续 25 条 → 恰好一块；30 条 → 两块（25+5）。
void test_plan_max_cues() {
    const auto cues25 = make_sequential(25);
    const auto plan25 = plan_translation_blocks(cues25);
    assert(plan25.blocks.size() == 1);
    assert(plan25.blocks[0].last_cue - plan25.blocks[0].first_cue == 25);

    const auto cues30 = make_sequential(30);
    const auto plan30 = plan_translation_blocks(cues30);
    assert(plan30.blocks.size() == 2);
    assert(plan30.blocks[0].first_cue == 0 && plan30.blocks[0].last_cue == 25);
    assert(plan30.blocks[1].first_cue == 25 && plan30.blocks[1].last_cue == 30);
    std::cout << "PASS: test_plan_max_cues" << std::endl;
}

/// 时间间隙 >2s 强制断块（评审 E4/G12 场景切换）。
void test_plan_scene_gap() {
    std::vector<SubtitleTranslationCue> cues = {
        make_cue(0, 1000, "a"),
        make_cue(1000, 1000, "b"),
        make_cue(5000, 1000, "c"),   // 与前条间隙 3000ms > 2000 → 断
        make_cue(6000, 1000, "d"),
    };
    const auto plan = plan_translation_blocks(cues);
    assert(plan.blocks.size() == 2);
    assert(plan.blocks[0].last_cue == 2);
    assert(plan.blocks[1].first_cue == 2);
    std::cout << "PASS: test_plan_scene_gap" << std::endl;
}

/// 时间跨度上限：90s 内的连续 cue 恰好封块。
void test_plan_max_span() {
    // 4 条各 30s：第 3 条使跨度 90s（不超），第 4 条使跨度 120s（超）→ 封块。
    std::vector<SubtitleTranslationCue> cues = {
        make_cue(0, 30000, "1"),
        make_cue(30000, 30000, "2"),
        make_cue(60000, 30000, "3"),
        make_cue(90000, 30000, "4"),
    };
    const auto plan = plan_translation_blocks(cues);
    assert(plan.blocks.size() == 2);
    assert(plan.blocks[0].last_cue == 3);
    assert(plan.blocks[1].first_cue == 3);
    std::cout << "PASS: test_plan_max_span" << std::endl;
}

/// 单条超限（超长独白）→ 独占成块，不拆 cue。
void test_plan_oversized_cue_solo() {
    std::vector<SubtitleTranslationCue> cues = {
        make_cue(0, 1000, "short"),
        make_cue(1000, 200000, "monologue"),   // 200s 跨度 > 90s 上限
        make_cue(201000, 1000, "after"),
    };
    const auto plan = plan_translation_blocks(cues);
    assert(plan.blocks.size() == 3);
    assert(plan.blocks[1].first_cue == 1 && plan.blocks[1].last_cue == 2);
    assert(plan.blocks[1].end_time_ms - plan.blocks[1].start_time_ms == 200000);
    std::cout << "PASS: test_plan_oversized_cue_solo" << std::endl;
}

/// 造 3 块结构：每块 3 条各 1s，块间 3s 场景间隙。
/// 块 0 = cue 0-2 [0,3000)ms；块 1 = cue 3-5 [6000,9000)ms；块 2 = cue 6-8 [12000,15000)ms。
std::vector<SubtitleTranslationCue> make_three_blocks()
{
    std::vector<SubtitleTranslationCue> cues;
    for (std::size_t block = 0; block < 3; ++block) {
        for (std::size_t index = 0; index < 3; ++index) {
            cues.push_back(make_cue(
                static_cast<std::int64_t>(block) * 6000 + static_cast<std::int64_t>(index) * 1000,
                1000));
        }
    }
    return cues;
}

/// pick：播放头在未完成块内 → preview=true，start_cue=播放头后首条（评审 E8）。
void test_pick_preview_slice() {
    const auto cues = make_three_blocks();
    const auto plan = plan_translation_blocks(cues);
    assert(plan.blocks.size() == 3);   // 场景间隙切开 3 块（前置确认）
    std::vector<bool> completed(3, false);

    // 播放头 2500ms：在块 0 [0,3000) 内，cue 0/1 已播（end ≤ 2500），cue 2 未播。
    const auto pick = pick_next_translation_block(plan.blocks, cues, completed, 2500);
    assert(pick.block_id == 0);
    assert(pick.preview);
    assert(pick.start_cue == 2);
    std::cout << "PASS: test_pick_preview_slice" << std::endl;
}

/// pick：播放头所在块已完成 → 播放头之后最近的未完成块（领先窗口）。
void test_pick_lead_window() {
    const auto cues = make_three_blocks();
    const auto plan = plan_translation_blocks(cues);
    std::vector<bool> completed = {true, false, false};

    // 播放头 2500ms：块 0 已完成 → 领先窗口选块 1（6000-9000 最近未完成）。
    const auto p1 = pick_next_translation_block(plan.blocks, cues, completed, 2500);
    assert(p1.block_id == 1 && p1.start_cue == 3 && !p1.preview);

    // 块 1 也完成后 → 选块 2。
    completed[1] = true;
    const auto p2 = pick_next_translation_block(plan.blocks, cues, completed, 2500);
    assert(p2.block_id == 2 && p2.start_cue == 6);
    std::cout << "PASS: test_pick_lead_window" << std::endl;
}

/// pick：全部完成 → kNoBlock。
void test_pick_all_done() {
    const auto cues = make_three_blocks();
    const auto plan = plan_translation_blocks(cues);
    std::vector<bool> completed = {true, true, true};
    const auto pick = pick_next_translation_block(plan.blocks, cues, completed, 2500);
    assert(pick.block_id == SubtitleTranslationBlockPick::kNoBlock);
    std::cout << "PASS: test_pick_all_done" << std::endl;
}

/// pick：播放头之后全完成 → 空洞回填（播放头之前最早未完成块）。
void test_pick_backfill() {
    const auto cues = make_three_blocks();
    const auto plan = plan_translation_blocks(cues);
    std::vector<bool> completed = {false, true, true};

    // 播放头 16000ms（片尾外）：无块含播放头、领先窗口无未完成 → 回填块 0。
    const auto pick = pick_next_translation_block(plan.blocks, cues, completed, 16000);
    assert(pick.block_id == 0);
    assert(pick.start_cue == 0 && !pick.preview);
    std::cout << "PASS: test_pick_backfill" << std::endl;
}

/// pick：播放头在两块之间的间隙 → 选下一块（从块首，非 preview）。
void test_pick_in_gap() {
    const auto cues = make_three_blocks();
    const auto plan = plan_translation_blocks(cues);
    std::vector<bool> completed = {false, false, false};

    // 播放头 4000ms：不在任何块 [0,3000)/[6000,9000) → 领先窗口选块 1。
    const auto pick = pick_next_translation_block(plan.blocks, cues, completed, 4000);
    assert(pick.block_id == 1);
    assert(pick.start_cue == 3 && !pick.preview);
    std::cout << "PASS: test_pick_in_gap" << std::endl;
}

/// pick：已完成位图短于 blocks → 视为未完成（容错）。
void test_pick_short_completed_bitmap() {
    const auto cues = make_three_blocks();
    const auto plan = plan_translation_blocks(cues);
    std::vector<bool> completed = {false};   // 块 1/2 无位图信息

    const auto pick = pick_next_translation_block(plan.blocks, cues, completed, 2500);
    assert(pick.block_id == 0);   // 块 0 含播放头且视为未完成
    assert(pick.preview);
    std::cout << "PASS: test_pick_short_completed_bitmap" << std::endl;
}

/// 集成：cue_assembler → planner 的类型贯通（同一家族内组合）。
void test_plan_from_assembled_cues() {
    const std::string srt =
        "1\n"
        "00:00:01,000 --> 00:00:02,000\n"
        "Hello.\n"
        "\n"
        "2\n"
        "00:00:02,000 --> 00:00:03,000\n"
        "How are you?\n";
    const auto assembled = assemble_subtitle_translation_cues(srt);
    const auto plan = plan_translation_blocks(assembled.cues);
    assert(plan.blocks.size() == 1);
    assert(plan.total_cues == 2);
    assert(plan.blocks[0].start_time_ms == 1000);
    assert(plan.blocks[0].end_time_ms == 3000);
    std::cout << "PASS: test_plan_from_assembled_cues" << std::endl;
}

int main() {
    disable_assert_dialog();
    test_plan_empty();
    test_plan_single_cue();
    test_plan_max_cues();
    test_plan_scene_gap();
    test_plan_max_span();
    test_plan_oversized_cue_solo();
    test_pick_preview_slice();
    test_pick_lead_window();
    test_pick_all_done();
    test_pick_backfill();
    test_pick_in_gap();
    test_pick_short_completed_bitmap();
    test_plan_from_assembled_cues();

    std::cout << "All Block Planner tests passed." << std::endl;
    return 0;
}