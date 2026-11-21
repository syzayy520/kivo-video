// ============================================================================
// Block Planner 实现。契约见 subtitle_translation_block_planner.hpp。
//
// 落位路径: backend/src/subtitle_translation_core/subtitle_translation_block_planner.cpp
// 手写源文件（注释含中文）→ CMake 该 target 显式 /utf-8。
// ============================================================================

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_block_planner.hpp"

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_cue_assembler.hpp"

namespace kivo::cinema_engine {

namespace {

/// 字符数粗估：CJK 表意按 2、拉丁/ASCII 按 1（评审 E4：planner 层无 tokenizer，
/// 这只是切块的粗估口径；精确 Token 预算在引擎侧用真实 tokenizer 做最后防线）。
std::size_t estimate_chars(std::string_view text) noexcept
{
    std::size_t units = 0;
    std::size_t index = 0;
    while (index < text.size()) {
        const auto lead = static_cast<unsigned char>(text[index]);
        if (lead < 0x80u) {
            units += 1;
            index += 1;
        } else if ((lead & 0xE0u) == 0xC0u) {
            units += 1;
            index += 2;
        } else {
            units += 2;   // 三字节 CJK / 四字节 emoji 统一按 2 计
            index += lead >= 0xF0u ? 4 : 3;
        }
    }
    return units;
}

}  // namespace

SubtitleTranslationBlockPlan plan_translation_blocks(
    const std::vector<SubtitleTranslationCue>& cues,
    const SubtitleTranslationBlockLimits& limits) noexcept
{
    SubtitleTranslationBlockPlan plan;
    plan.total_cues = cues.size();

    std::size_t first = 0;          // 当前块起点（cue 下标）
    std::size_t count = 0;          // 当前块内条数
    std::size_t chars = 0;          // 当前块内字符粗估

    auto close_block = [&](std::size_t last_exclusive) {
        if (count == 0) { return; }
        SubtitleTranslationBlock block;
        block.block_id = plan.blocks.size();
        block.first_cue = first;
        block.last_cue = last_exclusive;
        block.start_time_ms = cues[first].start_time_ms;
        block.end_time_ms = cues[last_exclusive - 1].end_time_ms;
        plan.blocks.push_back(block);
        first = last_exclusive;
        count = 0;
        chars = 0;
    };

    for (std::size_t index = 0; index < cues.size(); ++index) {
        const auto& cue = cues[index];
        const std::size_t cue_chars = estimate_chars(cue.text);
        const std::int64_t cue_span = cue.end_time_ms - cue.start_time_ms;

        if (count > 0) {
            // 场景间隙断开（评审 E4/G12）。
            const std::int64_t gap = cue.start_time_ms - cues[index - 1].end_time_ms;
            // 多维上限：任一超限即封块（时间跨度用块尾 cue 的 end 推算）。
            const std::int64_t span_with = cue.end_time_ms - cues[first].start_time_ms;
            if (gap > limits.scene_gap_ms ||
                count + 1 > limits.max_cues ||
                chars + cue_chars > limits.max_chars ||
                span_with > limits.max_span_ms) {
                close_block(index);
            }
        }

        // 计入当前条。
        ++count;
        chars += cue_chars;

        // 单条超限的独占块（超长独白）：立即封块（cue 是原子，不拆）。
        if (count == 1 &&
            (cue_chars > limits.max_chars || cue_span > limits.max_span_ms)) {
            close_block(index + 1);
        }
    }
    close_block(cues.size());

    return plan;
}

SubtitleTranslationBlockPick pick_next_translation_block(
    const std::vector<SubtitleTranslationBlock>& blocks,
    const std::vector<SubtitleTranslationCue>& cues,
    const std::vector<bool>& completed,
    std::int64_t playhead_ms) noexcept
{
    SubtitleTranslationBlockPick pick;
    pick.block_id = SubtitleTranslationBlockPick::kNoBlock;
    pick.start_cue = 0;
    pick.preview = false;
    if (blocks.empty() || cues.empty()) { return pick; }

    const auto is_done = [&](std::size_t id) {
        return id < completed.size() ? completed[id] : false;
    };
    // 播放头之后的首条 cue（Preview Slice 起点，评审 E8）：
    // cue 的 end > playhead ⇒ 该 cue 尚未播完/未播到。
    auto first_cue_at_or_after = [&](const SubtitleTranslationBlock& block) {
        for (std::size_t cue = block.first_cue; cue < block.last_cue; ++cue) {
            if (cues[cue].end_time_ms > playhead_ms) { return cue; }
        }
        return block.last_cue;
    };

    // 1) 包含播放头的未完成块（Preview Slice）。
    for (const auto& block : blocks) {
        if (playhead_ms >= block.start_time_ms && playhead_ms < block.end_time_ms &&
            !is_done(block.block_id)) {
            pick.block_id = block.block_id;
            pick.preview = true;
            pick.start_cue = first_cue_at_or_after(block);
            if (pick.start_cue >= block.last_cue || pick.start_cue == block.first_cue) {
                pick.preview = false;
                pick.start_cue = block.first_cue;
            }
            return pick;
        }
    }

    // 2) 播放头之后最近的未完成块（领先窗口）。
    const SubtitleTranslationBlock* next_after = nullptr;
    for (const auto& block : blocks) {
        if (is_done(block.block_id)) { continue; }
        if (block.start_time_ms >= playhead_ms &&
            (next_after == nullptr || block.start_time_ms < next_after->start_time_ms)) {
            next_after = &block;
        }
    }
    if (next_after != nullptr) {
        pick.block_id = next_after->block_id;
        pick.start_cue = next_after->first_cue;
        return pick;
    }

    // 3) 播放头之前的未完成块（空洞回填，最低优先级）。
    const SubtitleTranslationBlock* backfill = nullptr;
    for (const auto& block : blocks) {
        if (is_done(block.block_id)) { continue; }
        if (block.end_time_ms <= playhead_ms &&
            (backfill == nullptr || block.start_time_ms < backfill->start_time_ms)) {
            backfill = &block;
        }
    }
    if (backfill != nullptr) {
        pick.block_id = backfill->block_id;
        pick.start_cue = backfill->first_cue;
        return pick;
    }

    return pick;   // kNoBlock：全部已处理
}

}  // namespace kivo::cinema_engine