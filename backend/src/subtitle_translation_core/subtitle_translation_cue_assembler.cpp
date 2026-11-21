// ============================================================================
// Cue 装配器实现。契约见 subtitle_translation_cue_assembler.hpp。
//
// 落位路径: backend/src/subtitle_translation_core/subtitle_translation_cue_assembler.cpp
// 手写源文件（注释含中文）→ CMake 该 target 显式 /utf-8（契约头才是生成物）。
// ============================================================================

#include "kivo/cinema_engine/subtitle_translation_core/subtitle_translation_cue_assembler.hpp"

#include <cmath>
#include <string>

namespace kivo::cinema_engine {

namespace {

constexpr const char* kCpsIssueCategory = "cps";

bool is_space_char(char ch) noexcept
{
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\v' || ch == '\f';
}

std::string_view trim_view(std::string_view text) noexcept
{
    while (!text.empty() && is_space_char(text.front())) { text.remove_prefix(1); }
    while (!text.empty() && is_space_char(text.back())) { text.remove_suffix(1); }
    return text;
}

/// UTF-8 解码单个码点。返回字节数；非法序列返回 1 并把码点置为 0xFFFD。
std::size_t decode_utf8(std::string_view text, std::size_t index, std::uint32_t& codepoint) noexcept
{
    const auto lead = static_cast<unsigned char>(text[index]);
    codepoint = 0;
    std::size_t length = 1;
    if (lead < 0x80u) {
        codepoint = lead;
    } else if ((lead & 0xE0u) == 0xC0u) {
        codepoint = lead & 0x1Fu; length = 2;
    } else if ((lead & 0xF0u) == 0xE0u) {
        codepoint = lead & 0x0Fu; length = 3;
    } else if ((lead & 0xF8u) == 0xF0u) {
        codepoint = lead & 0x07u; length = 4;
    } else {
        codepoint = 0xFFFDu;
        return 1;
    }
    if (index + length > text.size()) {
        codepoint = 0xFFFDu;
        return 1;
    }
    for (std::size_t offset = 1; offset < length; ++offset) {
        const auto cont = static_cast<unsigned char>(text[index + offset]);
        if ((cont & 0xC0u) != 0x80u) {
            codepoint = 0xFFFDu;
            return 1;
        }
        codepoint = (codepoint << 6) | (cont & 0x3Fu);
    }
    return length;
}

bool is_cjk_ideograph(std::uint32_t cp) noexcept
{
    return (cp >= 0x4E00u && cp <= 0x9FFFu) || (cp >= 0x3400u && cp <= 0x4DBFu);
}

bool is_kana(std::uint32_t cp) noexcept
{
    return (cp >= 0x3040u && cp <= 0x309Fu) ||   // 平假名
           (cp >= 0x30A0u && cp <= 0x30FFu) ||   // 片假名
           (cp >= 0xFF66u && cp <= 0xFF9Du);     // 半角片假名
}

bool is_latin_letter(std::uint32_t cp) noexcept
{
    return (cp >= 0x41u && cp <= 0x5Au) || (cp >= 0x61u && cp <= 0x7Au);
}

/// 剥离大括号标签（{\an8}）与内联 HTML 式标签（<i>、</b>）。
std::string strip_tags(std::string_view text)
{
    std::string out;
    out.reserve(text.size());
    std::size_t index = 0;
    while (index < text.size()) {
        const char ch = text[index];
        if (ch == '{') {
            const std::size_t close = text.find('}', index);
            if (close == std::string_view::npos) { break; }
            index = close + 1;
            continue;
        }
        if (ch == '<') {
            const std::size_t close = text.find('>', index);
            if (close == std::string_view::npos) { break; }
            index = close + 1;
            continue;
        }
        // Tab 在字幕显示上无意义，归一成空格 —— 同时保证 JNI 平铺协议
        //（start	end	text）单行可解析（T-B nativeParseCues 依赖）。
        out.push_back(ch == '	' ? ' ' : ch);
        ++index;
    }
    return out;
}

/// cue 内折行合并：拉丁词间补一个空格，CJK 相邻直接拼接。
std::string merge_folded_lines(const std::vector<std::string>& lines)
{
    std::string out;
    bool last_was_cjk = false;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        const std::string& line = lines[index];
        if (line.empty()) { continue; }
        if (index > 0 && !last_was_cjk && !line.empty()) {
            const auto lead = static_cast<unsigned char>(line.front());
            const bool starts_cjk = lead >= 0x80u;  // 多字节首字节 ⇒ 非 ASCII（近似 CJK/全角）
            if (!starts_cjk) { out.push_back(' '); }
        }
        out += line;
        std::uint32_t codepoint = 0;
        std::size_t consumed = 0;
        std::size_t walk = 0;
        while (walk < line.size()) {
            consumed = decode_utf8(line, walk, codepoint);
            walk += consumed;
        }
        last_was_cjk = is_cjk_ideograph(codepoint);
    }
    return out;
}

/// 时间戳解析：`HH:MM:SS,mmm` / `MM:SS.mmm` / `SS,mmm` 都收。
/// 解析失败返回 false。
bool parse_srt_timestamp(std::string_view token, std::int64_t& out_ms) noexcept
{
    token = trim_view(token);
    if (token.empty()) { return false; }

    // 毫秒段：逗号或点分隔（"001" -> 1ms；scale 100/10/1 逐位递减，只算前三位）。
    std::int64_t millis = 0;
    const std::size_t frac_at = token.find_first_of(",.");
    if (frac_at != std::string_view::npos) {
        std::string_view frac = token.substr(frac_at + 1);
        token = token.substr(0, frac_at);
        std::size_t digits = 0;
        std::int64_t scale = 100;
        for (const char ch : frac) {
            if (ch < '0' || ch > '9') { return false; }
            if (digits < 3) {
                millis += static_cast<std::int64_t>(ch - '0') * scale;
                scale /= 10;
            }
            ++digits;
        }
    }

    // 时间段：按冒号从右往左（秒/分/时）。字段上界 = 上次扫描起点，
    // 否则 substr(colon+1) 会把「分:秒」整段吞成一个 field（探针实测 field='00:01'）。
    std::int64_t seconds = 0;
    std::int64_t multiplier = 1;
    std::size_t pos = token.size();
    std::size_t fields = 0;
    while (pos != std::string_view::npos && fields < 3) {
        const std::size_t colon = token.rfind(':', pos == token.size() ? pos : pos - 1);
        const std::string_view field =
            colon == std::string_view::npos
                ? token.substr(0, pos)
                : token.substr(colon + 1, pos - colon - 1);
        const std::string_view value_view = trim_view(field);
        if (value_view.empty()) { return false; }
        std::int64_t value = 0;
        for (const char ch : value_view) {
            if (ch < '0' || ch > '9') { return false; }
            value = value * 10 + (ch - '0');
        }
        seconds += value * multiplier;
        multiplier *= 60;
        ++fields;
        pos = colon;
    }
    if (fields == 0) { return false; }

    out_ms = seconds * 1000 + millis;
    return true;
}

/// 行是否为对话行（"- " / "-" 开头）。
bool is_dialogue_line(std::string_view line) noexcept
{
    line = trim_view(line);
    return line.size() >= 1 && line[0] == '-';
}

std::string strip_dialogue_prefix(std::string_view line)
{
    line = trim_view(line);
    if (line.size() >= 2 && (line[1] == ' ' || line[1] == '\t')) {
        return std::string(line.substr(2));
    }
    return std::string(line.substr(1));
}

}  // namespace

SubtitleCueLanguage detect_subtitle_cue_language(std::string_view text) noexcept
{
    std::size_t kana = 0, cjk = 0, latin = 0, total_letters = 0;
    std::size_t index = 0;
    while (index < text.size()) {
        std::uint32_t codepoint = 0;
        const std::size_t consumed = decode_utf8(text, index, codepoint);
        index += consumed;
        if (is_kana(codepoint)) { ++kana; ++total_letters; continue; }
        if (is_cjk_ideograph(codepoint)) { ++cjk; ++total_letters; continue; }
        if (is_latin_letter(codepoint)) { ++latin; ++total_letters; }
    }
    if (kana > 0) { return SubtitleCueLanguage::Japanese; }
    if (total_letters == 0) { return SubtitleCueLanguage::Other; }
    if (cjk * 10 >= total_letters * 3) { return SubtitleCueLanguage::Chinese; }
    // 真混排：CJK 与拉丁都显著存在且哪方都不占 30% —— v1 保守整 cue 翻译。
    if (cjk > 0 && latin > 0 && cjk * 10 < total_letters * 3 &&
        latin * 10 < total_letters * 4) {
        return SubtitleCueLanguage::Mixed;
    }
    // 拉丁为主（无 CJK）：一律按 English 处理 —— 高频词表区分西/法/德是
    // 打地鼠（"Stop right there." 三个实词无一命中即翻车，2026-11-21 实测）；
    // v1 输入只支持英文，拉丁语系整 cue 进同一翻译管线，诚实标注交给
    // 轨道名与完成卡，不在语言判定里硬分。
    if (latin * 10 >= total_letters * 4) { return SubtitleCueLanguage::English; }
    if (cjk > 0) { return SubtitleCueLanguage::Chinese; }
    return SubtitleCueLanguage::Other;
}

double subtitle_cue_cps(std::string_view text, std::int64_t duration_ms) noexcept
{
    if (duration_ms <= 0) { return 0.0; }
    double units = 0.0;
    std::size_t index = 0;
    while (index < text.size()) {
        std::uint32_t codepoint = 0;
        const std::size_t consumed = decode_utf8(text, index, codepoint);
        index += consumed;
        // 统一口径：CJK/全角按 1，拉丁字母按 0.5，其余忽略。
        if (is_cjk_ideograph(codepoint) || codepoint >= 0xFF00u) { units += 1.0; }
        else if (is_latin_letter(codepoint)) { units += 0.5; }
    }
    return units / (static_cast<double>(duration_ms) / 1000.0);
}

SubtitleTranslationCueAssembly assemble_subtitle_translation_cues(
    std::string_view srt_utf8,
    std::vector<SubtitleTranslationCueIssue>* out_issues) noexcept
{
    SubtitleTranslationCueAssembly assembly;
    auto add_issue = [&](std::size_t index, const char* category, const std::string& reason) {
        if (out_issues != nullptr) {
            out_issues->push_back({index, category, reason});
        }
    };

    // UTF-16 漏网：编码归一是导入层职责，本层只认 UTF-8。NUL 字节 = 不是 UTF-8 文本。
    if (srt_utf8.find('\0') != std::string_view::npos) {
        add_issue(0, "encoding", "input contains NUL bytes (UTF-16 not normalized)");
        return assembly;
    }

    // BOM 剥离 + 换行统一（CRLF/CR → LF）。
    std::string normalized;
    normalized.reserve(srt_utf8.size());
    std::string_view body = srt_utf8;
    if (body.size() >= 3 && static_cast<unsigned char>(body[0]) == 0xEF &&
        static_cast<unsigned char>(body[1]) == 0xBB &&
        static_cast<unsigned char>(body[2]) == 0xBF) {
        body.remove_prefix(3);
    }
    for (const char ch : body) {
        if (ch != '\r') { normalized.push_back(ch); }
    }

    // 状态机：序号行（可选，跳过）→ 时间轴行 → 文本行（至空行/序号/EOF）。
    SubtitleTranslationCue current;
    bool has_current = false;
    bool current_has_timestamp = false;
    std::vector<std::string> current_lines;
    std::size_t source_index = 0;

    auto flush_cue = [&]() {
        if (!has_current) { return; }
        ++source_index;  // 已结束的块计数（含空块）
        if (!current_has_timestamp) {
            add_issue(source_index - 1, "timestamp",
                      "cue block without a timestamp line; skipped");
        } else if (current_lines.empty()) {
            add_issue(source_index - 1, "empty", "cue has no text; skipped");
        } else {
            const std::int64_t duration_now =
                current.end_time_ms - current.start_time_ms;
            if (duration_now <= 0) {
                add_issue(source_index - 1, "timestamp",
                          "zero or inverted duration; cue kept as-is");
            }
            // 对话式 cue：每行独立单元（评审 A4a）。**不限行数** —— 单行对话
            // （"- Get out."）同样剥前缀进 dialogue_units，否则 "- " 跟着进
            // 模型（集成测试 source_lines[1] 实测抓出，2026-11-21）。
            bool all_dialogue = !current_lines.empty();
            for (const std::string& line : current_lines) {
                if (!is_dialogue_line(line)) { all_dialogue = false; break; }
            }
            if (all_dialogue) {
                for (std::string& line : current_lines) {
                    current.dialogue_units.push_back(strip_dialogue_prefix(line));
                }
                // 归一文本 = 各单元合并（展示用），时间轴不动（DEC-07 v2）。
                current.text = merge_folded_lines(current_lines);
            } else {
                current.text = merge_folded_lines(current_lines);
            }
            // 标签剥离在行进入时已做（strip_tags）；此处做语言判定与 CPS。
            const auto language = detect_subtitle_cue_language(current.text);
            switch (language) {
                case SubtitleCueLanguage::English: ++assembly.english_cue_count; break;
                case SubtitleCueLanguage::Chinese: ++assembly.chinese_cue_count; break;
                case SubtitleCueLanguage::Japanese: ++assembly.japanese_cue_count; break;
                case SubtitleCueLanguage::Mixed: ++assembly.mixed_cue_count; break;
                case SubtitleCueLanguage::Other: ++assembly.other_cue_count; break;
            }
            const std::int64_t duration = current.end_time_ms - current.start_time_ms;
            const double cps = subtitle_cue_cps(current.text, duration);
            // 占位阈值 25：DEC-17 定的「阈值 T-C 实测后冻结」—— 先记录不阻断。
            if (duration > 0 && cps > 25.0) {
                add_issue(source_index - 1, kCpsIssueCategory,
                          "reading speed above placeholder threshold (25 cps)");
            }
            assembly.cues.push_back(current);
        }
        current = SubtitleTranslationCue{};
        has_current = false;
        current_has_timestamp = false;
        current_lines.clear();
    };

    std::size_t line_start = 0;
    std::size_t line_end = 0;
    auto each_line = [&](const std::string& blob) {
        line_start = 0;
        while (line_start <= blob.size()) {
            line_end = blob.find('\n', line_start);
            if (line_end == std::string::npos) {
                line_end = blob.size();
            }
            std::string_view line = trim_view(
                std::string_view(blob).substr(line_start, line_end - line_start));
            if (line.empty()) {
                flush_cue();
            } else if (line.find("-->") != std::string_view::npos) {
                flush_cue();
                has_current = true;
                current_has_timestamp = true;
                const std::size_t arrow = line.find("-->");
                if (!parse_srt_timestamp(line.substr(0, arrow), current.start_time_ms) ||
                    !parse_srt_timestamp(line.substr(arrow + 3), current.end_time_ms)) {
                    add_issue(source_index, "timestamp",
                              "unparseable timestamp; cue skipped at flush");
                    current_has_timestamp = false;
                }
            } else if (!has_current &&
                       line.find_first_not_of("0123456789") == std::string_view::npos) {
                // 纯数字行 = SRT 序号，**只在 cue 边界有效**——cue 内部（时间轴
                // 之后）的纯数字行是字幕文本（年份 "1945"、计数），必须放行。
                // 集成测试 test_full_chain_mixed_source 的 "1945" 实测抓出。
                // 序号行本身可完全忽略：新 cue 的开始由时间轴行的 flush 语义保证。
            } else {
                if (!has_current) {
                    // 无时间轴的散文本：记 issue 后并入"无时间戳块"，flush 时丢弃。
                    has_current = true;
                    current_has_timestamp = false;
                    add_issue(source_index, "timestamp",
                              "text before any timestamp; block dropped");
                }
                current_lines.push_back(std::string(strip_tags(line)));
            }
            if (line_end == blob.size()) { break; }
            line_start = line_end + 1;
        }
    };
    each_line(normalized);
    flush_cue();

    return assembly;
}

}  // namespace kivo::cinema_engine