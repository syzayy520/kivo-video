#include "video/video_plane/probe/probe_report.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace kivo::video::video_plane::probe {
namespace {

[[nodiscard]] auto json_escape(const std::string& text) -> std::string {
    std::string out;
    out.reserve(text.size() + 8);
    for (const char ch : text) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    std::ostringstream hex;
                    hex << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(ch));
                    out += hex.str();
                } else {
                    out += ch;
                }
        }
    }
    return out;
}

[[nodiscard]] auto fixed(const double value, const int digits = 3) -> std::string {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(digits) << value;
    return stream.str();
}

[[nodiscard]] auto iso8601_now() -> std::string {
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &seconds);
#else
    localtime_r(&seconds, &local);
#endif
    std::ostringstream stream;
    stream << std::put_time(&local, "%Y-%m-%dT%H:%M:%S");
    return stream.str();
}

}  // namespace

auto percentile(const std::vector<double>& sorted_samples, const double p) noexcept -> double {
    if (sorted_samples.empty()) {
        return 0.0;
    }
    const double clamped = std::clamp(p, 0.0, 100.0);
    const auto count = static_cast<double>(sorted_samples.size());
    auto index = static_cast<std::size_t>(clamped / 100.0 * count);
    if (index >= sorted_samples.size()) {
        index = sorted_samples.size() - 1;
    }
    return sorted_samples[index];
}

auto metrics_to_json(const DecodeMetrics& m) -> std::string {
    std::ostringstream json;
    json << "{\n";
    json << "  \"stage\": \"" << json_escape(m.stage) << "\",\n";
    json << "  \"captured_at\": \"" << iso8601_now() << "\",\n";
    json << "  \"input\": \"" << json_escape(m.input_path) << "\",\n";
    json << "  \"codec\": \"" << json_escape(m.codec) << "\",\n";
    json << "  \"backend\": \"" << json_escape(m.backend) << "\",\n";
    json << "  \"device\": \"" << json_escape(m.device_name) << "\",\n";
    json << "  \"pixel_format\": \"" << json_escape(m.pixel_format) << "\",\n";
    json << "  \"width\": " << m.width << ",\n";
    json << "  \"height\": " << m.height << ",\n";
    json << "  \"bit_depth\": " << m.bit_depth << ",\n";
    json << "  \"decoded_frames\": " << m.decoded_frames << ",\n";
    json << "  \"submitted_packets\": " << m.submitted_packets << ",\n";
    json << "  \"elapsed_ms\": " << fixed(m.elapsed_ms) << ",\n";
    json << "  \"fps\": " << fixed(m.fps) << ",\n";
    json << "  \"cpu_time_ms\": " << fixed(m.cpu_time_ms) << ",\n";
    json << "  \"cpu_cycles\": " << fixed(m.cpu_cycles, 0) << ",\n";
    json << "  \"cpu_ratio\": " << fixed(m.cpu_ratio, 4) << ",\n";
    json << "  \"wall_time_ms\": " << fixed(m.wall_time_ms) << ",\n";
    json << "  \"step_ms_avg\": " << fixed(m.step_ms_avg) << ",\n";
    json << "  \"step_ms_p50\": " << fixed(m.step_ms_p50) << ",\n";
    json << "  \"step_ms_p99\": " << fixed(m.step_ms_p99) << ",\n";
    json << "  \"step_ms_max\": " << fixed(m.step_ms_max) << ",\n";
    json << "  \"hardware_decoded\": " << (m.hardware_decoded ? "true" : "false") << ",\n";
    json << "  \"ten_bit\": " << (m.ten_bit ? "true" : "false") << ",\n";
    json << "  \"failure_reason\": \"" << json_escape(m.failure_reason) << "\"\n";
    json << "}\n";
    return json.str();
}

auto metrics_to_text(const DecodeMetrics& m) -> std::string {
    std::ostringstream text;
    text << "  backend        : " << m.backend << '\n';
    text << "  device         : " << (m.device_name.empty() ? "<n/a>" : m.device_name) << '\n';
    text << "  codec          : " << m.codec << "  " << m.width << "x" << m.height
         << "  " << m.bit_depth << "-bit\n";
    text << "  pixel format   : "
         << (m.pixel_format.empty() ? "<none>" : m.pixel_format)
         << (m.ten_bit ? "  (10-bit preserved)" : "") << '\n';
    text << "  decoded frames : " << m.decoded_frames
         << "   from " << m.submitted_packets << " packets\n";
    text << "  fps            : " << fixed(m.fps, 2) << '\n';
    // CPU-side cost of one submit->drain step. Hardware decode itself runs on the
    // GPU asynchronously, so this is NOT frame decode latency; it is the CPU
    // overhead the backend adds per packet, which is what the goal constrains.
    text << "  submit cost    : avg " << fixed(m.step_ms_avg, 3)
         << " ms, p50 " << fixed(m.step_ms_p50, 3)
         << ", p99 " << fixed(m.step_ms_p99, 3)
         << ", max " << fixed(m.step_ms_max, 3) << '\n';
    text << "  cpu cycles     : " << fixed(m.cpu_cycles, 0) << '\n';
    if (m.cpu_ratio > 0.0) {
        text << "  cpu ratio      : " << fixed(m.cpu_ratio * 100.0, 2)
             << "% of software cycles (goal <= 10%)\n";
    }
    text << "  wall time      : " << fixed(m.wall_time_ms, 1) << " ms\n";
    if (!m.failure_reason.empty()) {
        text << "  FAILURE        : " << m.failure_reason << '\n';
    }
    return text.str();
}

auto write_metrics_file(const std::string& path, const DecodeMetrics& metrics) -> bool {
    const std::filesystem::path target(path);
    if (target.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(target.parent_path(), ec);
        if (ec) {
            return false;
        }
    }
    std::ofstream file(target, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file << metrics_to_json(metrics);
    return file.good();
}

}  // namespace kivo::video::video_plane::probe
