#include "video/video_plane/probe/audio/audio_file_probe_report.hpp"

#include <cstdio>
#include <string_view>

#include "video/video_plane/backends/aaudio/device/aaudio_format_names.hpp"
#include "video/video_plane/core/audio/clock/audio_clock_types.hpp"

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

std::string fixed(double value, int decimals) {
    char buffer[64] = {};
    std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
    return buffer;
}

std::string bool_text(bool value) { return value ? "true" : "false"; }

const char* requested_format_name(aaudio::AaudioStreamConfig::SampleFormat format) {
    return (format == aaudio::AaudioStreamConfig::SampleFormat::kInt16) ? "pcm_i16" : "pcm_float";
}

double ratio(std::int64_t numerator, std::int64_t denominator) {
    if (denominator <= 0) {
        return 0.0;
    }
    return static_cast<double>(numerator) / static_cast<double>(denominator);
}

}  // namespace

std::string render_audio_file_probe_json(const AudioFileProbeReport& report) {
    const auto& open = report.open;
    const auto& stats = report.stats;
    const auto& feed = report.feed;

    // 期望帧数按**容器声明的采样率**算，而不是按平台协商后的采样率。
    // 只有这样，采样率被平台改掉时 render_ratio 才会明显偏离 1.0 并被看见；
    // 用协商值当分母等于把错误除掉了，比值永远漂亮。
    const std::int64_t expected_frames =
        (report.container_duration_us > 0 && report.declared_sample_rate > 0)
            ? report.container_duration_us * report.declared_sample_rate / 1000000
            : 0;

    std::string json;
    json.reserve(2000);

    json += "{\n";
    json += "  \"schema\": \"kivo.p12c.audio-file-probe/1\",\n";

    json += "  \"device\": {\n";
    json += "    \"model\": " + quoted(report.device_model) + ",\n";
    json += "    \"abi\": " + quoted(report.device_abi) + ",\n";
    json += "    \"sdk\": " + std::to_string(report.device_sdk) + "\n";
    json += "  },\n";

    json += "  \"source\": {\n";
    json += "    \"label\": " + quoted(report.label) + ",\n";
    json += "    \"bytes\": " + std::to_string(report.file_bytes) + ",\n";
    json += "    \"mime\": " + quoted(report.mime) + ",\n";
    json += "    \"declared_sample_rate\": " + std::to_string(report.declared_sample_rate) + ",\n";
    json += "    \"declared_channel_count\": " + std::to_string(report.declared_channel_count) + ",\n";
    json += "    \"container_duration_us\": " + std::to_string(report.container_duration_us) + "\n";
    json += "  },\n";

    json += "  \"decode\": {\n";
    json += "    \"codec_name\": " + quoted(report.codec_name) + ",\n";
    json += "    \"output_sample_rate\": " + std::to_string(report.output_format.sample_rate) + ",\n";
    json += "    \"output_channel_count\": " + std::to_string(report.output_format.channel_count) + ",\n";
    json += "    \"output_encoding\": " +
            quoted(core::audio::pcm_encoding_name(report.output_format.encoding)) + ",\n";
    json += "    \"output_format_diverged\": " + bool_text(report.output_format_diverged) + ",\n";
    json += "    \"packets_submitted\": " + std::to_string(feed.packets_submitted) + ",\n";
    json += "    \"chunks_received\": " + std::to_string(feed.chunks_received) + ",\n";
    json += "    \"frames_decoded\": " + std::to_string(feed.frames_decoded) + ",\n";
    json += "    \"frames_written\": " + std::to_string(feed.frames_written) + ",\n";
    json += "    \"frames_dropped\": " + std::to_string(feed.frames_dropped) + ",\n";
    json += "    \"ring_full_waits\": " + std::to_string(feed.ring_full_waits) + ",\n";
    json += "    \"ring_capacity_frames\": " + std::to_string(report.ring_capacity_frames) + ",\n";
    json += "    \"reached_eos\": " + bool_text(feed.reached_eos) + ",\n";
    json += "    \"eos_flag_seen\": " + bool_text(feed.eos_flag_seen) + ",\n";
    json += "    \"decode_ratio\": " + fixed(ratio(feed.frames_decoded, expected_frames), 3) + ",\n";
    json += "    \"error\": " + quoted(feed.error) + "\n";
    json += "  },\n";

    json += "  \"requested\": {\n";
    json += "    \"sample_rate\": " + std::to_string(report.config.sample_rate) + ",\n";
    json += "    \"channel_count\": " + std::to_string(report.config.channel_count) + ",\n";
    json += "    \"format\": " + quoted(requested_format_name(report.config.format)) + ",\n";
    json += "    \"prefer_exclusive\": " + bool_text(report.config.prefer_exclusive) + ",\n";
    json += "    \"prefer_low_latency\": " + bool_text(report.config.prefer_low_latency) + "\n";
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

    json += "  \"output\": {\n";
    json += "    \"prefill_reached\": " + bool_text(report.prefill_reached) + ",\n";
    json += "    \"callbacks\": " + std::to_string(stats.callbacks) + ",\n";
    json += "    \"frames_rendered\": " + std::to_string(stats.frames_rendered) + ",\n";
    json += "    \"frames_silenced\": " + std::to_string(stats.frames_silenced) + ",\n";
    json += "    \"first_audible_frame\": " + std::to_string(stats.first_audible_frame) + ",\n";
    json += "    \"silence_head_frames\": " + std::to_string(stats.silence_head_frames) + ",\n";
    json += "    \"silence_tail_frames\": " + std::to_string(stats.silence_tail_frames) + ",\n";
    json += "    \"xrun_count\": " + std::to_string(stats.xrun_count) + ",\n";
    json += "    \"wall_millis\": " + std::to_string(stats.wall_millis) + ",\n";
    json += "    \"expected_frames\": " + std::to_string(expected_frames) + ",\n";
    json += "    \"render_ratio\": " + fixed(ratio(stats.frames_rendered, expected_frames), 3) + ",\n";
    json += "    \"error\": " + quoted(stats.error) + "\n";
    json += "  },\n";

    // 时钟段。`source` 用"实际发布次数最多的那一档"，不是"配置上打算用哪个"。
    const auto& clock = report.clock;
    const std::int64_t clock_span_ns = report.clock_end_ns - report.clock_start_ns;
    // 同样帧数本该对应多长时间：这是判断"时钟有没有跟着设备走"的基准。
    // 时钟若靠墙钟硬凑，这个比值会偏离 1 且随运行拉大。
    const std::int64_t rendered_span_ns =
        (open.sample_rate > 0) ? stats.frames_rendered * 1000000000LL / open.sample_rate : 0;

    json += "  \"clock\": {\n";
    json += "    \"source\": " +
            quoted(core::audio::audio_clock_source_name(clock.dominant_source())) + ",\n";
    json += "    \"queries\": " + std::to_string(clock.queries) + ",\n";
    json += "    \"startup_millis\": " + std::to_string(report.startup_millis) + ",\n";
    json += "    \"first_ok_frame_position\": " + std::to_string(clock.first_ok_frame_position) + ",\n";
    json += "    \"first_ok_time_nanos\": " + std::to_string(clock.first_ok_time_nanos) + ",\n";
    json += "    \"timestamp_ok\": " + std::to_string(clock.timestamp_ok) + ",\n";
    json += "    \"timestamp_not_started\": " + std::to_string(clock.timestamp_not_started) + ",\n";
    json += "    \"timestamp_fail\": " + std::to_string(clock.timestamp_fail) + ",\n";
    json += "    \"fallback_frames_read\": " + std::to_string(clock.fallback_frames_read) + ",\n";
    json += "    \"fallback_frames_written\": " + std::to_string(clock.fallback_frames_written) + ",\n";
    json += "    \"stale_freezes\": " + std::to_string(clock.stale_freezes) + ",\n";
    json += "    \"monotonic_rejects\": " + std::to_string(clock.monotonic_rejects) + ",\n";
    json += "    \"jumps_clamped\": " + std::to_string(clock.jumps_clamped) + ",\n";
    json += "    \"latency_frames\": " + std::to_string(clock.latency_frames) + ",\n";
    json += "    \"max_extrapolation_ns\": " + std::to_string(clock.max_extrapolation_ns) + ",\n";
    json += "    \"clock_start_ns\": " + std::to_string(report.clock_start_ns) + ",\n";
    json += "    \"clock_end_ns\": " + std::to_string(report.clock_end_ns) + ",\n";
    json += "    \"clock_span_ns\": " + std::to_string(clock_span_ns) + ",\n";
    json += "    \"rendered_span_ns\": " + std::to_string(rendered_span_ns) + ",\n";
    json += "    \"span_ratio\": " + fixed(ratio(clock_span_ns, rendered_span_ns), 4) + "\n";
    json += "  },\n";

    json += "  \"wall_millis\": " + std::to_string(report.wall_millis) + ",\n";
    json += "  \"failure\": " + quoted(report.failure) + "\n";
    json += "}\n";

    return json;
}

}  // namespace kivo::video::video_plane::probe
