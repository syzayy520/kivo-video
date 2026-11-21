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

// ---------------------------------------------------------------------------
// Block mode (added 2026-11-21, design v3.2 DEC-03).
//
// Block prompt renders "1. translation" lines for alignment back to cues.
// The context lines (previous block's raw English, trailing 2 cues) go into
// the USER turn only -- never into the system/few-shot prefix -- so the
// prefix stays content-independent and byte-constant (KV-reuse red line).
// Block few-shot demos also carry a fixed demo context, teaching the model
// "context is not translated; numbered lines are".
// ---------------------------------------------------------------------------

struct SubtitleTranslationBlockPrompt
{
    std::string schema_version{"cinema_engine.subtitle_translation.prompt.block.v1"};

    /// Block system text = system_text + numbered-lines instruction.
    static constexpr std::string_view block_system_text{
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
        "\x88\xe4\xba\xba\xe7\x89\xa9\xe8\xaf\xb4\xe8\xaf\x9d\xe6\x96\xb9\xe5\xbc\x8f\xe3\x80\x82\n"
        "\xe6\x9c\xac\xe6\xac\xa1\xe8\xbe\x93\xe5\x85\xa5\xe5\x8c\x85\xe5\x90\xab\xe5\xa4\x9a\xe6\x9d"
        "\xa1\xe5\xad\x97\xe5\xb9\x95\xe3\x80\x82\n"
        "\xe8\xaf\xb7\xe6\x8c\x89\xe8\xbe\x93\xe5\x85\xa5\xe9\xa1\xba\xe5\xba\x8f\xe9\x80\x90\xe6\x9d"
        "\xa1\xe7\xbf\xbb\xe8\xaf\x91\xef\xbc\x8c\xe6\xaf\x8f\xe6\x9d\xa1\xe8\xaf\x91\xe6\x96\x87\xe5"
        "\x8d\x95\xe7\x8b\xac\xe4\xb8\x80\xe8\xa1\x8c\xef\xbc\x8c\n"
        "\xe5\xb9\xb6\xe4\xb8\x94\xe4\xbf\x9d\xe7\x95\x99\xe6\xaf\x8f\xe6\x9d\xa1\xe5\x89\x8d\xe9\x9d"
        "\xa2\xe7\x9a\x84\xe7\xbc\x96\xe5\x8f\xb7\xef\xbc\x8c\xe6\xa0\xbc\xe5\xbc\x8f\xe4\xb8\xba\xe3"
        "\x80\x8c\xe7\xbc\x96\xe5\x8f\xb7. \xe8\xaf\x91\xe6\x96\x87\xe3\x80\x8d\xe3\x80\x82\n"
        "\xe4\xb8\x8d\xe8\xa6\x81\xe5\x90\x88\xe5\xb9\xb6\xe3\x80\x81\xe6\x8b\x86\xe5\x88\x86\xe6\x88"
        "\x96\xe8\xb7\xb3\xe8\xbf\x87\xe4\xbb\xbb\xe4\xbd\x95\xe4\xb8\x80\xe6\x9d\xa1\xe3\x80\x82"
    };

    static constexpr std::string_view block_user_prefix{
        "\xe8\x8b\xb1\xe6\x96\x87\xe5\xad\x97\xe5\xb9\x95\xef\xbc\x9a\n"
    };
    static constexpr std::string_view block_user_suffix{
        "\n"
        "\n"
        "\xe5\x8f\xaa\xe8\xbe\x93\xe5\x87\xba\xe4\xb8\xad\xe6\x96\x87\xe5\xad\x97\xe5\xb9\x95\xef\xbc"
        "\x9a\n"
    };

    /// Label introducing the reference-context block (context is NOT translated).
    static constexpr std::string_view block_context_label{
        "\xe4\xb8\x8a\xe6\x96\x87\xef\xbc\x88\xe4\xbb\x85\xe4\xbe\x9b\xe7\x90\x86\xe8\xa7\xa3\xef\xbc"
        "\x8c\xe4\xb8\x8d\xe8\xa6\x81\xe7\xbf\xbb\xe8\xaf\x91\xef\xbc\x89\xef\xbc\x9a\n"
    };

    /// Fixed demo context used by the few-shot demo turns (content-constant).
    static constexpr std::size_t block_fewshot_demo_context_count =
        2;
};

/// Renders the block-mode ChatML prompt for a batch of numbered English lines.
///
/// Output shape (fewshot=false, context empty):
///     system\n<block_system>\nuser\n[block_prefix]1. line\n2. line...[block_suffix]\nassistant\n<think>\n\n</think>\n\n
///
/// @param english_lines  Numbered source lines for this block.
/// @param line_count     Number of lines (1..25 per planner budget).
/// @param context_lines  Previous cues' raw English (reference only, not
///                       numbered, NOT to be translated); may be null/0.
/// @param context_count  Number of context lines.
/// @param fewshot        Prepend block-format demo turns (with demo context).
/// @return               Rendered prompt, ready to feed to llama.cpp completion.
std::string build_subtitle_translation_block_prompt(
    const std::string_view* english_lines, std::size_t line_count,
    const std::string_view* context_lines, std::size_t context_count,
    bool fewshot);

struct SubtitleTranslationBlockParse
{
    /// True when every numbered slot 1..expected was present in the output.
    bool count_matched;
};

/// Parses a block completion back into per-line translations (by number).
///
/// Mirrors subtitle_prompt.py::parse_block_output exactly:
///   - blank/whitespace-only lines are skipped;
///   - a line contributes only when its head is purely digits before the
///     first '.'; the number is 1-based and must be within [1, expected];
///   - when all expected slots are found, count_matched = true;
///   - otherwise count_matched = false and only found slots are filled --
///     missing slots stay EMPTY (never fabricated; caller falls back).
///
/// @param raw        Raw block completion.
/// @param expected   Number of lines the block contained.
/// @param out_lines  Array of `expected` std::string slots; missing ones
///                   are left empty.
/// @return           count_matched (see above).
bool parse_subtitle_translation_block_output(
    std::string_view raw, std::size_t expected,
    std::string* out_lines) noexcept;

}  // namespace kivo::cinema_engine
