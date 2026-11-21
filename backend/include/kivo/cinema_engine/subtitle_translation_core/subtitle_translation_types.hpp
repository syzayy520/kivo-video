#pragma once

// ============================================================================
// 字幕翻译的共享契约词汇。
//
// 落位路径: backend/include/kivo/cinema_engine/subtitle_translation_core/
//            subtitle_translation_types.hpp
//
// 家族树位置：`cinema_engine` 家族下的 `subtitle_translation_core`，
// 与既有的 `subtitle_core`（字幕轨/条目/渲染计划）并列 ——
// `subtitle_core` 管「已有字幕怎么存怎么渲染」，本文件管「字幕怎么翻译出来」。
//
// 零平台依赖：不含 Android 头，不含 llama.cpp 头。
// 翻译引擎（llama.cpp）在这层之下，本层只定义「进什么、出什么、失败怎么说」。
//
// 为什么是纯类型（无 .cpp）：这三个结构体全是数据，没有行为。
// 与既有的 subtitle_entry.hpp 同款做法。
// ============================================================================

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::cinema_engine {

/// SubtitleTranslationUnit —— 一次翻译请求的粒度。
/// schema_version: cinema_engine.subtitle_translation.unit.v1
///
/// 两种粒度：
///   - 单行（source_lines.size() == 1）：一条字幕一次请求
///   - 块  （source_lines.size() > 1）  ：一次请求翻多条
///
/// 「块」这一档不是设计偏好，是实测逼出来的：
///   system prompt 约 150 token。逐行模式下它被**每行**重复 prefill，
///   1 小时视频（1000 行）要 prefill 17 万 token ——
///   K40（骁龙 870）实测 prefill 68 tok/s，即 42 分钟，功能等于没有。
///   块模式（20 行/块）把它摊薄到 24 token/行，**省 7.1 倍**，
///   且 chrF 不降反升（19.68 vs 19.89）。
///
/// 建议块大小 20~25 行：实测 10 行/块时小模型回吐英文率从 1% 涨到 21%。
struct SubtitleTranslationUnit
{
    std::string schema_version{"cinema_engine.subtitle_translation.unit.v1"};

    /// 英文原文，按播放顺序。一行一个字幕条目。
    std::vector<std::string> source_lines;

    /// 是否在 prompt 里带上真实样例（few-shot）。
    /// 实测 1.7B 上它让 BLEU 从 8.02 涨到 13.77，**零训练成本**。
    bool use_fewshot{false};

    /// 前块原文上下文（v3.2 DEC-19，2026-11-21）。
    ///
    /// 设计约束（自查 N1 / DEC-06）：**只能进 user 轮** —— system + few-shot
    /// 组成的前缀区必须与源内容无关且逐字节恒定，否则前缀 KV 复用（红线③）失效。
    /// 引擎实现须用 build_subtitle_translation_block_prompt 的 context 参数传入。
    ///
    /// 口径：前块**原文**末 N 条（DEC-14：不带前块译文 —— 上文只帮模型理解
    /// 指代/人称，不参与前缀内容；带译文会破坏 KV 复用且引入顺序依赖）。
    /// 空表 = 不启用上下文（与既有契约逐字节一致）。
    std::vector<std::string> context_lines;
};

/// SubtitleTranslationOutcome —— 单行译文的结果。
/// schema_version: cinema_engine.subtitle_translation.outcome.v1
///
/// ok=false 时 text 为空、reason 非空。**调用方必须检查 ok**：
/// 实测小模型有 2%~6% 的样本直接回吐英文原文
/// （"desk"、"paranoid and reactionary"、"Asset on board. En route."），
/// 这样的结果绝不能当字幕显示出去。
///
/// 判定「是否回吐英文」用 subtitle_translation_prompt.hpp 里的
/// subtitle_translation_has_cjk() —— 译文里一个汉字都没有 ⇒ 失败。
struct SubtitleTranslationOutcome
{
    std::string schema_version{"cinema_engine.subtitle_translation.outcome.v1"};

    /// 清洗后的译文。ok=true 时非空。
    std::string text;

    /// 本行是否可用。
    bool ok{true};

    /// 失败原因。ok=false 时**必须非空**（G9 要求的口径）。
    std::string reason;
};

/// SubtitleTranslationBatch —— 一次块翻译的整体结果。
/// schema_version: cinema_engine.subtitle_translation.batch.v1
///
/// count_matched=false 表示**行数对不上**（模型静默漏行或串行）。
/// 此时 outcomes 的下标与 source_lines **可能已经错位**。
///
/// 调用方必须整块回退到逐行模式，**不能就地凑数**：
/// 错的字幕比没有字幕糟糕 —— 错位不会报错，只会静默给出错的字幕。
///
/// 实测：0.6B 在 20 行/块时把 19 行全翻对了、编号也连续，
/// 但**漏了一行** ⇒ 后面 6 行全部错位，而编号保护不了（模型会重新连续编号）。
/// 1.7B 同样条件下回退率为 0%。
struct SubtitleTranslationBatch
{
    std::string schema_version{"cinema_engine.subtitle_translation.batch.v1"};

    /// 逐行结果。长度应等于请求的 source_lines.size()；
    /// count_matched=false 时允许出现缺失槽位（对应行被判失败）。
    std::vector<SubtitleTranslationOutcome> outcomes;

    /// 模型是否产出了与请求行数一致的编号。
    bool count_matched{false};
};

/// SubtitleTranslationTimedLine —— 带时间戳的一条字幕。
/// schema_version: cinema_engine.subtitle_translation.timed_line.v1
///
/// 时间戳沿用仓库既有口径：毫秒、int64。
/// 与既有 SubtitleEntry 的 start_time_ms / end_time_ms 同义。
struct SubtitleTranslationTimedLine
{
    std::string schema_version{"cinema_engine.subtitle_translation.timed_line.v1"};

    int64_t start_time_ms{0};
    int64_t end_time_ms{0};

    /// 译文文本。写 SRT 时空串的行会被跳过。
    std::string text;
};

}  // namespace kivo::cinema_engine
