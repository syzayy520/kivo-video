// sd_probe_main.cxx — 211 软解兜底探针（kivo_sd_probe）。
//
// 回答一个问题：**FFmpeg 静态库在 Android 真机上，"fd 回调 → demux → 软解 →
// RGBA 帧" 这条链路能不能走通、每个环节长什么样**。
// 它不碰 Surface（呈现集成是软解会话的事），证据打 stdout
// （与 kivo_hdr_probe 同款约定：`kivo_sd_probe <media> > out.json`）。
//
// 证据 schema：kivo.p211.sd-probe/1。判据（设计稿 §五 AC1/AC2 的探针前置）：
// wmv2 与 rv10 两支测试片都能解出 ≥1 帧 RGBA，宽高与源一致、无 error。
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include "kivo/cinema_engine/demux_core/kivo_packet.hpp"
#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"
#include "video/video_plane/runtime/local_media/ffmpeg_fd_callbacks.hpp"

// 三段式诊断需要直呼 FFmpeg（只在这个探针里，不影响任何共享代码）。
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
}

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

void fail(const std::string& stage, const std::string& reason, int fd) {
    if (fd >= 0) {
#if defined(_WIN32)
        _close(fd);
#else
        close(fd);
#endif
    }
    std::printf("{\n  \"schema\": \"kivo.p211.sd-probe/1\",\n");
    std::printf("  \"ok\": false,\n  \"stage\": \"%s\",\n", escape_json(stage).c_str());
    std::printf("  \"reason\": \"%s\"\n", escape_json(reason).c_str());
    std::printf("}\n");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <media-path> [max-frames]\n", argv[0]);
        return 2;
    }
    const std::string media = argv[1];
    const int target_frames = argc >= 3 ? std::atoi(argv[2]) : 30;

    // 1. fd + 数据长度（探针以 shell 身份直接读 /data/local/tmp，路径可信；
    //    APK 会话走 JNI 传 fd，语义同款）。
#if defined(_WIN32)
    const int fd = static_cast<int>(_open(media.c_str(), _O_RDONLY | _O_BINARY));
#else
    const int fd = open(media.c_str(), O_RDONLY);
#endif
    if (fd < 0) {
        fail("open-fd", "cannot open " + media, -1);
        return 1;
    }
#if defined(_WIN32)
    const std::int64_t length = _lseeki64(fd, 0, SEEK_END);
#else
    const std::int64_t length = lseek(fd, 0, SEEK_END);
#endif

    // 2. fd 回调 → 自定义 AVIO → demux 打开。
    kivo::video::video_plane::runtime::local_media::FfFdSource source;
    source.fd = fd;
    source.offset = 0;
    source.length = length;
    auto callbacks =
        kivo::video::video_plane::runtime::local_media::make_ff_fd_callbacks(&source);
    kivo::cinema_engine::FfmpegInputConfig config;
    config.callbacks = callbacks;
    auto handle = kivo::cinema_engine::FfmpegAdapter::open_custom_io(config);
    if (handle.native == nullptr) {
        fail("demux-open", "open_custom_io returned a null format context", fd);
        return 1;
    }

    // 3. 容器与流枚举。
    std::string container;
    double duration = 0;
    std::int64_t bitrate = 0;
    std::string demux_error;
    if (!kivo::cinema_engine::FfmpegAdapter::probe_container(
            handle, container, duration, bitrate)) {
        demux_error = "probe_container failed";
    }
    std::vector<int> video_indices;
    std::vector<int> audio_indices;
    std::vector<int> subtitle_indices;
    if (demux_error.empty() &&
        !kivo::cinema_engine::FfmpegAdapter::enumerate_streams(
            handle, video_indices, audio_indices, subtitle_indices)) {
        demux_error = "enumerate_streams failed";
    }
    if (demux_error.empty() && video_indices.empty()) {
        demux_error = "no video stream in container";
    }
    if (!demux_error.empty()) {
        kivo::cinema_engine::FfmpegAdapter::free_format_context(handle);
        fail("demux-probe", demux_error, fd);
        return 1;
    }
    const int video_stream = video_indices.front();
    std::string codec;
    int width = 0;
    int height = 0;
    double fps = 0;
    std::string src_pixel;
    kivo::cinema_engine::FfmpegAdapter::get_stream_codec_name(handle, video_stream, codec);
    kivo::cinema_engine::FfmpegAdapter::get_stream_video_info(handle, video_stream, width,
                                                              height, fps, src_pixel);

    // 4. 软解循环：read → 转包 → decode_video_rgba。
    //
    // 解码器上下文必须**从流的 codecpar 创建**（width/height/extradata 全在里面）：
    // 裸 open-by-name 的 0×0 上下文过不了 avcodec_open2 的尺寸校验
    // （三段式诊断实锤：find=ok / alloc=ok / open2=-22 Invalid argument，
    //  stderr 是 `Picture size 0x0 is invalid`）。桌面 pump 同款流：
    // create_codec_context → adopt_codec_context（local_video_runtime_pump.cpp）。
    kivo::cinema_engine::RealSoftwareDecodeRuntime decoder;
    auto codec_handle =
        kivo::cinema_engine::FfmpegAdapter::create_codec_context(handle, video_stream);
    if (codec_handle.native == nullptr) {
        kivo::cinema_engine::FfmpegAdapter::free_format_context(handle);
        fail("decoder-open", "create_codec_context(" + codec + ") failed", fd);
        return 1;
    }
    if (!decoder.adopt_codec_context(codec_handle.native, codec, "p211_sd_probe")) {
        const std::string reason =
            "adopt_codec_context(" + codec + ") failed: " + decoder.error_code();
        kivo::cinema_engine::FfmpegAdapter::free_format_context(handle);
        fail("decoder-open", reason, fd);
        return 1;
    }

    int packets = 0;
    int video_packets = 0;
    int keyframes = 0;
    int decoded = 0;
    int out_width = 0;
    int out_height = 0;
    std::size_t first_frame_bytes = 0;
    std::string out_pixel;
    double decode_ms_sum = 0;
    std::vector<std::string> errors;

    while (decoded < target_frames) {
        auto read_result = kivo::cinema_engine::FfmpegAdapter::read_packet(handle);
        if (read_result.eof) {
            break;
        }
        if (!read_result.success) {
            errors.push_back(read_result.error_message);
            if (errors.size() > 5) {
                break;
            }
            continue;
        }
        ++packets;
        if (read_result.packet.stream_index != video_stream) {
            continue;
        }
        ++video_packets;
        if (read_result.packet.is_key_frame) {
            ++keyframes;
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
        const auto started = std::chrono::steady_clock::now();
        auto result = decoder.decode_video_rgba(packet, "p211_sd_probe");
        const double ms =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() -
                                                      started)
                .count();
        decode_ms_sum += ms;
        if (!result.success || result.needs_more_input || result.eof) {
            continue;
        }
        if (result.frame.width <= 0 || result.frame.height <= 0 ||
            result.frame.frame_data.empty()) {
            continue;
        }
        ++decoded;
        if (decoded == 1) {
            out_width = result.frame.width;
            out_height = result.frame.height;
            out_pixel = result.frame.pixel_format;
            first_frame_bytes = result.frame.frame_data.size();
        }
    }

    // 5. 证据（stdout；字段名与解码证据 family 一致风格）。
    std::printf("{\n");
    std::printf("  \"schema\": \"kivo.p211.sd-probe/1\",\n");
    std::printf("  \"ok\": %s,\n", decoded > 0 ? "true" : "false");
    std::printf("  \"media\": \"%s\",\n", escape_json(media).c_str());
    std::printf("  \"container\": \"%s\",\n", escape_json(container).c_str());
    std::printf("  \"duration_s\": %.3f,\n", duration);
    std::printf("  \"video_stream\": %d,\n", video_stream);
    std::printf("  \"codec\": \"%s\",\n", escape_json(codec).c_str());
    std::printf("  \"source_width\": %d,\n", width);
    std::printf("  \"source_height\": %d,\n", height);
    std::printf("  \"source_fps\": %.3f,\n", fps);
    std::printf("  \"source_pixel\": \"%s\",\n", escape_json(src_pixel).c_str());
    std::printf("  \"packets_read\": %d,\n", packets);
    std::printf("  \"video_packets\": %d,\n", video_packets);
    std::printf("  \"keyframes\": %d,\n", keyframes);
    std::printf("  \"frames_decoded\": %d,\n", decoded);
    std::printf("  \"first_frame\": {\n");
    std::printf("    \"width\": %d,\n", out_width);
    std::printf("    \"height\": %d,\n", out_height);
    std::printf("    \"pixel_format\": \"%s\",\n", escape_json(out_pixel).c_str());
    std::printf("    \"bytes\": %zu\n", first_frame_bytes);
    std::printf("  },\n");
    std::printf("  \"decode_ms_avg\": %.3f,\n",
                decoded > 0 ? decode_ms_sum / decoded : 0.0);
    std::printf("  \"errors\": [");
    for (std::size_t i = 0; i < errors.size(); ++i) {
        std::printf("%s\"%s\"", i > 0 ? ", " : "", escape_json(errors[i]).c_str());
    }
    std::printf("],\n");
    std::printf("  \"generated_at_unix_ms\": %lld\n",
                static_cast<long long>(std::time(nullptr)) * 1000);
    std::printf("}\n");

    kivo::cinema_engine::FfmpegAdapter::free_format_context(handle);
#if defined(_WIN32)
    _close(fd);
#else
    close(fd);
#endif
    return decoded > 0 ? 0 : 1;
}
