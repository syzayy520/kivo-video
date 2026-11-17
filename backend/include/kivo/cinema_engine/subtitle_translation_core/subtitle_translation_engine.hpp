#pragma once

// ============================================================================
// 字幕翻译引擎的接口契约。
//
// 落位路径: backend/include/kivo/cinema_engine/subtitle_translation_core/
//            subtitle_translation_engine.hpp
//
// 【这份文件是「缝」】
//   一侧是已经实现好的部分：prompt 契约、译文写出、类型定义（都在本目录）。
//   另一侧是要新写的部分：真正调用推理内核的那一层（llama.cpp 封装、
//   批处理调度、KV 缓存复用、术语表、云端兜底）。
//   本文件只描述两侧之间的约定，**不规定实现**。
//
// 【为什么零平台依赖】
//   与本目录其余文件一致：不含 Android 头，不含 llama.cpp 头。
//   推理内核怎么实现（llama.cpp / 别的引擎 / 云端 HTTP）由实现方决定，
//   但**块/行的粒度语义、行数校验、回吐英文判定**这些与产品行为直接相关的
//   规则由本目录统一，避免在实现里各写一套。
//
// 【实现方必须满足的三条产品规则】
//   ① 块翻译必须做**行数校验**：模型静默漏行会让后续行整体错位，
//      而编号保护不了（模型会重新连续编号）。行数对不上时
//      SubtitleTranslationBatch::count_matched 必须为 false。
//   ② 单行结果必须过 **CJK 守卫**：不含任何中日韩表意文字的输出判为失败
//      （实测小模型有 2%~6% 的样本直接回吐英文原文）。
//   ③ 单行翻译必须**复用 system prompt 的 KV 前缀**，或改走块翻译。
//      逐行模式下 system prompt（约 150 token）被每行重复 prefill，
//      1 小时视频（1000 行）要 prefill 17 万 token —— K40 实测 68 tok/s，
//      即 42 分钟，功能等于没有。块模式（20 行/块）摊薄到 24 token/行。
// ============================================================================

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_types.hpp"

namespace kivo::cinema_engine {

/// SubtitleTranslationEngine —— 字幕翻译引擎。
///
/// 生命周期与线程模型由实现方定；**调用方不得假设任何并发语义**，
/// 跨线程调用同一实例的安全性由实现方在文档里写明。
class SubtitleTranslationEngine
{
public:
    virtual ~SubtitleTranslationEngine() = default;

    /// 翻译一个翻译单元（单行或一块）。
    ///
    /// 语义：
    ///   - source_lines.size() == 1 → 单行翻译
    ///   - source_lines.size() >  1 → 块翻译
    ///
    /// 返回：
    ///   - 单行：outcomes.size() == 1
    ///   - 块  ：outcomes.size() == source_lines.size()
    ///
    /// 实现方必须保证：
    ///   - 块模式下若模型产出的行数与请求不一致，count_matched 置 false，
    ///     且 outcomes 里未能确认对应的槽位保持 ok=false / text 为空，
    ///     **不要就地凑数**（凑出来的会错位，而错位的字幕比没有字幕糟糕）。
    ///   - 每条 outcome 的 ok=false 时 reason 必须非空。
    ///
    /// @param unit  待翻译的单元。
    /// @return       逐行结果。实现失败时返回 count_matched=false 的空 batch。
    virtual SubtitleTranslationBatch translate(const SubtitleTranslationUnit& unit) = 0;

    /// 引擎是否已就绪（模型是否加载成功）。
    ///
    /// 用于 UI 灰显：模型没下载 / 没加载完时，「AI 字幕」开关应该不可用。
    virtual bool ready() const noexcept = 0;

    /// 人类可读的失败原因。ready() 为 false 时必须非空。
    virtual std::string last_error() const noexcept = 0;

    /// 累计 token 用量（本地推理也有成本：电量、发热）。
    ///
    /// SettingsAiSub 那一页已经有「翻译用量」的位置，这里给它数据源。
    /// 云端兜底时由实现方把云端用量一并累加进来。
    virtual std::int64_t consumed_tokens() const noexcept = 0;
};

}  // namespace kivo::cinema_engine
