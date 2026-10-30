#include "video/video_plane/probe/software_baseline.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <algorithm>
#include <cstring>
#include <vector>

#include "video/video_plane/probe/media_source.hpp"
#include "video/video_plane/probe/probe_clock.hpp"

namespace kivo::video::video_plane::probe {
namespace {

[[nodiscard]] auto ff_error_text(const int code) -> std::string {
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    if (av_strerror(code, buffer, sizeof(buffer)) < 0) {
        return "error " + std::to_string(code);
    }
    return buffer;
}

}  // namespace

auto measure_software_baseline(const ProbeOptions& options, DecodeMetrics& out, std::string& error)
    -> bool {
    MediaSource source;
    if (!source.open(options.input_path, error)) {
        return false;
    }
    const VideoStreamInfo& info = source.video_stream();

    const AVCodec* codec = avcodec_find_decoder_by_name(info.codec_name.c_str());
    if (codec == nullptr) {
        error = "software decoder not available for " + info.codec_name;
        return false;
    }

    AVCodecContext* context = avcodec_alloc_context3(codec);
    if (context == nullptr) {
        error = "avcodec_alloc_context3 failed";
        return false;
    }
    context->width = info.width;
    context->height = info.height;
    if (info.extradata != nullptr && info.extradata_size > 0) {
        const std::size_t padding = AV_INPUT_BUFFER_PADDING_SIZE;
        context->extradata =
            static_cast<std::uint8_t*>(av_mallocz(info.extradata_size + padding));
        if (context->extradata == nullptr) {
            avcodec_free_context(&context);
            error = "extradata allocation failed";
            return false;
        }
        std::memcpy(context->extradata, info.extradata, info.extradata_size);
        context->extradata_size = static_cast<int>(info.extradata_size);
    }

    if (avcodec_open2(context, codec, nullptr) < 0) {
        avcodec_free_context(&context);
        error = "avcodec_open2 failed for software baseline";
        return false;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    if (packet == nullptr || frame == nullptr) {
        av_packet_free(&packet);
        av_frame_free(&frame);
        avcodec_free_context(&context);
        error = "frame/packet allocation failed";
        return false;
    }

    out.stage = "p11-a2-software-baseline";
    out.input_path = options.input_path;
    out.codec = info.codec_name;
    out.backend = "software";
    out.device_name = "cpu";
    out.width = info.width;
    out.height = info.height;
    out.bit_depth = info.bit_depth;
    out.pixel_format = info.container_pixel_format;

    std::int32_t frames = 0;
    std::int32_t packets = 0;
    std::vector<double> step_ms;
    step_ms.reserve(static_cast<std::size_t>(options.max_frames) + 16);

    const double cpu_before = process_cpu_seconds();
    const unsigned long long cycles_before = process_cycle_count();
    const double wall_before = steady_seconds();

    const auto drain = [&](const double step_start) -> bool {
        bool produced = false;
        for (;;) {
            const int receive = avcodec_receive_frame(context, frame);
            if (receive == AVERROR(EAGAIN) || receive == AVERROR_EOF) {
                break;
            }
            if (receive < 0) {
                return false;
            }
            ++frames;
            produced = true;
            av_frame_unref(frame);
            if (frames >= options.max_frames) {
                break;
            }
        }
        if (produced) {
            step_ms.push_back((steady_seconds() - step_start) * 1000.0);
        }
        return true;
    };

    bool ok = true;
    while (frames < options.max_frames) {
        const ReadStatus read = source.read_next(error);
        if (read == ReadStatus::EndOfStream) {
            avcodec_send_packet(context, nullptr);
            drain(steady_seconds());
            break;
        }
        if (read == ReadStatus::Failed) {
            ok = false;
            break;
        }

        const SourcePacketView& view = source.current_packet();
        av_packet_unref(packet);
        if (av_new_packet(packet, static_cast<int>(view.size)) < 0) {
            error = "av_new_packet failed";
            ok = false;
            break;
        }
        std::memcpy(packet->data, view.data, view.size);
        packet->pts = view.pts;
        packet->dts = view.dts;

        const double step_start = steady_seconds();
        const int send = avcodec_send_packet(context, packet);
        ++packets;
        if (send < 0 && send != AVERROR(EAGAIN)) {
            error = "software avcodec_send_packet failed: " + ff_error_text(send);
            ok = false;
            break;
        }
        if (!drain(step_start)) {
            error = "software avcodec_receive_frame failed";
            ok = false;
            break;
        }
    }

    const double wall_after = steady_seconds();
    const double cpu_after = process_cpu_seconds();
    const unsigned long long cycles_after = process_cycle_count();

    out.decoded_frames = frames;
    out.submitted_packets = packets;
    out.elapsed_ms = (wall_after - wall_before) * 1000.0;
    out.wall_time_ms = out.elapsed_ms;
    out.cpu_time_ms = (cpu_after - cpu_before) * 1000.0;
    out.cpu_cycles = static_cast<double>(cycles_after - cycles_before);
    out.fps = out.elapsed_ms > 0.0 ? static_cast<double>(frames) * 1000.0 / out.elapsed_ms : 0.0;
    out.hardware_decoded = false;
    out.ten_bit = info.bit_depth >= 10;

    if (!step_ms.empty()) {
        std::sort(step_ms.begin(), step_ms.end());
        double sum = 0.0;
        for (const double value : step_ms) {
            sum += value;
        }
        out.step_ms_avg = sum / static_cast<double>(step_ms.size());
        out.step_ms_p50 = percentile(step_ms, 50.0);
        out.step_ms_p99 = percentile(step_ms, 99.0);
        out.step_ms_max = step_ms.back();
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&context);

    if (!ok) {
        out.failure_reason = error;
        return false;
    }
    return frames > 0;
}

}  // namespace kivo::video::video_plane::probe
