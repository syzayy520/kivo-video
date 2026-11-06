#include "video/subtitle_plane/render/srt_convert.hpp"

#include <cstdio>
#include <cstdlib>

namespace kivo::video::subtitle_plane::render {
namespace {

/// 单个时间戳：找 "HH:MM:SS,mmm" 并转毫秒。pos 从这里推进到时间戳之后。
bool extract_time(const std::string& line, std::size_t& pos, long long& out_ms) {
    // 跳到第一个数字
    while (pos < line.size() && (line[pos] < '0' || line[pos] > '9')) {
        ++pos;
    }
    if (pos + 12 > line.size()) {
        return false;
    }
    // 严格形状：2位:2位:2位,3位（SRT 规范；容错到 '.' 小数分隔符）
    const char* s = line.c_str() + pos;
    int h = 0, m = 0, sec = 0, ms = 0;
    char sep1 = 0, sep2 = 0, frac = 0;
    if (std::sscanf(s, "%2d%c%2d%c%2d%c%3d", &h, &sep1, &m, &sep2, &sec, &frac, &ms) < 6) {
        return false;
    }
    if (sep1 != ':' || sep2 != ':' || (frac != ',' && frac != '.')) {
        return false;
    }
    out_ms = (static_cast<long long>(h) * 3600 + m * 60 + sec) * 1000 + ms;
    pos += 12;
    return true;
}

std::string ms_to_ass_time(long long ms) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%d:%02d:%02d.%02d",
                  static_cast<int>(ms / 3600000),
                  static_cast<int>(ms / 60000 % 60),
                  static_cast<int>(ms / 1000 % 60),
                  static_cast<int>(ms % 1000 / 10));
    return buffer;
}

}  // namespace

long long parse_srt_time(const std::string& text) {
    std::size_t pos = 0;
    long long ms = 0;
    return extract_time(text, pos, ms) ? ms : -1;
}

std::string srt_to_ass(const std::string& srt_text) {
    // 输出：最小 ASS 头 + Default 样式（libass 的 srt 兼容渲染需要的全部）。
    std::string out =
        "[Script Info]\n"
        "ScriptType: v4.00+\n"
        "PlayResX: 1920\n"
        "PlayResY: 1080\n"
        "\n"
        "[V4+ Styles]\n"
        "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, "
        "OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, "
        "ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, "
        "MarginL, MarginR, MarginV, Encoding\n"
        "Style: Default,sans-serif,64,&H00FFFFFF,&H000000FF,&H00000000,&H7F000000,"
        "0,0,0,0,100,100,0,0,1,3,1,2,60,60,40,1\n"
        "\n"
        "[Events]\n"
        "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, "
        "Effect, Text\n";

    // 逐块扫描：以时间行为锚（"--> " 在行内），跳过序号行，收集到下一个空行。
    std::size_t pos = 0;
    int dialogue_count = 0;
    while (pos < srt_text.size()) {
        // 找下一行
        const std::size_t line_end = srt_text.find('\n', pos);
        const std::string line =
            srt_text.substr(pos, line_end == std::string::npos ? std::string::npos
                                                               : line_end - pos);
        std::size_t cursor = 0;
        long long start_ms = 0;
        long long end_ms = 0;
        if (line.find("-->") != std::string::npos &&
            extract_time(line, cursor, start_ms)) {
            // 第一个时间戳之后找 "-->"，再找结束时间
            const std::size_t arrow = line.find("-->", cursor);
            if (arrow != std::string::npos) {
                std::size_t cursor2 = arrow + 3;
                if (extract_time(line, cursor2, end_ms)) {
                    // 文本 = 后续行直到空行/EOF
                    std::size_t text_pos =
                        line_end == std::string::npos ? srt_text.size() : line_end + 1;
                    std::string text;
                    while (text_pos < srt_text.size()) {
                        const std::size_t e = srt_text.find('\n', text_pos);
                        const std::string text_line =
                            srt_text.substr(text_pos, e == std::string::npos
                                                         ? std::string::npos
                                                         : e - text_pos);
                        if (text_line.empty()) {
                            break;
                        }
                        if (!text.empty()) {
                            text += "\\N";
                        }
                        text += text_line;
                        if (e == std::string::npos) {
                            break;
                        }
                        text_pos = e + 1;
                    }
                    if (!text.empty()) {
                        out += "Dialogue: 0," + ms_to_ass_time(start_ms) + "," +
                               ms_to_ass_time(end_ms) + ",Default,,0,0,0,," + text + "\n";
                        ++dialogue_count;
                    }
                }
            }
        }
        pos = line_end == std::string::npos ? srt_text.size() : line_end + 1;
    }
    (void)dialogue_count;
    return out;
}

}  // namespace kivo::video::subtitle_plane::render
