#include "video/video_plane/probe/audio/aaudio_probe_report.hpp"

#include <cstdio>
#include <string_view>

#include "video/video_plane/backends/aaudio/device/aaudio_format_names.hpp"

namespace kivo::video::video_plane::probe {
namespace {

std::string escape_json(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (const char ch : text) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: {
                const auto raw = static_cast<unsigned char>(ch);
                if (raw < 0x20U) {
                    static constexpr char kHex[] = "0123456789abcdef";
                    out += "\\u00";
                    out += kHex[(raw >> 4U) & 0x0FU];
                    out += kHex[raw & 0x0FU];
                } else {
                    out += ch;
                }
                break;
            }
        }
    }
    return out;
}

std::string quoted(std::string_view text) { return "\"" + escape_json(text) + "\""; }

const char* requested_format_name(aaudio::AaudioStreamConfig::SampleFormat format) {
    return (format == aaudio::AaudioStreamConfig::SampleFormat::kInt16) ? "pcm_i16" : "pcm_float";
}

std::string fixed(double value, int decimals) {
    char buffer[64] = {};
    std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
    return buffer;
}

std::string bool_text(bool value) { return value ? "true" : "false"; }

}  // namespace

std::string render_aaudio_probe_json(const AaudioProbeReport& report) {
    const auto& config = report.config;
    const auto& open = report.open;
    const auto& stats = report.stats;

    const std::int64_t sample_rate = (open.sample_rate > 0) ? open.sample_rate : config.sample_rate;
    const std::int64_t expected_frames = sample_rate * report.requested_millis / 1000;
    const double render_ratio =
        (expected_frames > 0)
            ? static_cast<double>(stats.frames_rendered) / static_cast<double>(expected_frames)
            : 0.0;

    std::string json;
    json.reserve(1400);

    json += "{\n";
    json += "  \"schema\": \"kivo.p12c.aaudio-probe/2\",\n";

    json += "  \"device\": {\n";
    json += "    \"model\": " + quoted(report.device_model) + ",\n";
    json += "    \"abi\": " + quoted(report.device_abi) + ",\n";
    json += "    \"sdk\": " + std::to_string(report.device_sdk) + "\n";
    json += "  },\n";

    json += "  \"tone\": {\n";
    json += "    \"frequency_hz\": " + fixed(report.tone_frequency_hz, 1) + ",\n";
    json += "    \"requested_millis\": " + std::to_string(report.requested_millis) + "\n";
    json += "  },\n";

    json += "  \"requested\": {\n";
    json += "    \"sample_rate\": " + std::to_string(config.sample_rate) + ",\n";
    json += "    \"channel_count\": " + std::to_string(config.channel_count) + ",\n";
    json += "    \"format\": " + quoted(requested_format_name(config.format)) + ",\n";
    json += "    \"prefer_exclusive\": " + bool_text(config.prefer_exclusive) + ",\n";
    json += "    \"prefer_low_latency\": " + bool_text(config.prefer_low_latency) + ",\n";
    json += "    \"buffer_capacity_frames\": " + std::to_string(config.buffer_capacity_frames) + "\n";
    json += "  },\n";

    json += "  \"negotiated\": {\n";
    json += "    \"sample_rate\": " + std::to_string(open.sample_rate) + ",\n";
    json += "    \"channel_count\": " + std::to_string(open.channel_count) + ",\n";
    json += "    \"format\": " + quoted(aaudio::platform_format_name(open.platform_format)) + ",\n";
    json += "    \"sharing_mode\": " + quoted(aaudio::sharing_mode_name(open.platform_sharing_mode)) + ",\n";
    json += "    \"performance_mode\": " +
            quoted(aaudio::performance_mode_name(open.platform_performance_mode)) + ",\n";
    json += "    \"frames_per_burst\": " + std::to_string(open.frames_per_burst) + ",\n";
    json += "    \"buffer_capacity_frames\": " + std::to_string(open.buffer_capacity_frames) + "\n";
    json += "  },\n";

    json += "  \"downgrade_reason\": " + quoted(open.downgrade_reason) + ",\n";

    // 供给侧事实：欠载到底发生在"设备没取走"还是"我们没供上"，
    // 只有把生产侧计数和渲染侧 frames_silenced 摆在一起才分得清。
    json += "  \"feed\": {\n";
    json += "    \"mode\": " + quoted(report.feed_mode) + ",\n";
    json += "    \"producer_speed\": " + fixed(report.producer_speed, 3) + ",\n";
    json += "    \"ring_capacity_frames\": " + std::to_string(report.ring_capacity_frames) + ",\n";
    json += "    \"frames_pushed\": " + std::to_string(report.frames_pushed) + ",\n";
    json += "    \"producer_short_writes\": " + std::to_string(report.producer_short_writes) + "\n";
    json += "  },\n";

    json += "  \"open\": {\n";
    json += "    \"ok\": " + bool_text(open.ok()) + ",\n";
    json += "    \"code\": " + std::to_string(static_cast<std::int32_t>(open.code)) + ",\n";
    json += "    \"message\": " + quoted(open.message) + "\n";
    json += "  },\n";

    json += "  \"start\": {\n";
    json += "    \"ok\": " + bool_text(report.start.ok()) + ",\n";
    json += "    \"code\": " + std::to_string(static_cast<std::int32_t>(report.start.code)) + ",\n";
    json += "    \"message\": " + quoted(report.start.message) + "\n";
    json += "  },\n";

    json += "  \"stats\": {\n";
    json += "    \"callbacks\": " + std::to_string(stats.callbacks) + ",\n";
    json += "    \"frames_rendered\": " + std::to_string(stats.frames_rendered) + ",\n";
    json += "    \"frames_silenced\": " + std::to_string(stats.frames_silenced) + ",\n";
    json += "    \"xrun_count\": " + std::to_string(stats.xrun_count) + ",\n";
    json += "    \"wall_millis\": " + std::to_string(stats.wall_millis) + ",\n";
    json += "    \"expected_frames\": " + std::to_string(expected_frames) + ",\n";
    json += "    \"render_ratio\": " + fixed(render_ratio, 3) + ",\n";
    json += "    \"started\": " + bool_text(stats.started) + ",\n";
    json += "    \"error\": " + quoted(stats.error) + "\n";
    json += "  }\n";
    json += "}\n";

    return json;
}

}  // namespace kivo::video::video_plane::probe
