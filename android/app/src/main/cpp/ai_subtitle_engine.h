#pragma once

// ============================================================================
// LlamaSubtitleEngine —— llama.cpp 封装（T-B 核心，API 形态已经 T-0 spike 验证）。
//
// 落位路径: android/app/src/main/cpp/ai_subtitle_engine.h
// 链接: llama_ext（IMPORTED 静态库）+ kivo_core_subtitle_translation（prompt 契约）
//
// 【T-0 spike 验证过的 API 形态，不许再猜】
//   - llama_model_load_from_file / llama_init_from_model / llama_get_vocab
//   - llama_batch_ext_* 系列；decode 单 token batch **必须 set_output_logits**
//     （漏标 = sample 读不到 logits → SIGABRT，spike 实测）
//   - llama_set_n_threads(ctx, gen, batch)（动态换线程；矩阵实测 n_threads=4 最优）
//   - 矩阵/每请求间 llama_memory_clear（重复 prefill 的 pos 0 被拒，spike 实测）
//
// 【n_ctx 预算口径（T-0 长跑数据）】
//   KV 预分配 = n_ctx × ~102KB/token（Qwen3-1.7B）——n_ctx 按块预算（2048 →
//   KV ≈ 230MB），**不按长跑时长**（T-0 长跑 n_ctx=18256 → RSS 3.2GB 教训）。
//
// 【 KV 复用现状（如实标注）】
//   v1 每请求 llama_memory_clear 完整重建 —— 正确性优先；前缀 KV 复用
//   （system+few-shot prefill 一次、每块 seq_rm 截回）列 T-B 后续优化，
//   届时以「任意两块共享前缀字节」断言验证（N1）。
// ============================================================================

#include <cstdint>
#include <mutex>
#include <string>

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_engine.hpp"
#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_types.hpp"

// llama.cpp 的不透明句柄 —— 前向声明避免在头文件里 include llama.h
// （llama.h 只在实现文件里 include，污染面收窄）。
struct llama_model;
struct llama_context;
struct llama_sampler;
struct llama_vocab;

namespace kivo::cinema_engine {

struct LlamaEngineConfig
{
    std::string model_path;
    int n_threads{4};                 ///< DEC-18：T-0 矩阵实测 4 最优（6 更慢且更热）。
    std::uint32_t n_ctx{2048};        ///< 按块预算（KV ≈ 230MB，T-0 长跑口径见头注）。
    std::size_t max_output_tokens{256}; ///< 单次生成硬上限（防复读烧 ctx，评审 E30）。
};

/// llama.cpp 的 SubtitleTranslationEngine 实现。
/// 线程模型：**单实例单翻译线程**（调用方保证）；cancel 标志可跨线程设置（原子）。
class LlamaSubtitleEngine final : public SubtitleTranslationEngine
{
public:
    explicit LlamaSubtitleEngine(const LlamaEngineConfig& config);
    ~LlamaSubtitleEngine() override;

    LlamaSubtitleEngine(const LlamaSubtitleEngine&) = delete;
    LlamaSubtitleEngine& operator=(const LlamaSubtitleEngine&) = delete;

    SubtitleTranslationBatch translate(const SubtitleTranslationUnit& unit) override;
    bool ready() const noexcept override;
    std::string last_error() const noexcept override;
    std::int64_t consumed_tokens() const noexcept override;

    /// 跨线程取消：置原子标志，decode 循环每 token 检查（评审 E29/G32——
    /// 退出播放页应在 1 秒内响应）。
    void request_cancel() noexcept;

private:
    LlamaEngineConfig config_;
    llama_model* model_{nullptr};
    llama_context* ctx_{nullptr};
    const llama_vocab* vocab_{nullptr};
    llama_sampler* sampler_{nullptr};
    std::mutex translate_mutex_;      ///< 单实例防并发（单翻译线程约定的守门）。
    std::string last_error_;
    std::int64_t consumed_tokens_{0};
    bool ready_{false};
};

}  // namespace kivo::cinema_engine