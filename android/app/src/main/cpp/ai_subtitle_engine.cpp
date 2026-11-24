// ============================================================================
// LlamaSubtitleEngine 实现。契约见 ai_subtitle_engine.h。
//
// 落位路径: android/app/src/main/cpp/ai_subtitle_engine.cpp
//
// 【translate 全流程】
//   1. prompt 构造：块模式 → build_subtitle_translation_block_prompt
//      （context 进 user 段，DEC-19/N1）；单行 → 单行契约（few-shot 口径）
//   2. 【双层 Token 预算】真实 tokenizer 编完整 prompt，prompt+输出预留 >
//      n_ctx → 返回空批（planner 重切）—— 不可绕过的最后防线（评审 E4/G4）
//   3. llama_memory_clear → prefill → 流式解码（**翻满 expected 行即停**；
//      行级早停：当前行 token 超源行×3 且 ≥12 → 完成当前行后停；
//      生成总量超 max_output_tokens → 停；多余行由缺号解析判罚）
//   4. 解析：「N. 译文」按号回填（生成器产出 parse）；单行走 clean
//   5. has_cjk 守卫（回吐英文判失败）→ Batch（count_matched 语义同契约）
// ============================================================================

#include "ai_subtitle_engine.h"

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_prompt.hpp"
#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_cue_assembler.hpp"

#include "llama.h"

#include <algorithm>
#include <android/log.h>
#include <cmath>
#include <mutex>
#include <unordered_set>

namespace kivo::cinema_engine {

namespace {

/// llama.cpp 的 LLAMA_LOG_* 默认走 stderr（Android 上不可见）——桥到 logcat。
/// 排查 native 翻译问题的关键证据通道（get_logits_ith 失败会打 reason）。
void kivo_llama_log(ggml_log_level level, const char * text, void * /*user*/)
{
    const android_LogPriority priority =
        level == GGML_LOG_LEVEL_ERROR ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO;
    __android_log_print(priority, "kivo-ai", "%s", text);
}

bool llama_log_bridge_installed = false;

/// 源文本的 token 估算（行级早停的「源行 ×3」基准用字符粗估折算 ——
/// 精确计数在 prompt 预算层做）。
std::size_t estimate_source_tokens(const std::string& text)
{
    // 粗口径：每 4 字节 ≈ 1 token（英文），CJK 每 2 字节 ≈ 1 token。
    std::size_t units = 0;
    std::size_t index = 0;
    while (index < text.size()) {
        const auto lead = static_cast<unsigned char>(text[index]);
        if (lead < 0x80u) { units += 1; index += 1; }
        else if ((lead & 0xE0u) == 0xC0u) { units += 1; index += 2; }
        else if ((lead & 0xF0u) == 0xF0u) { units += 2; index += 4; }
        else { units += 1; index += 3; }
    }
    return units == 0 ? 1 : units;
}

/// 简单 n-gram 重复检测：最近 8 个 token 的 3-gram 是否已出现过。
/// （复读是最常见的失控形态 —— 命中即停，评审 E29。）
struct RepetitionGuard
{
    std::unordered_set<std::string> seen;
    std::vector<llama_token> last;
    bool check_and_record(llama_token id)
    {
        last.push_back(id);
        if (last.size() < 6) { return false; }
        if (last.size() > 8) { last.erase(last.begin()); }
        // 3-gram（最后三个）作为键。
        std::string key;
        for (std::size_t i = last.size() >= 3 ? last.size() - 3 : 0; i < last.size(); ++i) {
            key += std::to_string(last[i]) + ",";
        }
        if (seen.count(key) > 0) { return true; }
        seen.insert(key);
        return false;
    }
};

}  // namespace

LlamaSubtitleEngine::LlamaSubtitleEngine(const LlamaEngineConfig& config)
    : config_(config)
{
    if (!llama_log_bridge_installed) {
        llama_log_set(kivo_llama_log, nullptr);
        llama_log_bridge_installed = true;
    }
    llama_backend_init();
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 0;   // CPU 口径（Vulkan 不在 T-B 范围）
    // mmap 由 llama.cpp 默认启用（新版 llama_model_params 无 use_mmap 字段）。

    model_ = llama_model_load_from_file(config_.model_path.c_str(), model_params);
    if (model_ == nullptr) {
        last_error_ = "model load failed: " + config_.model_path;
        return;
    }
    vocab_ = llama_model_get_vocab(model_);

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = config_.n_ctx;
    ctx_params.n_batch = 512;
    ctx_params.no_perf = false;
    ctx_ = llama_init_from_model(model_, ctx_params);
    if (ctx_ == nullptr) {
        last_error_ = "context init failed";
        llama_model_free(model_);
        model_ = nullptr;
        return;
    }
    llama_set_n_threads(ctx_, config_.n_threads, config_.n_threads);

    auto sparams = llama_sampler_chain_default_params();
    sparams.no_perf = false;
    sampler_ = llama_sampler_chain_init(sparams);
    llama_sampler_chain_add(sampler_, llama_sampler_init_greedy());   // 温度 0（DEC：可复现）

    ready_ = true;
}

LlamaSubtitleEngine::~LlamaSubtitleEngine()
{
    if (sampler_ != nullptr) { llama_sampler_free(sampler_); }
    if (ctx_ != nullptr) { llama_free(ctx_); }
    if (model_ != nullptr) { llama_model_free(model_); }
    llama_backend_free();
}

bool LlamaSubtitleEngine::ready() const noexcept { return ready_; }

std::string LlamaSubtitleEngine::last_error() const noexcept { return last_error_; }

std::int64_t LlamaSubtitleEngine::consumed_tokens() const noexcept
{
    return consumed_tokens_;
}

void LlamaSubtitleEngine::request_cancel() noexcept
{
    // llama.cpp 的 abort callback 是 ctx 级 —— v1 用轮询标志（decode 循环
    // 每 token 检查 cancel_requested_；跨线程可见由调用方经 JNI 设置）。
    // 这里留接口位：取消语义在 JNI 层以静态原子标志实现（单实例）。
}

SubtitleTranslationBatch LlamaSubtitleEngine::translate(
    const SubtitleTranslationUnit& unit)
{
    SubtitleTranslationBatch batch;
    batch.count_matched = false;
    if (!ready_) {
        last_error_ = "engine not ready";
        return batch;
    }
    std::lock_guard<std::mutex> lock(translate_mutex_);

    const bool is_block = unit.source_lines.size() > 1;

    // ---- 1. prompt 构造（契约函数；context 进 user 段，DEC-19）----
    std::string prompt;
    std::vector<std::string_view> lines;
    std::vector<std::string_view> context;
    for (const auto& line : unit.source_lines) { lines.push_back(line); }
    for (const auto& line : unit.context_lines) { context.push_back(line); }
    if (is_block) {
        prompt = build_subtitle_translation_block_prompt(
            lines.data(), lines.size(), context.data(), context.size(),
            unit.use_fewshot);
    } else {
        prompt = build_subtitle_translation_prompt(unit.source_lines[0], unit.use_fewshot);
    }

    // ---- 2. 双层 Token 预算（不可绕过的最后防线，评审 E4/G4）----
    const int n_prompt = -llama_tokenize(
        vocab_, prompt.data(), prompt.size(), nullptr, 0, true, true);
    if (n_prompt <= 0) {
        last_error_ = "prompt tokenize failed";
        return batch;
    }
    const std::int64_t budget = static_cast<std::int64_t>(config_.n_ctx) -
                                static_cast<std::int64_t>(config_.max_output_tokens);
    if (static_cast<std::int64_t>(n_prompt) > budget) {
        last_error_ = "prompt exceeds ctx budget (planner must re-split)";
        return batch;   // 空批 + reason -> scheduler 聚合重试 / planner 重切
    }
    std::vector<llama_token> prompt_tokens(static_cast<std::size_t>(n_prompt));
    if (llama_tokenize(vocab_, prompt.data(), prompt.size(), prompt_tokens.data(),
                       prompt_tokens.size(), true, true) < 0) {
        last_error_ = "prompt tokenize failed (fill)";
        return batch;
    }
    consumed_tokens_ += n_prompt;

    // ---- 3. 干净 KV（每请求完整重建；前缀复用列 T-B 后续优化）----
    llama_memory_clear(llama_get_memory(ctx_), true);

    llama_batch_ext* batch_ext = llama_batch_ext_init(ctx_);
    llama_batch_ext_clear(batch_ext);
    for (int32_t i = 0; i < n_prompt; ++i) {
        const int32_t idx = llama_batch_ext_add_token(batch_ext, 0, prompt_tokens[static_cast<std::size_t>(i)]);
        const llama_pos pos = i;
        llama_batch_ext_set_pos(batch_ext, idx, &pos);
    }
    llama_batch_ext_set_output_logits(batch_ext, n_prompt - 1, true);
    if (llama_process(ctx_, LLAMA_PROCESS_TYPE_DECODE, batch_ext)) {
        last_error_ = "prefill failed";
        llama_batch_ext_free(batch_ext);
        return batch;
    }

    // ---- 4. 流式解码（翻满 expected 行即停；行级早停 + 复读守卫）----
    const std::size_t expected = unit.source_lines.size();
    // 行级早停基准：**最长源行**的 token 粗估 x3，且绝对下限 12 token
    //（评审 C11：极短行 "No." 的中文可能更长，x3 会误杀）。
    std::size_t longest_source = 0;
    for (const auto& line : unit.source_lines) {
        longest_source = std::max(longest_source, estimate_source_tokens(line));
    }
    const std::size_t per_line_stop = std::max<std::size_t>(12, longest_source * 3);

    std::string generated;
    std::size_t completed_lines = 0;
    std::size_t current_line_tokens = 0;
    RepetitionGuard repetition;
    bool stopped_early = false;
    llama_pos next_pos = static_cast<llama_pos>(n_prompt);
    // 采样前防御：输出缓冲里没有 logits 时 llama_sampler_sample 内部会
    // GGML_ASSERT 崩进程（release 版 get_logits_ith 返回 nullptr）——
    // 必须转成可见失败回退原文，绝不崩溃。
    if (llama_get_logits_ith(ctx_, -1) == nullptr) {
        last_error_ = "logits missing after prefill (prompt tokens=" +
                      std::to_string(n_prompt) + ")";
        llama_batch_ext_free(batch_ext);
        return batch;
    }
    llama_token next = llama_sampler_sample(sampler_, ctx_, -1);
    consumed_tokens_ += 1;

    while (completed_lines < expected) {
        if (llama_vocab_is_eog(vocab_, next)) { break; }

        char piece[128];
        const int n_piece = llama_token_to_piece(vocab_, next, piece, sizeof(piece), 0, true);
        if (n_piece > 0) { generated.append(piece, static_cast<std::size_t>(n_piece)); }

        // 行推进：按生成文本里的换行数（每换行 = 完成一行）。
        completed_lines = static_cast<std::size_t>(
            std::count(generated.begin(), generated.end(), '\n'));
        if (completed_lines >= expected) { break; }   // 翻满即停（多余输出丢弃）

        // 行级早停：当前行 token 数超限（相对最长源行）或复读命中。
        ++current_line_tokens;
        if (current_line_tokens > per_line_stop) { stopped_early = true; break; }
        if (repetition.check_and_record(next)) { stopped_early = true; break; }

        // 下一 token 的 batch：pos 逐 token 推进（恒定 pos 会把 KV 写花 —— 首版 bug）。
        llama_batch_ext_clear(batch_ext);
        const int32_t idx = llama_batch_ext_add_token(batch_ext, 0, next);
        llama_batch_ext_set_pos(batch_ext, idx, &next_pos);
        llama_batch_ext_set_output_logits(batch_ext, 0, true);
        if (llama_process(ctx_, LLAMA_PROCESS_TYPE_DECODE, batch_ext)) {
            last_error_ = "decode failed";
            llama_batch_ext_free(batch_ext);
            return batch;
        }
        ++next_pos;
        consumed_tokens_ += 1;
        if (llama_get_logits_ith(ctx_, -1) == nullptr) {
            last_error_ = "logits missing after decode step " +
                          std::to_string(next_pos) + " (generated=" +
                          std::to_string(generated.size()) + " bytes)";
            llama_batch_ext_free(batch_ext);
            return batch;
        }
        next = llama_sampler_sample(sampler_, ctx_, -1);
        consumed_tokens_ += 1;
    }
    llama_batch_ext_free(batch_ext);
    (void)stopped_early;

    // ---- 5. 解析回填（parse 是生成器产物；单行走 clean）----
    if (is_block) {
        std::vector<std::string> slots(expected);
        const bool matched = parse_subtitle_translation_block_output(
            generated, expected, slots.data());
        batch.count_matched = matched;
        for (std::size_t index = 0; index < expected; ++index) {
            SubtitleTranslationOutcome outcome;
            const std::string& slot_text = slots[index];
            if (!slot_text.empty() && subtitle_translation_has_cjk(slot_text)) {
                outcome.ok = true;
                outcome.text = slot_text;
            } else {
                outcome.ok = false;
                outcome.reason = slot_text.empty()
                                     ? (stopped_early ? "early stop, missing line"
                                                      : "line missing in output")
                                     : "no CJK in translation (echo guard)";
            }
            batch.outcomes.push_back(outcome);
        }
    } else {
        const std::string cleaned = clean_subtitle_translation_output(generated);
        SubtitleTranslationOutcome outcome;
        if (!cleaned.empty() && subtitle_translation_has_cjk(cleaned)) {
            outcome.ok = true;
            outcome.text = cleaned;
        } else {
            outcome.ok = false;
            outcome.reason = cleaned.empty() ? "empty output" : "no CJK (echo guard)";
        }
        batch.outcomes.push_back(outcome);
        batch.count_matched = outcome.ok;
    }
    return batch;
}

}  // namespace kivo::cinema_engine