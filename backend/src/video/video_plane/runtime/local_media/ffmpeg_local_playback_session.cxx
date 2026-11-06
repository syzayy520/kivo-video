#include "video/video_plane/runtime/local_media/ffmpeg_local_playback_session.hpp"

#include <chrono>
#include <cstdio>
#include <utility>

namespace kivo::video::video_plane::runtime::local_media {
namespace {

std::string escape_json(const std::string& in) {
    std::string out;
    for (char c : in) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

std::int64_t steady_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

}  // namespace

FfmpegLocalPlaybackSession::~FfmpegLocalPlaybackSession() {
    std::string evidence;
    // 析构里证据没人接：显式弃值（nodiscard 是给正常 stop 路径的）。
    (void)stop(evidence);
}

FfmpegSessionOutcome FfmpegLocalPlaybackSession::open(const Config& config,
                                                      IFfFrameSink* sink) {
    if (running_.load(std::memory_order_relaxed)) {
        return {FfmpegSessionOutcome::Code::kAlreadyRunning, "session is running"};
    }
    if (config.fd < 0) {
        return {FfmpegSessionOutcome::Code::kOpenFailed, "fd is negative"};
    }
    if (sink == nullptr) {
        return {FfmpegSessionOutcome::Code::kOpenFailed, "sink is null"};
    }
    config_ = config;
    sink_ = sink;
    opened_ = true;
    return {};
}

FfmpegSessionOutcome FfmpegLocalPlaybackSession::start() {
    if (!opened_) {
        return {FfmpegSessionOutcome::Code::kNotStarted, "open() first"};
    }
    if (running_.load(std::memory_order_relaxed)) {
        return {FfmpegSessionOutcome::Code::kAlreadyRunning, "session is running"};
    }
    running_.store(true, std::memory_order_relaxed);
    worker_ = std::thread([this] { run_loop_(); });
    return {};
}

FfmpegSessionOutcome FfmpegLocalPlaybackSession::stop(std::string& evidence_json) {
    if (!opened_) {
        return {FfmpegSessionOutcome::Code::kNotStarted, "nothing to stop"};
    }
    if (worker_.joinable()) {
        running_.store(false, std::memory_order_relaxed);
        worker_.join();
    }
    opened_ = false;

    // ---- 证据（kivo.p211.decode/1）：与硬解证据面同构的计数 + 差异如实出版 ----
    std::string json = "{\n";
    json += "  \"schema\": \"kivo.p211.decode/1\",\n";
    json += "  \"decoder\": { \"backend\": \"ffmpeg\", \"codec\": \"" +
            escape_json(codec_name_) + "\" },\n";
    json += "  \"media\": \"" + escape_json(config_.label) + "\",\n";
    json += "  \"container\": \"" + escape_json(container_) + "\",\n";
    json += "  \"source\": { \"width\": " + std::to_string(source_width_) +
            ", \"height\": " + std::to_string(source_height_) + " },\n";
    json += "  \"counters\": {\n";
    json += "    \"packets_read\": " + std::to_string(counters_.packets_read) + ",\n";
    json += "    \"video_packets\": " + std::to_string(counters_.video_packets) + ",\n";
    json += "    \"keyframes\": " + std::to_string(counters_.keyframes) + ",\n";
    json += "    \"frames_decoded\": " + std::to_string(counters_.frames_decoded) + ",\n";
    json += "    \"frames_rendered\": " + std::to_string(counters_.frames_rendered) + ",\n";
    json += "    \"render_failures\": " + std::to_string(counters_.render_failures) + ",\n";
    json += "    \"read_errors\": " + std::to_string(counters_.read_errors) + "\n";
    json += "  },\n";
    json += "  \"timing\": {\n";
    json += "    \"wall_us\": " + std::to_string(counters_.wall_us) + ",\n";
    json += "    \"decode_ms_total\": " + std::to_string(counters_.decode_ms_total) + "\n";
    json += "  },\n";
    json += "  \"render\": { \"sink\": \"" + std::string(sink_ ? sink_->name() : "none") +
            "\" },\n";
    json += "  \"routing\": { \"backend_decision\": \"" +
            escape_json(config_.routing_decision) + "\", \"reason\": \"" +
            escape_json(config_.routing_reason) + "\" },\n";
    json += "  \"pacing\": \"pts_sleep_simple\",\n";
    json += "  \"end_reason\": \"" + escape_json(end_reason_) + "\",\n";
    json += "  \"first_error\": " +
            std::string(first_error_.empty() ? "null" : "\"" + escape_json(first_error_) + "\"") +
            "\n";
    json += "}\n";
    evidence_json = std::move(json);
    return {};
}

void FfmpegLocalPlaybackSession::run_loop_() {
    // **FFmpeg 全生命周期都在本线程**：建（demux/解码器）、解、毁一律不跨线程。
    // 跨线程使用 FFmpeg 句柄（主线程建、工作线程用）会在 Windows 的
    // MSVC-Debug + FFmpeg-DLL 组合上把退出期炸成 abort（exit=3）——
    // 二分实验记录在本测试文件（--trio-only/--decode-inline/--thread-only/
    // --thread-ffmpeg-use 四段）。单线程持有一切后，桌面测试干净退出。
    const std::int64_t started_us = steady_us();
    std::int64_t first_pts_us = -1;
    std::int64_t pace_origin_us = 0;
    bool completed = false;
    bool error_reported = false;

    // 1. fd 回调 → 自定义 AVIO → demux（本线程）。
    fd_source_.fd = config_.fd;
    fd_source_.offset = config_.offset;
    fd_source_.length = config_.length;
    fd_source_.cursor = 0;
    auto callbacks = make_ff_fd_callbacks(&fd_source_);
    kivo::cinema_engine::FfmpegInputConfig io_config;
    io_config.callbacks = callbacks;
    format_ = kivo::cinema_engine::FfmpegAdapter::open_custom_io(io_config);
    double duration = 0;
    std::int64_t bitrate = 0;
    if (format_.native == nullptr ||
        !kivo::cinema_engine::FfmpegAdapter::probe_container(format_, container_,
                                                             duration, bitrate)) {
        counters_.wall_us = steady_us() - started_us;
        end_reason_ = "error: demux open/probe failed";
        if (format_.native != nullptr) {
            kivo::cinema_engine::FfmpegAdapter::free_format_context(format_);
        }
        running_.store(false, std::memory_order_relaxed);
        return;
    }
    std::vector<int> video_indices;
    std::vector<int> audio_indices;
    std::vector<int> subtitle_indices;
    if (!kivo::cinema_engine::FfmpegAdapter::enumerate_streams(
            format_, video_indices, audio_indices, subtitle_indices) ||
        video_indices.empty()) {
        counters_.wall_us = steady_us() - started_us;
        end_reason_ = "error: no video stream";
        kivo::cinema_engine::FfmpegAdapter::free_format_context(format_);
        running_.store(false, std::memory_order_relaxed);
        return;
    }
    video_stream_ = video_indices.front();
    kivo::cinema_engine::FfmpegAdapter::get_stream_codec_name(format_, video_stream_,
                                                              codec_name_);
    std::string source_pixel_format;
    kivo::cinema_engine::FfmpegAdapter::get_stream_video_info(
        format_, video_stream_, source_width_, source_height_, source_fps_,
        source_pixel_format);

    auto codec_handle =
        kivo::cinema_engine::FfmpegAdapter::create_codec_context(format_, video_stream_);
    if (codec_handle.native == nullptr ||
        !decoder_.adopt_codec_context(codec_handle.native, codec_name_, "p211_session")) {
        counters_.wall_us = steady_us() - started_us;
        end_reason_ = "error: decoder open failed (" + decoder_.error_code() + ")";
        kivo::cinema_engine::FfmpegAdapter::free_format_context(format_);
        running_.store(false, std::memory_order_relaxed);
        return;
    }

    // 2. 解码循环（本线程）。
    while (running_.load(std::memory_order_relaxed)) {
        auto read_result = kivo::cinema_engine::FfmpegAdapter::read_packet(format_);
        if (read_result.eof) {
            completed = true;
            break;
        }
        ++counters_.packets_read;
        if (!read_result.success) {
            ++counters_.read_errors;
            if (!error_reported && !read_result.error_message.empty()) {
                first_error_ = read_result.error_message;
                error_reported = true;
            }
            continue;
        }
        if (read_result.packet.stream_index != video_stream_) {
            continue;
        }
        ++counters_.video_packets;
        if (read_result.packet.is_key_frame) {
            ++counters_.keyframes;
        }
        // 桌面 pump 的转换逐字段照抄（local_video_runtime_pump.cpp:201-211）。
        kivo::cinema_engine::KivoPacket packet;
        packet.stream_index = read_result.packet.stream_index;
        packet.pts = read_result.packet.pts;
        packet.dts = read_result.packet.dts;
        packet.duration = read_result.packet.duration;
        packet.is_key_frame = read_result.packet.is_key_frame;
        packet.data = read_result.packet_data;
        for (auto& side : read_result.side_data) {
            packet.side_data.push_back({side.type, std::move(side.data)});
        }

        const auto decode_started = std::chrono::steady_clock::now();
        auto result = decoder_.decode_video_rgba(packet, "p211_session");
        counters_.decode_ms_total +=
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() -
                                                      decode_started)
                .count();
        if (!result.success || result.needs_more_input || result.eof) {
            continue;
        }
        if (result.frame.width <= 0 || result.frame.height <= 0 ||
            result.frame.frame_data.empty()) {
            continue;
        }
        ++counters_.frames_decoded;

        // 节拍（simple pts sleep）：首帧起锚，之后每帧睡到"起点 + 媒体时间差"。
        // 差异如实出版（pacing 字段）；比硬解的 FramePacer 粗，M1 不追求等价。
        std::int64_t media_us = 0;
        const std::int64_t pts =
            result.frame.pts != 0 ? result.frame.pts : read_result.packet.pts;
        if (kivo::cinema_engine::FfmpegAdapter::rescale_stream_timestamp_us(
                format_, video_stream_, pts, media_us) &&
            media_us > 0) {
            if (first_pts_us < 0) {
                first_pts_us = media_us;
                pace_origin_us = steady_us();
            }
            const std::int64_t due = pace_origin_us + (media_us - first_pts_us);
            std::int64_t now = steady_us();
            if (due > now) {
                std::this_thread::sleep_for(std::chrono::microseconds(due - now));
            }
        }

        std::string render_error;
        if (sink_ != nullptr && sink_->render(result.frame, render_error)) {
            ++counters_.frames_rendered;
        } else {
            ++counters_.render_failures;
            if (!error_reported && !render_error.empty()) {
                first_error_ = render_error;
                error_reported = true;
            }
        }
    }

    // 3. 清理（本线程）。
    decoder_.close();
    if (format_.native != nullptr) {
        kivo::cinema_engine::FfmpegAdapter::free_format_context(format_);
    }
    counters_.wall_us = steady_us() - started_us;
    end_reason_ = completed ? "completed" : "interrupted";
    // 自然 EOS 也算停了：is_running() 的语义是"线程还活着"，
    // 不清标志会让调用方在 EOS 后白等到 stop()（桌面测试当场暴露）。
    running_.store(false, std::memory_order_relaxed);
}

}  // namespace kivo::video::video_plane::runtime::local_media
