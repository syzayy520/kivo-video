#pragma once

// ============================================================================
// AUTO-GENERATED -- DO NOT EDIT BY HAND.
//
// Source of truth : <toolchain>/eval/subtitle_prompt.py
// Generator       : <toolchain>/eval/gen_cpp_header.py
// Verifier        : <toolchain>/eval/verify_generated.py  (compile + byte-compare)
// SHA256          : system prompt = f51fff798489e6fe87f9ad4cd09947badd95be89baff4e969b83679e712e3ca1
//
// The prompt below is byte-identical to the one used to train the
// Qwen3-14B subtitle LoRA, so desktop evaluation and on-device inference
// use exactly the same template. To change the prompt, edit the Python
// source of truth and regenerate this file -- never hand-edit.
//
// The output cleaning rules and the CJK guard live here too, for the same
// reason: the desktop harness and the phone must agree bit for bit.
//
// This file is pure ASCII: every non-ASCII byte is escaped as \xNN.
//
// Rationale: this header carries the PROMPT STRINGS, which must be
// byte-exact. The repo's desktop build is already safe -- the root
// CMakeLists.txt applies /utf-8 globally for MSVC -- and NDK clang reads
// UTF-8 by default. Pure ASCII is kept anyway so the contract does not
// depend on that global flag.
//
// Why it matters (measured): compiling a single source file standalone
// without /utf-8 makes MSVC read it as CP936 (GBK). GBK is a double-byte
// encoding, and the LAST byte of a 3-byte UTF-8 character (such as the
// ideographic full stop) is a dangling GBK lead byte -- it swallows the
// newline that follows, so the comment never ends and the next line is
// pulled into it. The code structure then breaks, and the compiler reports
// line numbers that do not match the source.
//
// Rule of thumb:
//   - generated contracts (carry prompt strings) -> pure ASCII, \xNN escapes
//   - hand-written sources (Chinese in comments only) -> repo convention,
//     UTF-8 without BOM, and their target passes /utf-8 explicitly
//
// Target path: backend/include/kivo/cinema_engine/subtitle_translation_core/subtitle_translation_prompt.hpp
// ============================================================================

#include <cstddef>
#include <string>
#include <string_view>

namespace kivo::cinema_engine {

/// SubtitleTranslationPrompt holds the subtitle translation prompt contract.
/// schema_version: cinema_engine.subtitle_translation.prompt.v1
///
/// Notes:
///   - No platform dependencies, no Android headers: safe for backend/core.
///   - The Qwen3 chat template hardcodes an empty think block in the
///     assistant turn. The llama.cpp side MUST use the same template with
///     enable_thinking=false, otherwise the model emits a reasoning block
///     before the translation and the subtitle is unusable.
struct SubtitleTranslationPrompt
{
    std::string schema_version{"cinema_engine.subtitle_translation.prompt.v1"};

    /// System role text. Byte-identical to the teacher training log.
    static constexpr std::string_view system_text{
        "\xe4\xbd\xa0\xe6\x98\xaf\xe4\xb8\x93\xe4\xb8\x9a\xe5\xbd\xb1\xe8\xa7\x86\xe5\xad\x97\xe5\xb9"
        "\x95\xe7\xbf\xbb\xe8\xaf\x91\xe6\xa8\xa1\xe5\x9e\x8b\xe3\x80\x82\n"
        "\xe4\xbb\xbb\xe5\x8a\xa1\xef\xbc\x9a\xe6\x8a\x8a\xe8\x8b\xb1\xe6\x96\x87\xe5\xbd\xb1\xe8\xa7"
        "\x86\xe5\xad\x97\xe5\xb9\x95\xe7\xbf\xbb\xe8\xaf\x91\xe6\x88\x90\xe8\x87\xaa\xe7\x84\xb6\xe3"
        "\x80\x81\xe7\xae\x80\xe6\xb4\x81\xe3\x80\x81\xe5\x8f\xa3\xe8\xaf\xad\xe5\x8c\x96\xe7\x9a\x84"
        "\xe7\xae\x80\xe4\xbd\x93\xe4\xb8\xad\xe6\x96\x87\xe5\xad\x97\xe5\xb9\x95\xe3\x80\x82\n"
        "\xe8\xbe\x93\xe5\x87\xba\xe8\xa6\x81\xe6\xb1\x82\xef\xbc\x9a\n"
        "- \xe5\x8f\xaa\xe8\xbe\x93\xe5\x87\xba\xe6\x9c\x80\xe7\xbb\x88\xe4\xb8\xad\xe6\x96\x87\xe5\xad"
        "\x97\xe5\xb9\x95\xe3\x80\x82\n"
        "- \xe4\xb8\x8d\xe8\xa6\x81\xe8\xa7\xa3\xe9\x87\x8a\xef\xbc\x8c\xe4\xb8\x8d\xe8\xa6\x81\xe5\x88"
        "\x86\xe6\x9e\x90\xef\xbc\x8c\xe4\xb8\x8d\xe8\xa6\x81\xe5\xa4\x87\xe6\xb3\xa8\xe3\x80\x82\n"
        "- \xe4\xb8\x8d\xe8\xa6\x81\xe8\xbe\x93\xe5\x87\xba\xe5\x8e\x9f\xe6\x96\x87\xe3\x80\x82\n"
        "- \xe4\xb8\x8d\xe8\xa6\x81\xe8\xbe\x93\xe5\x87\xba\xe2\x80\x9c\xe7\xbf\xbb\xe8\xaf\x91\xef\xbc"
        "\x9a\xe2\x80\x9d\xe2\x80\x9c\xe8\xaf\x91\xe6\x96\x87\xef\xbc\x9a\xe2\x80\x9d\xe2\x80\x9c\xe4"
        "\xb8\xad\xe6\x96\x87\xef\xbc\x9a\xe2\x80\x9d\xe7\xad\x89\xe5\x89\x8d\xe7\xbc\x80\xe3\x80\x82"
        "\n"
        "- \xe4\xb8\x8d\xe8\xa6\x81\xe6\xb7\xbb\xe5\x8a\xa0\xe6\x8b\xac\xe5\x8f\xb7\xe8\xaf\xb4\xe6\x98"
        "\x8e\xe3\x80\x82\n"
        "- \xe4\xb8\x8d\xe8\xa6\x81\xe9\x80\x90\xe5\xad\x97\xe7\xa1\xac\xe7\xbf\xbb\xef\xbc\x8c\xe8\xa6"
        "\x81\xe5\x83\x8f\xe7\x9c\x9f\xe5\xae\x9e\xe5\xbd\xb1\xe8\xa7\x86\xe5\xaf\xb9\xe7\x99\xbd\xe3"
        "\x80\x82\n"
        "- \xe5\x8f\xa5\xe5\xad\x90\xe5\xb0\xbd\xe9\x87\x8f\xe7\x9f\xad\xef\xbc\x8c\xe9\x80\x82\xe5\x90"
        "\x88\xe5\xb1\x8f\xe5\xb9\x95\xe5\xad\x97\xe5\xb9\x95\xe9\x98\x85\xe8\xaf\xbb\xe3\x80\x82\n"
        "- \xe4\xbf\x9d\xe7\x95\x99\xe5\xbf\x85\xe8\xa6\x81\xe7\x9a\x84\xe4\xba\xba\xe5\x90\x8d\xe3\x80"
        "\x81\xe5\x9c\xb0\xe5\x90\x8d\xe3\x80\x81\xe5\x93\x81\xe7\x89\x8c\xe5\x90\x8d\xe5\x92\x8c\xe4"
        "\xb8\x93\xe6\x9c\x89\xe5\x90\x8d\xe8\xaf\x8d\xe3\x80\x82\n"
        "- \xe8\xaf\xad\xe6\xb0\x94\xe8\xa6\x81\xe8\x87\xaa\xe7\x84\xb6\xef\xbc\x8c\xe7\xac\xa6\xe5\x90"
        "\x88\xe4\xba\xba\xe7\x89\xa9\xe8\xaf\xb4\xe8\xaf\x9d\xe6\x96\xb9\xe5\xbc\x8f\xe3\x80\x82"
    };

    /// User role prefix, before the English source line.
    static constexpr std::string_view user_prefix{
        "\xe8\x8b\xb1\xe6\x96\x87\xe5\xad\x97\xe5\xb9\x95\xef\xbc\x9a\n"
    };

    /// User role suffix, after the English source line.
    static constexpr std::string_view user_suffix{
        "\n"
        "\n"
        "\xe5\x8f\xaa\xe8\xbe\x93\xe5\x87\xba\xe4\xb8\xad\xe6\x96\x87\xe5\xad\x97\xe5\xb9\x95\xef\xbc"
        "\x9a\n"
    };

    /// Empty think block emitted by the Qwen3 chat template.
    static constexpr std::string_view think_block{
        "<think>\n"
        "\n"
        "</think>\n"
        "\n"
    };
};

/// Few-shot example pair (English source, expected Chinese subtitle).
struct SubtitleTranslationExample
{
    std::string_view english;
    std::string_view chinese;
};

/// Returns the built-in few-shot examples (8 pairs, real OpenSubtitles lines).
/// @param capacity  Maximum number of entries to write into out_examples.
/// @return          Number of entries actually written.
std::size_t subtitle_translation_fewshot_examples(
    SubtitleTranslationExample* out_examples,
    std::size_t capacity) noexcept;

/// Builds the full ChatML prompt for one English subtitle line.
///
/// Output shape:
///     system\n<system_text>\nuser\n<user_prefix><english><user_suffix>\nassistant\n<think>\n\n</think>\n\n
///
/// @param english  Source subtitle line (may contain newlines).
/// @param fewshot  When true, prepend the built-in examples as prior turns.
/// @return         Rendered prompt, ready to feed to llama.cpp completion.
std::string build_subtitle_translation_prompt(std::string_view english,
                                              bool fewshot = false);

/// Prefixes the model sometimes emits despite the prompt forbidding them.
/// Kept in sync with subtitle_prompt.py::OUTPUT_PREFIXES.
struct SubtitleTranslationPrefix
{
    std::string_view text;
};

/// Returns the output prefixes to strip, in order.
/// @param capacity  Maximum number of entries to write into out_prefixes.
/// @return          Number of entries actually written.
std::size_t subtitle_translation_output_prefixes(
    SubtitleTranslationPrefix* out_prefixes,
    std::size_t capacity) noexcept;

/// Cleans a raw model completion into a displayable subtitle line.
///
/// Mirrors subtitle_prompt.py::clean_output exactly:
///   1. trim surrounding whitespace
///   2. drop anything before the last "</think>" (think-block residue)
///   3. strip known prefixes such as the Chinese "translate:" markers
///   4. strip one pair of surrounding double quotes
///   5. keep only the first line
///
/// @param raw  Raw completion text.
/// @return     Cleaned subtitle line (may be empty).
std::string clean_subtitle_translation_output(std::string_view raw);

/// Reports whether text contains at least one CJK ideograph (U+4E00..U+9FFF).
///
/// This is the guard for the "echo the English source" failure mode observed on
/// small models: when the model refuses to translate, the completion contains
/// no CJK at all. The backend should treat such a completion as a failure and
/// retry (or escalate to the cloud model) instead of displaying it.
///
/// @param text  Text to inspect (UTF-8).
/// @return      True when at least one CJK ideograph is present.
bool subtitle_translation_has_cjk(std::string_view text) noexcept;

}  // namespace kivo::cinema_engine
