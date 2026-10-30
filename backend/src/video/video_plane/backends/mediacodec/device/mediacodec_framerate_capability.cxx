#include "video/video_plane/backends/mediacodec/device/mediacodec_framerate_capability.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string_view>
#include <utility>

namespace kivo::video::mediacodec {
namespace {

constexpr char kFieldSeparator = '\t';
constexpr std::size_t kExpectedFieldCount = 8;
constexpr const char* kSchema = "kivo.p13a.framerate/1";

/// 需要回答的目标规格。**这张表就是本卡的判据**：
/// 每次改它都要同步改 `56-p13a-design.md` §二，否则证据与设计会脱节。
struct Target {
    const char* label;
    const char* mime;
    std::int32_t width;
    std::int32_t height;
    double fps;
};

constexpr Target kTargets[] = {
    {"hevc_1080p60", "video/hevc", 1920, 1080, 60.0},
    {"hevc_4k30", "video/hevc", 3840, 2160, 30.0},
    {"hevc_4k60", "video/hevc", 3840, 2160, 60.0},
    {"avc_1080p60", "video/avc", 1920, 1080, 60.0},
    {"avc_4k30", "video/avc", 3840, 2160, 30.0},
    {"avc_4k60", "video/avc", 3840, 2160, 60.0},
};

[[nodiscard]] std::vector<std::string_view> split_fields(std::string_view raw) {
    std::vector<std::string_view> fields;
    std::size_t begin = 0;
    while (true) {
        const std::size_t pos = raw.find(kFieldSeparator, begin);
        if (pos == std::string_view::npos) {
            fields.push_back(raw.substr(begin));
            break;
        }
        fields.push_back(raw.substr(begin, pos - begin));
        begin = pos + 1;
    }
    return fields;
}

[[nodiscard]] bool parse_i32(std::string_view text, std::int32_t& out) {
    if (text.empty()) {
        return false;
    }
    std::int32_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    out = value;
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

/// Kotlin 侧用 `Double.toString()`，会产生 `60.0` / `-1.0` / `null` 之外的形式。
/// 解析失败返回 false，由调用方计入 rejected —— 不静默当 0。
[[nodiscard]] bool parse_f64(std::string_view text, double& out) {
    if (text.empty()) {
        return false;
    }
    // `from_chars` 的浮点重载在部分 MSVC 版本上不完全支持，这里走 strtod。
    std::string buffer(text);
    char* end = nullptr;
    const double value = std::strtod(buffer.c_str(), &end);
    if (end == buffer.c_str() || *end != '\0' || !std::isfinite(value)) {
        return false;
    }
    out = value;
    return true;
}

[[nodiscard]] std::string escape_json(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (const char c : raw) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            default: out += c; break;
        }
    }
    return out;
}

[[nodiscard]] std::int64_t now_unix_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

}  // namespace

double best_fps_for(const std::vector<FramerateRecord>& records, const std::string& mime,
                     std::int32_t width, std::int32_t height) {
    double best = -1.0;
    for (const FramerateRecord& record : records) {
        if (record.mime != mime || record.width != width || record.height != height) {
            continue;
        }
        if (record.size_supported != 1) {
            continue;
        }
        // 两个数都看：厂商声明（max）与像素吞吐可达（achievable）取小，
        // 因为"声明能跑"但"吞吐达不到"时真播会掉帧。
        double candidate = record.max_fps;
        if (record.achievable_fps >= 0.0) {
            candidate = (candidate < 0.0) ? record.achievable_fps
                                          : std::min(candidate, record.achievable_fps);
        }
        if (candidate > best) {
            best = candidate;
        }
    }
    return best;
}

bool parse_framerate_record(const std::string& raw, FramerateRecord& out) {
    const std::vector<std::string_view> fields = split_fields(raw);
    if (fields.size() != kExpectedFieldCount) {
        return false;
    }
    FramerateRecord parsed;
    parsed.name = std::string(fields[0]);
    parsed.mime = std::string(fields[1]);
    if (parsed.name.empty() || parsed.mime.empty()) {
        return false;
    }
    if (!parse_i32(fields[2], parsed.profile) || !parse_i32(fields[3], parsed.width) ||
        !parse_i32(fields[4], parsed.height) || !parse_i32(fields[5], parsed.size_supported) ||
        !parse_f64(fields[6], parsed.max_fps) || !parse_f64(fields[7], parsed.achievable_fps)) {
        return false;
    }
    out = std::move(parsed);
    return true;
}

FramerateProbeOutcome probe_framerates(const std::string& output_path,
                                       const std::vector<std::string>& records) {
    FramerateProbeOutcome outcome;

    if (output_path.empty()) {
        outcome.code = 2;
        outcome.message = "output path is empty";
        return outcome;
    }

    std::vector<FramerateRecord> parsed;
    parsed.reserve(records.size());
    for (const std::string& line : records) {
        FramerateRecord record;
        if (parse_framerate_record(line, record)) {
            parsed.push_back(std::move(record));
        } else {
            outcome.rejected_records += 1;
        }
    }
    outcome.parsed_records = parsed.size();
    if (parsed.empty()) {
        outcome.code = 3;
        outcome.message = "no records parsed (" + std::to_string(outcome.rejected_records) +
                          " rejected)";
        return outcome;
    }

    std::ostringstream out;
    out << "{\n";
    out << "  \"schema\": \"" << kSchema << "\",\n";
    out << "  \"generated_at_unix_ms\": " << now_unix_ms() << ",\n";
    // 范围声明：这份证据只回答"厂商声明了什么"，不回答"真播会不会掉帧"。
    // 把这句话写进文件里，比写在文档里更难被忽略。
    out << "  \"scope\": \"vendor-declared capabilities from MediaCodecInfo.VideoCapabilities; "
           "these are claims, not measurements — a real playback run is still required (see "
           "56-p13a-design.md section 5)\",\n";
    out << "  \"record_count\": " << parsed.size() << ",\n";
    out << "  \"rejected_count\": " << outcome.rejected_records << ",\n";

    out << "  \"records\": [\n";
    for (std::size_t i = 0; i < parsed.size(); ++i) {
        const FramerateRecord& r = parsed[i];
        out << "    {\"name\": \"" << escape_json(r.name) << "\", \"mime\": \"" << escape_json(r.mime)
            << "\", \"profile\": " << r.profile << ", \"width\": " << r.width
            << ", \"height\": " << r.height << ", \"size_supported\": " << r.size_supported
            << ", \"max_fps\": " << r.max_fps << ", \"achievable_fps\": " << r.achievable_fps
            << "}" << (i + 1 == parsed.size() ? "\n" : ",\n");
    }
    out << "  ],\n";

    // verdict：直接回答"能不能"，而不是让读者自己在一堆行里找。
    out << "  \"verdict\": {\n";
    std::size_t index = 0;
    for (const Target& target : kTargets) {
        const double fps = best_fps_for(parsed, target.mime, target.width, target.height);
        // 三态：支持 / 不支持 / 平台没声明。**缺省不许写成 "no"** ——
        // "没说" 与 "说了不行" 是两件事，混起来会让人以为片子选错了。
        const char* answer = (fps < 0.0) ? "unknown" : (fps + 1e-9 >= target.fps ? "yes" : "no");
        out << "    \"" << target.label << "\": {\"answer\": \"" << answer
            << "\", \"best_fps\": " << fps << "}" << (++index == std::size(kTargets) ? "\n" : ",\n");
    }
    out << "  }\n";
    out << "}\n";

    std::ofstream file(output_path, std::ios::binary | std::ios::trunc);
    if (!file) {
        outcome.code = 4;
        outcome.message = "cannot open output file: " + output_path;
        return outcome;
    }
    const std::string text = out.str();
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    file.flush();
    if (!file) {
        outcome.code = 4;
        outcome.message = "write failed: " + output_path;
        return outcome;
    }

    outcome.ok = true;
    outcome.code = 0;
    return outcome;
}

}  // namespace kivo::video::mediacodec
