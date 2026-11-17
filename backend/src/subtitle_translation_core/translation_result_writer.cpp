// ============================================================================
// 译文写出：SRT 生成。
//
// 落位路径: backend/src/subtitle_translation_core/translation_result_writer.cpp
// 零平台依赖：只用 <cstdio> / <string> / <vector>。
// ============================================================================

#include "kivo/cinema_engine/subtitle_translation_core/translation_result_writer.hpp"

#include <cstdio>

namespace kivo::cinema_engine {

namespace {

/// 毫秒 → SRT 时间戳 "HH:MM:SS,mmm"。
/// 负值钳到 0：SRT 没有负时间，负值一定是上游算错了，
/// 写出去也只会让播放器丢弃这一条，不如统一钳成 0 让它可见。
std::string srt_timestamp(int64_t time_ms) {
    const int64_t clamped = time_ms < 0 ? 0 : time_ms;
    const int64_t hours = clamped / 3600000;
    const int64_t minutes = (clamped / 60000) % 60;
    const int64_t seconds = (clamped / 1000) % 60;
    const int64_t millis = clamped % 1000;

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%02lld:%02lld:%02lld,%03lld",
                  static_cast<long long>(hours), static_cast<long long>(minutes),
                  static_cast<long long>(seconds), static_cast<long long>(millis));
    return std::string(buffer);
}

}  // namespace

std::string write_translated_srt(const std::vector<SubtitleTranslationTimedLine>& lines) {
    std::string out;
    int index = 0;

    for (const auto& line : lines) {
        // 翻译失败或时间倒挂的行不写。序号不跳号 —— 跳号会让部分播放器拒收。
        if (line.text.empty() || line.end_time_ms <= line.start_time_ms) {
            continue;
        }
        ++index;

        out += std::to_string(index);
        out += "\r\n";
        out += srt_timestamp(line.start_time_ms);
        out += " --> ";
        out += srt_timestamp(line.end_time_ms);
        out += "\r\n";
        out += line.text;
        out += "\r\n";
        out += "\r\n";
    }

    return out;
}

}  // namespace kivo::cinema_engine
