// ============================================================================
// kivo_ai_spike —— T-0 引擎压力冒烟（独立 NDK 可执行，绕开 JNI/APK）。
//
// 落位路径: third_party/llama.cpp-android/spike/spike_main.cpp
//
// 采集项（设计 v3.2 §4 T-0）：
//   - n_threads 扫描（argv 逗号列表，单次模型加载逐档重设 llama_set_n_threads）
//   - 每档 tok/s + 耗时（decode 口径，ggml_time_us）
//   - cpuset 采样（/proc/self/task/<tid>/cpuset —— 验证线程落在哪簇核，
//     评审 A2：BACKGROUND 优先级可能锁小核）
//   - 温度（/sys/class/thermal/thermal_zone*/temp，每档前后打点）
//   - 内存（/proc/self/status VmRSS/VmHWM）
//
// 不采集（由后续批次承接）：prefill/decode 拆分（T-B BlockStats）、
// sub-reload（T-C）、真实字幕工作负载（T-B 后）。
// ============================================================================

#include "llama.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

std::string read_first_line(const char* path) {
    FILE* f = fopen(path, "r");
    if (f == nullptr) { return ""; }
    char buf[256] = {};
    const size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    std::string out(buf, n);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == ' ')) {
        out.pop_back();
    }
    return out;
}

void print_thermal() {
    // 扫描 thermal_zone 前 12 个：type + 毫摄氏度。
    for (int zone = 0; zone < 12; ++zone) {
        char type_path[128];
        char temp_path[128];
        snprintf(type_path, sizeof(type_path),
                 "/sys/class/thermal/thermal_zone%d/type", zone);
        snprintf(temp_path, sizeof(temp_path),
                 "/sys/class/thermal/thermal_zone%d/temp", zone);
        const std::string type = read_first_line(type_path);
        const std::string temp = read_first_line(temp_path);
        if (type.empty() || temp.empty()) { continue; }
        fprintf(stderr, "[thermal] %s = %s\n", type.c_str(), temp.c_str());
    }
}

void print_cpu_affinity() {
    // 主线程 cpuset：确认落在哪簇核（A77 大核簇 / A55 小核簇）。
    const std::string cpuset = read_first_line("/proc/self/task/self/cpuset");
    const std::string allowed = read_first_line("/proc/self/status");
    fprintf(stderr, "[affinity] cpuset=%s\n", cpuset.c_str());
    // Cpus_allowed_list 从 status 里抠。
    FILE* f = fopen("/proc/self/status", "r");
    if (f != nullptr) {
        char line[256];
        while (fgets(line, sizeof(line), f) != nullptr) {
            if (strncmp(line, "Cpus_allowed_list", 17) == 0) {
                fprintf(stderr, "[affinity] %s", line);
                break;
            }
        }
        fclose(f);
    }
    (void)allowed;
}

void print_memory() {
    FILE* f = fopen("/proc/self/status", "r");
    if (f == nullptr) { return; }
    char line[256];
    while (fgets(line, sizeof(line), f) != nullptr) {
        if (strncmp(line, "VmRSS", 5) == 0 || strncmp(line, "VmHWM", 5) == 0) {
            fprintf(stderr, "[mem] %s", line);
        }
    }
    fclose(f);
}

std::vector<int> parse_thread_list(const char* spec) {
    std::vector<int> out;
    std::string cur;
    for (const char* p = spec; ; ++p) {
        if (*p == ',' || *p == '\0') {
            if (!cur.empty()) { out.push_back(atoi(cur.c_str())); cur.clear(); }
            if (*p == '\0') { break; }
            continue;
        }
        cur.push_back(*p);
    }
    if (out.empty()) { out.push_back(4); }
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    std::string model_path;
    std::string thread_spec = "4";
    int n_predict = 64;
    int min_seconds = 0;   // >0 = 长跑模式：decode 至少跑满该秒数（T-0 热平台期）
    std::string prompt =
        "English subtitle:\n"
        "I thought you were leaving.\n\n"
        "Output the Chinese subtitle only:\n";

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) { model_path = argv[++i]; }
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) { thread_spec = argv[++i]; }
        else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) { n_predict = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) { min_seconds = atoi(argv[++i]); }
        else { prompt = argv[i]; }
    }
    if (model_path.empty()) {
        fprintf(stderr, "usage: kivo_ai_spike -m model.gguf [-t 4,2,6] [-n 64] [prompt]\n");
        return 2;
    }

    fprintf(stderr, "[spike] model=%s threads=[%s] n_predict=%d\n",
            model_path.c_str(), thread_spec.c_str(), n_predict);
    print_cpu_affinity();

    llama_backend_init();

    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 0;   // CPU 口径（Vulkan 不在 T-0 范围）
    llama_model* model = llama_model_load_from_file(model_path.c_str(), model_params);
    if (model == nullptr) {
        fprintf(stderr, "[spike] FAIL model load\n");
        return 1;
    }
    const llama_vocab* vocab = llama_model_get_vocab(model);

    const int n_prompt = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), nullptr, 0, true, true);
    std::vector<llama_token> prompt_tokens(n_prompt);
    if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(),
                       prompt_tokens.size(), true, true) < 0) {
        fprintf(stderr, "[spike] FAIL tokenize\n");
        return 1;
    }

    llama_context_params ctx_params = llama_context_default_params();
    // 长跑模式下 decode token 数远超 n_predict —— n_ctx 按秒数估算
    // （热稳态 ~11 tok/s 上限 × 1.5 余量），避免 pos 越界 llama_process 失败。
    const std::int64_t n_ctx_tokens =
        static_cast<std::int64_t>(n_prompt) +
        (min_seconds > 0 ? static_cast<std::int64_t>(min_seconds) * 20 : n_predict) + 256;
    ctx_params.n_ctx = static_cast<uint32_t>(n_ctx_tokens);
    ctx_params.n_batch = static_cast<uint32_t>(n_prompt);
    ctx_params.no_perf = false;
    llama_context* ctx = llama_init_from_model(model, ctx_params);
    if (ctx == nullptr) {
        fprintf(stderr, "[spike] FAIL ctx\n");
        return 1;
    }

    auto sparams = llama_sampler_chain_default_params();
    sparams.no_perf = false;
    llama_sampler* smpl = llama_sampler_chain_init(sparams);
    llama_sampler_chain_add(smpl, llama_sampler_init_greedy());

    llama_batch_ext* batch = llama_batch_ext_init(ctx);
    print_thermal();
    print_memory();

    // ---- 矩阵：单次模型加载，逐档 llama_set_n_threads ----
    for (const int threads : parse_thread_list(thread_spec.c_str())) {
        llama_set_n_threads(ctx, threads, threads);
        // 【每档清 KV】上一档的 decode 已把 pos 推到尾部，重复 prefill 的
        // pos 0 会被拒（实测 FAIL prefill）—— 矩阵每档从干净 KV 起。
        llama_memory_clear(llama_get_memory(ctx), true);
        // Prefill（prompt 一次性）。
        llama_batch_ext_clear(batch);
        for (int32_t i = 0; i < n_prompt; ++i) {
            const int32_t idx = llama_batch_ext_add_token(batch, 0, prompt_tokens[i]);
            const llama_pos pos = i;
            llama_batch_ext_set_pos(batch, idx, &pos);
        }
        llama_batch_ext_set_output_logits(batch, n_prompt - 1, true);
        const auto t_prefill_0 = ggml_time_us();
        if (llama_process(ctx, LLAMA_PROCESS_TYPE_DECODE, batch)) {
            fprintf(stderr, "[spike] FAIL prefill\n");
            return 1;
        }
        const auto t_prefill_1 = ggml_time_us();

        // Decode：min_seconds>0 = 长跑模式（跑满秒数，n_predict 无效——
        // T-0 热平台期采集）；每 500 token 一段采样（分段 tok/s 让热降频曲线可见）。
        llama_token next = llama_sampler_sample(smpl, ctx, -1);
        int n_decode = 0;
        std::int64_t t_decode_0 = ggml_time_us();
        std::int64_t t_probe_0 = ggml_time_us();
        int last_probe_token = 0;
        while (true) {
            const auto now_us = ggml_time_us();
            const double elapsed = (now_us - t_decode_0) / 1000000.0;
            if (min_seconds > 0) {
                if (elapsed >= min_seconds) { break; }
            } else if (n_decode >= n_predict) {
                break;
            }
            if (llama_vocab_is_eog(vocab, next)) { break; }
            llama_batch_ext_clear(batch);
            const int32_t idx = llama_batch_ext_add_token(batch, 0, next);
            const llama_pos pos = n_prompt + n_decode;
            llama_batch_ext_set_pos(batch, idx, &pos);
            // 【必标 logits】sample 从 batch 的 logits 读 —— simple.cpp 每轮
            // set_output_logits，漏了就是真机 SIGABRT（T-0 spike 实测）。
            llama_batch_ext_set_output_logits(batch, 0, true);
            if (llama_process(ctx, LLAMA_PROCESS_TYPE_DECODE, batch)) {
                fprintf(stderr, "[spike] FAIL decode\n");
                return 1;
            }
            ++n_decode;
            next = llama_sampler_sample(smpl, ctx, -1);
            if (llama_vocab_is_eog(vocab, next)) { break; }
            // 分段采样：每 500 token 打段速 + 温度 + 内存。
            if (n_decode - last_probe_token >= 500) {
                const double seg_s = (ggml_time_us() - t_probe_0) / 1000000.0;
                fprintf(stderr,
                        "[SEG] at=%d tok seg_tok_per_s=%.2f elapsed=%.1fs\n",
                        n_decode, (n_decode - last_probe_token) / seg_s, elapsed);
                print_thermal();
                print_memory();
                last_probe_token = n_decode;
                t_probe_0 = ggml_time_us();
            }
        }
        const auto t_decode_1 = ggml_time_us();
        const double decode_s = (t_decode_1 - t_decode_0) / 1000000.0;
        const double prefill_s = (t_prefill_1 - t_prefill_0) / 1000000.0;

        fprintf(stderr,
                "[RESULT] threads=%d prefill=%.2fs decode=%.2fs tok_per_s=%.2f (n=%d)\n",
                threads, prefill_s, decode_s,
                n_decode > 0 ? n_decode / decode_s : 0.0, n_decode);
        print_thermal();
        print_memory();
    }

    llama_batch_ext_free(batch);
    llama_sampler_free(smpl);
    llama_free(ctx);
    llama_model_free(model);
    fprintf(stderr, "[spike] done\n");
    return 0;
}