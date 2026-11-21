#pragma once

// ============================================================================
// Cue 装配器 —— SRT 文本 → 归一 cue 列表 + 语言判定（AI 字幕翻译的输入端）。
//
// 落位路径: backend/include/kivo/cinema_engine/subtitle_translation_core/
//            subtitle_translation_cue_assembler.hpp
//
// 设计依据: docs/design/ai-subtitle-translation-family-tree.md (v3.2 DEC-07 v2 / DEC-17)
//   - cue 内折行合并，**时间轴不变**（DEC-07 v2 撤销跨 cue 合并后这条是硬约束）；
//   - 对话式 cue（每行以 "- " 开头）拆成独立翻译单元，各自翻完再各归各的时间槽；
//   - 语言检测三层（评审 E5）：文件级 / cue 级 / 文本级；假名单独区分
//     （日文汉字也是 CJK —— 只看表意文字会把日文误判成中文）；
//   - CPS 阅读速度检查（DEC-17）：超长译文在对应显示时长内读不完，
//     语义正确也是体验失败 —— 检查在装配端做，不放到 UI 层。
//
// 输入约定：调用方传入**已归一为 UTF-8** 的 SRT 文本（编码归一是导入层
// SubtitleEncoding 的职责，本层只认 UTF-8；检出 NUL 字节视为 UTF-16 漏网，整批拒收）。
//
// 零平台依赖：不含 Android 头，不含 llama.cpp 头。纯函数、无 IO。
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace kivo::cinema_engine {

/// 归一后的单条 cue。**时间轴永不因翻译而改变**（DEC-07 v2）。
struct SubtitleTranslationCue
{
    /// 毫秒时间轴（仓库既有口径 int64）。
    std::int64_t start_time_ms{0};
    std::int64_t end_time_ms{0};

    /// 归一后的单行文本（cue 内折行已合并；标签已剥离）。
    std::string text;

    /// 对话式 cue 的独立翻译单元（每行以 "- " 开头时拆分，行序即播放序）。
    /// 非对话 cue 此表为空 —— 翻译时整条 cue 就是唯一单元。
    std::vector<std::string> dialogue_units;
};

/// 解析期发现的问题。**问题不阻断装配**（宁缺勿错的"缺"指翻译结果，
/// 输入端的畸形 cue 尽量保住：时间轴错了照样进列表，只是带 issue 上报）。
struct SubtitleTranslationCueIssue
{
    /// 在输入里出现的第几个 cue 块（0 起算，含被跳过的）。
    std::size_t source_index{0};

    /// 机器可读类别："timestamp" / "empty" / "ordering" / "encoding" / "cps"。
    std::string category;

    /// 人读原因（带进 trace，用户不可见）。
    std::string reason;
};

/// cue 级语言判定（评审 E5 三层规则的中间层）。
enum class SubtitleCueLanguage
{
    English,   ///< 拉丁字母为主，高频英文词命中
    Chinese,   ///< CJK 表意为主
    Japanese,  ///< 含假名（假名优先于表意 —— 日文汉字不能算中文，G5）
    Other,     ///< 数字/符号/专有名词为主（INTENTIONAL_KEEP 候选）
    Mixed      ///< 同 cue 内多语言混排（v1 明示不保证，走整 cue 保守翻译）
};

/// 装配结果：cue 列表 + 语言统计 + 问题清单。
struct SubtitleTranslationCueAssembly
{
    std::vector<SubtitleTranslationCue> cues;
    std::vector<SubtitleTranslationCueIssue> issues;

    // ---- 语言统计（文件级，UX「已包含中文/仅支持英文」提示的依据）----
    std::size_t english_cue_count{0};
    std::size_t chinese_cue_count{0};
    std::size_t japanese_cue_count{0};
    std::size_t other_cue_count{0};
    std::size_t mixed_cue_count{0};

    /// 是否已含中文（≥30% cue 为中文）——UX 拒收「该字幕已包含中文」的依据。
    bool contains_chinese() const noexcept
    {
        const std::size_t total = english_cue_count + chinese_cue_count +
                                  japanese_cue_count + other_cue_count + mixed_cue_count;
        return total > 0 && chinese_cue_count * 10 >= total * 3;
    }

    /// 是否含假名（日文提示）。
    bool contains_kana() const noexcept { return japanese_cue_count > 0; }
};

/// 把 SRT 文本装配成归一 cue 列表。
///
/// 容错清单（评审 E31）：BOM 剥离、CRLF/CR 统一、序号行可选（重复/乱序不阻断）、
/// 时间戳兼容 `,` 与 `.` 毫秒分隔、缺时段从右往左解析、零时长与开始>结束保留但上报、
/// 空 cue 跳过并上报、UTF-16 漏网（NUL 字节）整批拒收（返回空列表 + encoding issue）。
///
/// @param srt_utf8   UTF-8 编码的 SRT 全文。
/// @param out_issues 可选；非空时接收全部问题（含 CPS 超限）。
/// @return           装配结果。拒收时 cues 为空。
SubtitleTranslationCueAssembly assemble_subtitle_translation_cues(
    std::string_view srt_utf8,
    std::vector<SubtitleTranslationCueIssue>* out_issues = nullptr) noexcept;

/// cue 级语言判定。
///
/// 规则（顺序即优先级，2026-11-21 实测修订）：
///   1. 含假名（U+3040..U+30FF）→ Japanese（假名是最强信号）；
///   2. CJK 表意占比 ≥30% → Chinese；
///   3. CJK 与拉丁都存在且都不占优 → Mixed（真混排，v1 保守整 cue 翻译）；
///   4. 拉丁为主 → English（高频词表区分西/法/德是打地鼠 ——
///      "Stop right there." 三个实词无一命中即翻车实测，v1 输入只支持英文，
///      拉丁语系整 cue 进同一翻译管线）；
///   5. 其余 → Other（纯数字/符号/专名）。
SubtitleCueLanguage detect_subtitle_cue_language(std::string_view text) noexcept;

/// 单条 cue 的 CPS（每秒字符数）。中文字符按 1 计、拉丁字母按 0.5 计的
/// 统一口径（评审 E26：口径一致才可比）。
double subtitle_cue_cps(std::string_view text, std::int64_t duration_ms) noexcept;

}  // namespace kivo::cinema_engine