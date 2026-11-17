// ============================================================================
// AUTO-GENERATED -- DO NOT EDIT BY HAND.
// Source of truth: <toolchain>/eval/subtitle_prompt.py
// Pure ASCII (non-ASCII bytes escaped as \xNN); see the header for the reason.
// Target path: backend/src/subtitle_translation_core/subtitle_translation_prompt.cpp
// ============================================================================

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_prompt.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace kivo::cinema_engine {

namespace {

constexpr SubtitleTranslationExample kFewshotExamples[] = {
    {{
        "I cannot hear you clearly."
    }, {
        "\xe6\x88\x91\xe5\x90\xac\xe4\xb8\x8d\xe6\xb8\x85\xe6\xa5\x9a \xe8\xaf\xb7\xe5\x86\x8d\xe8\xaf"
        "\xb4\xe4\xb8\x80\xe9\x81\x8d"
    }},
    {{
        "- You want to call your daddy?"
    }, {
        "- \xe6\x83\xb3\xe7\xbb\x99\xe7\x88\xb8\xe7\x88\xb8\xe6\x89\x93\xe7\x94\xb5\xe8\xaf\x9d\xe5\x90"
        "\x97?"
    }},
    {{
        "Look, I don't want any trouble."
    }, {
        "\xe5\x90\xac\xe7\x9d\x80 \xe6\x88\x91\xe4\xb8\x8d\xe6\x83\xb3\xe6\x83\xb9\xe9\xba\xbb\xe7\x83"
        "\xa6"
    }},
    {{
        "What are you doing here?"
    }, {
        "\xe4\xbd\xa0\xe6\x9d\xa5\xe5\xb9\xb2\xe4\xbb\x80\xe4\xb9\x88?"
    }},
    {{
        "We should go. It's getting late."
    }, {
        "\xe6\x88\x91\xe4\xbb\xac\xe8\xaf\xa5\xe8\xb5\xb0\xe4\xba\x86 \xe5\xa4\xa9\xe4\xb8\x8d\xe6\x97"
        "\xa9\xe4\xba\x86"
    }},
    {{
        "That's not what I meant."
    }, {
        "\xe6\x88\x91\xe4\xb8\x8d\xe6\x98\xaf\xe9\x82\xa3\xe4\xb8\xaa\xe6\x84\x8f\xe6\x80\x9d"
    }},
    {{
        "Get out of my house!"
    }, {
        "\xe6\xbb\x9a\xe5\x87\xba\xe6\x88\x91\xe5\xae\xb6!"
    }},
    {{
        "I'll do it. I promise."
    }, {
        "\xe6\x88\x91\xe6\x9d\xa5 \xe6\x88\x91\xe4\xbf\x9d\xe8\xaf\x81"
    }},
};

constexpr std::size_t kFewshotCount = sizeof(kFewshotExamples) / sizeof(kFewshotExamples[0]);

constexpr SubtitleTranslationPrefix kOutputPrefixes[] = {
    {{
        "\xe7\xbf\xbb\xe8\xaf\x91\xef\xbc\x9a"
    }},
    {{
        "\xe8\xaf\x91\xe6\x96\x87\xef\xbc\x9a"
    }},
    {{
        "\xe4\xb8\xad\xe6\x96\x87\xef\xbc\x9a"
    }},
    {{
        "\xe7\xbf\xbb\xe8\xaf\x91:"
    }},
    {{
        "\xe8\xaf\x91\xe6\x96\x87:"
    }},
    {{
        "\xe4\xb8\xad\xe6\x96\x87:"
    }},
    {{
        "\xe5\xad\x97\xe5\xb9\x95\xef\xbc\x9a"
    }},
};

constexpr std::size_t kOutputPrefixCount = sizeof(kOutputPrefixes) / sizeof(kOutputPrefixes[0]);

constexpr std::string_view kThinkClose{"</think>"};

void append_user_turn(std::string& out, std::string_view english)
{
    out.append("user\n");
    out.append(SubtitleTranslationPrompt::user_prefix);
    out.append(english);
    out.append(SubtitleTranslationPrompt::user_suffix);
    out.push_back('\n');
}

void append_assistant_turn(std::string& out, std::string_view chinese)
{
    out.append("assistant\n");
    out.append(SubtitleTranslationPrompt::think_block);
    out.append(chinese);
    out.push_back('\n');
}

bool is_space(char ch) noexcept
{
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\v' || ch == '\f';
}

std::string_view trim(std::string_view text) noexcept
{
    while (!text.empty() && is_space(text.front())) { text.remove_prefix(1); }
    while (!text.empty() && is_space(text.back())) { text.remove_suffix(1); }
    return text;
}

}  // namespace

std::size_t subtitle_translation_fewshot_examples(
    SubtitleTranslationExample* out_examples,
    std::size_t capacity) noexcept
{
    if (out_examples == nullptr) {
        return 0;
    }
    const std::size_t count = capacity < kFewshotCount ? capacity : kFewshotCount;
    for (std::size_t index = 0; index < count; ++index) {
        out_examples[index] = kFewshotExamples[index];
    }
    return count;
}

std::size_t subtitle_translation_output_prefixes(
    SubtitleTranslationPrefix* out_prefixes,
    std::size_t capacity) noexcept
{
    if (out_prefixes == nullptr) {
        return 0;
    }
    const std::size_t count = capacity < kOutputPrefixCount ? capacity : kOutputPrefixCount;
    for (std::size_t index = 0; index < count; ++index) {
        out_prefixes[index] = kOutputPrefixes[index];
    }
    return count;
}

std::string clean_subtitle_translation_output(std::string_view raw)
{
    // 1. trim
    std::string_view text = trim(raw);

    // 2. drop think-block residue: keep whatever follows the last "</think>"
    const std::size_t think_at = text.rfind(kThinkClose);
    if (think_at != std::string_view::npos) {
        text.remove_prefix(think_at + kThinkClose.size());
        text = trim(text);
    }

    // 3. strip known prefixes (repeat, so a doubled prefix is fully stripped)
    bool stripped = true;
    while (stripped) {
        stripped = false;
        for (std::size_t index = 0; index < kOutputPrefixCount; ++index) {
            const std::string_view prefix = kOutputPrefixes[index].text;
            if (!prefix.empty() && text.size() >= prefix.size() &&
                text.compare(0, prefix.size(), prefix) == 0) {
                text.remove_prefix(prefix.size());
                text = trim(text);
                stripped = true;
            }
        }
    }

    // 4. strip one pair of surrounding double quotes
    if (text.size() > 1 && text.front() == '"' && text.back() == '"') {
        text.remove_prefix(1);
        text.remove_suffix(1);
        text = trim(text);
    }

    // 5. keep only the first line
    const std::size_t newline_at = text.find('\n');
    if (newline_at != std::string_view::npos) {
        text = text.substr(0, newline_at);
        text = trim(text);
    }

    return std::string(text);
}

bool subtitle_translation_has_cjk(std::string_view text) noexcept
{
    std::size_t index = 0;
    while (index < text.size()) {
        const auto lead = static_cast<unsigned char>(text[index]);
        std::uint32_t codepoint = 0;
        std::size_t length = 0;

        if (lead < 0x80u) {
            codepoint = lead;
            length = 1;
        } else if ((lead & 0xE0u) == 0xC0u) {
            codepoint = lead & 0x1Fu;
            length = 2;
        } else if ((lead & 0xF0u) == 0xE0u) {
            codepoint = lead & 0x0Fu;
            length = 3;
        } else if ((lead & 0xF8u) == 0xF0u) {
            codepoint = lead & 0x07u;
            length = 4;
        } else {
            ++index;
            continue;
        }

        if (index + length > text.size()) {
            break;
        }
        for (std::size_t offset = 1; offset < length; ++offset) {
            codepoint = (codepoint << 6) |
                        (static_cast<unsigned char>(text[index + offset]) & 0x3Fu);
        }

        if (codepoint >= 0x4E00u && codepoint <= 0x9FFFu) {
            return true;
        }
        index += length;
    }
    return false;
}

std::string build_subtitle_translation_prompt(std::string_view english,
                                              bool fewshot)
{
    std::string out;
    out.append("system\n");
    out.append(SubtitleTranslationPrompt::system_text);
    out.push_back('\n');

    if (fewshot) {
        SubtitleTranslationExample examples[16]{};
        const std::size_t count = subtitle_translation_fewshot_examples(examples, 16);
        for (std::size_t index = 0; index < count; ++index) {
            append_user_turn(out, examples[index].english);
            append_assistant_turn(out, examples[index].chinese);
        }
    }

    append_user_turn(out, english);
    out.append("assistant\n");
    out.append(SubtitleTranslationPrompt::think_block);
    return out;
}

}  // namespace kivo::cinema_engine
