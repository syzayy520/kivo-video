#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_prompt.hpp"

using namespace kivo::cinema_engine;

// ============================================================================
// 字幕翻译 prompt 契约的漂移锁。
//
// 落位路径: backend/tests/subtitle_translation/subtitle_translation_prompt_test.cpp
//
// 【这道锁在防什么】
//   prompt 是桌面评测（Python）与手机端推理（C++）之间唯一的约定。
//   两边一旦不一致，评测结论全部作废，而且现场症状是「手机上字幕偶尔是英文」
//   这类偶发问题，极难定位。所以必须在**编译期**就把两边钉死。
//
//   做法：C++ 侧算出实际 prompt 的 FNV-1a 64 摘要，与**来自 Python 真源**
//   的期望摘要比对。摘要由 <toolchain>/eval/gen_cpp_header.py 写进
//   prompt_manifest.json，改 Python 真源必须重跑生成器并同步这里的常量 ——
//   两边不一致 ⇒ 这条测试红。
//
// 【为什么用 FNV-1a 而不是 SHA-256】
//   仓库 backend 里没有可用的 SHA-256 实现（只有 compliance 核里的 fake 字符串），
//   为一个「检测字符串有没有变」的锁引入完整密码学实现不划算。
//   这里防的是「改了真源忘重新生成」和「手改了生成物」，不是防篡改。
//
// 【本文件是纯 ASCII】
//   与生成的契约头同一理由：主库目标 kivo_cinema_engine 没有 /utf-8，
//   MSVC 会按系统代码页 936 解读源码并静默损坏中文。
//   这里不写任何中文字面量，期望值全是 ASCII 十六进制摘要。
// ============================================================================

namespace {

/// FNV-1a 64 位。与 <toolchain>/eval/gen_cpp_header.py::fnv1a64 必须一致。
std::uint64_t fnv1a64(std::string_view text) {
    std::uint64_t value = 0xcbf29ce484222325ULL;
    for (const char ch : text) {
        value ^= static_cast<std::uint64_t>(static_cast<unsigned char>(ch));
        value *= 0x100000001b3ULL;
    }
    return value;
}

std::string to_hex(std::uint64_t value) {
    static const char* digits = "0123456789abcdef";
    std::string out(16, '0');
    for (int index = 15; index >= 0; --index) {
        out[static_cast<std::size_t>(index)] = digits[value & 0xF];
        value >>= 4;
    }
    return out;
}

/// 来自 <toolchain>/eval/generated/prompt_manifest.json 的期望摘要。
/// 改 Python 真源 → 重跑生成器 → 同步这三个常量。
constexpr const char* kExpectedSystemPrompt = "fd02f9ebd70db0e1";
constexpr const char* kExpectedPromptSingle = "b4969d0b8e8f3a37";
constexpr const char* kExpectedPromptFewshot = "0f2b30ad34d46177";

constexpr const char* kProbeLine = "I cannot hear you clearly.";

}  // namespace

void test_system_prompt_digest() {
    const std::string digest = to_hex(fnv1a64(SubtitleTranslationPrompt::system_text));
    assert(digest == kExpectedSystemPrompt);
    std::cout << "PASS: test_system_prompt_digest" << std::endl;
}

void test_single_line_prompt_digest() {
    const std::string prompt = build_subtitle_translation_prompt(kProbeLine, false);
    const std::string digest = to_hex(fnv1a64(prompt));
    assert(digest == kExpectedPromptSingle);
    std::cout << "PASS: test_single_line_prompt_digest" << std::endl;
}

void test_fewshot_prompt_digest() {
    const std::string prompt = build_subtitle_translation_prompt(kProbeLine, true);
    const std::string digest = to_hex(fnv1a64(prompt));
    assert(digest == kExpectedPromptFewshot);
    std::cout << "PASS: test_fewshot_prompt_digest" << std::endl;
}

/// prompt 的骨架：system 段 → user 段 → assistant 生成提示。
/// 形状错较较比内容错更致命（内容错只影响一批样本，形状错会让整条链路空转）。
void test_prompt_shape() {
    const std::string prompt = build_subtitle_translation_prompt(kProbeLine, false);

    assert(prompt.rfind("system\n", 0) == 0);
    assert(prompt.find("\nuser\n") != std::string::npos);
    assert(prompt.find("\nassistant\n") != std::string::npos);

    // Qwen3 chat template 在 assistant 段硬编码了空 think 块。
    // 手机端 llama.cpp 必须用同一模板且 enable_thinking=false，
    // 否则模型会先吐一段思考再给译文 —— 这是整条链路最容易翻车的地方。
    assert(SubtitleTranslationPrompt::think_block == "<think>\n\n</think>\n\n");
    assert(prompt.find("<think>\n\n</think>\n\n") != std::string::npos);

    // 末尾就是生成提示，后面不能再有内容。
    assert(prompt.size() >= SubtitleTranslationPrompt::think_block.size());
    assert(prompt.compare(prompt.size() - SubtitleTranslationPrompt::think_block.size(),
                           SubtitleTranslationPrompt::think_block.size(),
                           SubtitleTranslationPrompt::think_block) == 0);
    std::cout << "PASS: test_prompt_shape" << std::endl;
}

/// few-shot 变体必须**更长**，且包含多轮 assistant 段。
void test_fewshot_extends_prompt() {
    const std::string single = build_subtitle_translation_prompt(kProbeLine, false);
    const std::string fewshot = build_subtitle_translation_prompt(kProbeLine, true);
    assert(fewshot.size() > single.size());

    std::size_t assistant_turns = 0;
    std::size_t cursor = 0;
    while ((cursor = fewshot.find("assistant\n", cursor)) != std::string::npos) {
        ++assistant_turns;
        cursor += 10;
    }
    assert(assistant_turns == 9);  // 8 个样例 + 1 个生成提示
    std::cout << "PASS: test_fewshot_extends_prompt" << std::endl;
}

/// 输出清洗：去掉模型多吐的前缀与思考残留。
/// 规则与 <toolchain>/eval/subtitle_prompt.py::clean_output 严格一一对应。
void test_clean_output() {
    assert(clean_subtitle_translation_output("  hello  ") == "hello");
    assert(clean_subtitle_translation_output("<think>reasoning</think>answer") == "answer");
    assert(clean_subtitle_translation_output("<think>reasoning</think>\nanswer") == "answer");
    assert(clean_subtitle_translation_output("\"quoted\"") == "quoted");
    assert(clean_subtitle_translation_output("first\nsecond") == "first");
    assert(clean_subtitle_translation_output("") == "");

    // 清洗必须不能吃掉正文里的内容 —— 字幕是逐字显示的。
    assert(clean_subtitle_translation_output("a-b") == "a-b");
    std::cout << "PASS: test_clean_output" << std::endl;
}

/// CJK 守卫：识别「回吐英文」失效。
/// 实测小模型有 2%~6% 的样本直接输出英文原文
/// （"desk"、"paranoid and reactionary"），译文里一个汉字都没有。
void test_has_cjk_guard() {
    assert(subtitle_translation_has_cjk("hello") == false);
    assert(subtitle_translation_has_cjk("") == false);
    assert(subtitle_translation_has_cjk("desk") == false);
    assert(subtitle_translation_has_cjk("Hello world") == false);

    // 至少一个 CJK 表意文字即判通过（U+4E00..U+9FFF）。
    assert(subtitle_translation_has_cjk("\xe4\xbd\xa0") == true);                 // U+4E00
    assert(subtitle_translation_has_cjk("abc\xe4\xbd\xa0") == true);
    assert(subtitle_translation_has_cjk("\xe4\xb8\x80") == true);                 // U+4E00
    assert(subtitle_translation_has_cjk("\xe9\xbe\xa5") == true);                 // U+9FA5
    assert(subtitle_translation_has_cjk("\xef\xbc\x8c") == false);                // U+FF0C 全角逗号
    assert(subtitle_translation_has_cjk("\xe4\xb8\xad") == true);                 // U+4E2D
    std::cout << "PASS: test_has_cjk_guard" << std::endl;
}

/// 输出前缀清单必须与 Python 真源的 OUTPUT_PREFIXES 一致。
void test_output_prefixes_present() {
    SubtitleTranslationPrefix prefixes[16] = {};
    const std::size_t count = subtitle_translation_output_prefixes(prefixes, 16);
    assert(count >= 4);
    std::cout << "PASS: test_output_prefixes_present" << std::endl;
}

/// few-shot 样例清单必须存在且非空。
void test_fewshot_examples_present() {
    SubtitleTranslationExample examples[16] = {};
    const std::size_t count = subtitle_translation_fewshot_examples(examples, 16);
    assert(count == 8);
    for (std::size_t index = 0; index < count; ++index) {
        assert(!examples[index].english.empty());
        assert(!examples[index].chinese.empty());
    }
    std::cout << "PASS: test_fewshot_examples_present" << std::endl;
}

int main() {
    test_system_prompt_digest();
    test_single_line_prompt_digest();
    test_fewshot_prompt_digest();
    test_prompt_shape();
    test_fewshot_extends_prompt();
    test_clean_output();
    test_has_cjk_guard();
    test_output_prefixes_present();
    test_fewshot_examples_present();

    std::cout << "All Subtitle Translation Prompt tests passed." << std::endl;
    return 0;
}
