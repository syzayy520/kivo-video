// ============================================================================
// AI 翻译引擎的 JNI 桥（libkivo_ai.so，独立故障域 —— 评审 C1/E3：加载期隔离）。
//
// 落位路径: android/app/src/main/cpp/AiTranslationNative.cpp
//
// 【口径对齐 README §5.1 + v3.2 扩展】
//   - nativeTranslateBlock 返回数组长度 == 输入行数；失败槽位 = **空串**
//     （不返回 null 元素 —— to_string_vector 跳过 null 会让 Kotlin 下标错位）；
//   - context_lines 随 translate 传入（DEC-19：进 user 段保前缀 KV）；
//   - v1 偏差如实登记：nativeCancel 在同步调用期间不中断推理（mutex 单
//     翻译线程），decode 内中断（abort callback）列 T-B 后续；WriteSrt 由
//     Kotlin 侧实现（格式化简单，不占 JNI 面）。
//
// 【单例 + mutex】单实例单翻译线程（调用方约定）；mutex 防跨线程并发进入。
// ============================================================================

#include <jni.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ai_subtitle_engine.h"
#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_cue_assembler.hpp"

namespace {

std::mutex g_engine_mutex;
std::unique_ptr<kivo::cinema_engine::LlamaSubtitleEngine> g_engine;
std::string g_last_error;
std::atomic<bool> g_cancel_requested{false};

std::string to_std_string(JNIEnv* env, jstring value)
{
    if (value == nullptr) { return {}; }
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (chars == nullptr) { return {}; }
    std::string out(chars);
    env->ReleaseStringUTFChars(value, chars);
    return out;
}

std::vector<std::string> to_std_strings(JNIEnv* env, jobjectArray array)
{
    std::vector<std::string> out;
    if (array == nullptr) { return out; }
    const jsize count = env->GetArrayLength(array);
    out.reserve(static_cast<std::size_t>(count));
    for (jsize index = 0; index < count; ++index) {
        const auto element = static_cast<jstring>(
            env->GetObjectArrayElement(array, index));
        out.push_back(to_std_string(env, element));
        if (element != nullptr) { env->DeleteLocalRef(element); }
    }
    return out;
}

jobjectArray from_std_strings(JNIEnv* env, const std::vector<std::string>& values)
{
    const auto string_class = static_cast<jclass>(
        env->FindClass("java/lang/String"));
    jobjectArray out = env->NewObjectArray(
        static_cast<jsize>(values.size()), string_class, nullptr);
    for (jsize index = 0; index < static_cast<jsize>(values.size()); ++index) {
        env->SetObjectArrayElement(out, index,
                                   env->NewStringUTF(values[index].c_str()));
    }
    env->DeleteLocalRef(string_class);
    return out;
}

}  // namespace

// nativeParseCues —— SRT 文本 → 归一 cue 的平铺数组（cue_assembler 权威实现）。
// 返回 String[]，每个元素 = "<startMs>	<endMs>	<text>"（text 已 strip 标签与
// tab；单行）。Kotlin 侧 split 三段。避免在 Kotlin 侧重复解析（偏差累积防线）。
extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_kivo_cinema_subtitle_AiTranslationNative_nativeParseCues(
    JNIEnv* env, jclass, jstring srt_utf8)
{
    const std::string srt = to_std_string(env, srt_utf8);
    const auto assembly = kivo::cinema_engine::assemble_subtitle_translation_cues(srt, nullptr);
    std::vector<std::string> rows;
    rows.reserve(assembly.cues.size());
    for (const auto& cue : assembly.cues) {
        std::string row = std::to_string(cue.start_time_ms);
        row.push_back('	');
        row += std::to_string(cue.end_time_ms);
        row.push_back('	');
        row += cue.text;
        rows.push_back(std::move(row));
    }
    return from_std_strings(env, rows);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_kivo_cinema_subtitle_AiTranslationNative_nativeLoadModel(
    JNIEnv* env, jclass, jstring model_path)
{
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    const std::string path = to_std_string(env, model_path);
    if (path.empty()) {
        g_last_error = "model path is empty";
        return JNI_FALSE;
    }
    kivo::cinema_engine::LlamaEngineConfig config;
    config.model_path = path;
    config.n_threads = 4;     // DEC-18：T-0 矩阵实测 4 最优
    config.n_ctx = 2048;      // 按块预算（KV ≈ 230MB，T-0 长跑口径）
    g_engine = std::make_unique<kivo::cinema_engine::LlamaSubtitleEngine>(config);
    if (!g_engine->ready()) {
        g_last_error = g_engine->last_error();
        g_engine.reset();
        return JNI_FALSE;
    }
    g_last_error.clear();
    return JNI_TRUE;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_kivo_cinema_subtitle_AiTranslationNative_nativeTranslateBlock(
    JNIEnv* env, jclass, jobjectArray source_lines, jboolean use_fewshot,
    jobjectArray context_lines)
{
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (!g_engine || !g_engine->ready()) {
        return nullptr;   // 引擎未就绪 —— reason 走 nativeLastError()
    }
    const auto lines = to_std_strings(env, source_lines);
    const auto context = to_std_strings(env, context_lines);
    if (lines.empty()) { return from_std_strings(env, {}); }

    kivo::cinema_engine::SubtitleTranslationUnit unit;
    unit.source_lines = lines;
    unit.use_fewshot = use_fewshot == JNI_TRUE;
    unit.context_lines = context;

    const auto batch = g_engine->translate(unit);

    // 槽位口径（README §5.1）：长度必须等于输入行数；失败槽位空串。
    std::vector<std::string> slots(lines.size());
    for (std::size_t index = 0; index < lines.size() &&
                                index < batch.outcomes.size(); ++index) {
        const auto& outcome = batch.outcomes[index];
        slots[index] = (outcome.ok && !outcome.text.empty()) ? outcome.text : "";
    }
    g_last_error = batch.count_matched ? "" : "count mismatch (partial slots)";
    return from_std_strings(env, slots);
}

extern "C" JNIEXPORT void JNICALL
Java_com_kivo_cinema_subtitle_AiTranslationNative_nativeCancel(JNIEnv*, jclass)
{
    g_cancel_requested = true;   // v1：任务级取消（decode 内中断列后续）
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_kivo_cinema_subtitle_AiTranslationNative_nativeReady(JNIEnv*, jclass)
{
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    return (g_engine && g_engine->ready()) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_kivo_cinema_subtitle_AiTranslationNative_nativeLastError(JNIEnv* env, jclass)
{
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    return env->NewStringUTF(g_last_error.c_str());
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_kivo_cinema_subtitle_AiTranslationNative_nativeConsumedTokens(JNIEnv*, jclass)
{
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    return g_engine ? g_engine->consumed_tokens() : 0;
}