#include "video/playback_graph/local_media/local_media_first_frame_pipeline.hpp"

#include "video/playback_graph/local_media/local_media_frame_rgba_converter.hpp"

#include <algorithm>
#include <filesystem>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef KIVO_ENABLE_FFMPEG
#include "kivo/cinema_engine/demux_core/real_demux_runtime.hpp"
#include "kivo/cinema_engine/demux_core/real_probe_runtime.hpp"
#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_device_context.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_texture_upload.hpp"
#endif

namespace kivo::video::playback_graph::local_media {
namespace {

[[nodiscard]] bool file_exists(const std::string& path) noexcept {
    if (path.empty()) {
        return false;
    }
#ifdef _WIN32
    const int wide_length =
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wide_length <= 0) {
        return false;
    }
    std::wstring wide_path(static_cast<std::size_t>(wide_length), L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wide_path.data(), wide_length) <= 0) {
        return false;
    }
    const DWORD attributes = GetFileAttributesW(wide_path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
    std::error_code error;
    return std::filesystem::exists(path, error) && std::filesystem::is_regular_file(path, error);
#endif
}

}  // namespace

bool LocalMediaFirstFramePipeline::open(const std::string& media_path) noexcept {
    close();
    status_ = {};
    status_.active = true;
    media_path_ = media_path;

    if (!file_exists(media_path_)) {
        status_.error = "media file not found";
        status_.active = false;
        return false;
    }

#ifndef KIVO_ENABLE_FFMPEG
    status_.error = "ffmpeg disabled";
    status_.active = false;
    return false;
#else
    using namespace kivo::cinema_engine;

    const auto probe = RealProbeRuntime::probe_file(media_path_, "p10_local_probe");
    if (!probe.success) {
        status_.error = "probe failed";
        status_.active = false;
        return false;
    }

    status_.source_opened = true;
    status_.container = probe.container_format;
    status_.duration_ms = static_cast<std::int64_t>(probe.duration * 1000.0);
    if (!probe.video_streams.empty()) {
        video_stream_index_ = probe.video_streams.front().index;
        status_.video_codec = probe.video_streams.front().codec_name;
        status_.width = probe.video_streams.front().width;
        status_.height = probe.video_streams.front().height;
    } else {
        status_.error = "no video stream";
        status_.active = false;
        return false;
    }

    return true;
#endif
}

bool LocalMediaFirstFramePipeline::decode_first_frame() noexcept {
#ifndef KIVO_ENABLE_FFMPEG
    status_.error = "ffmpeg disabled";
    return false;
#else
    using namespace kivo::cinema_engine;

    if (!status_.active || !status_.source_opened || video_stream_index_ < 0) {
        status_.error = "pipeline not open";
        return false;
    }

    FfmpegFormatHandle fmt_handle = FfmpegAdapter::open_file(media_path_);
    if (!fmt_handle.native) {
        status_.error = "format open failed";
        return false;
    }

    FfmpegCodecHandle codec_handle =
        FfmpegAdapter::create_codec_context(fmt_handle, video_stream_index_);
    if (!codec_handle.native) {
        FfmpegAdapter::free_format_context(fmt_handle);
        status_.error = "codec context failed";
        return false;
    }

    RealSoftwareDecodeRuntime decoder;
    if (!decoder.adopt_codec_context(codec_handle.native, status_.video_codec, "p10_decode")) {
        FfmpegAdapter::free_format_context(fmt_handle);
        status_.error = "codec adopt failed";
        return false;
    }

    RealDemuxRuntime demux;
    if (!demux.open(media_path_, "p10_demux")) {
        decoder.close();
        FfmpegAdapter::free_format_context(fmt_handle);
        status_.error = "demux open failed";
        return false;
    }

    status_.demux_started = true;
    status_.video_decode_started = true;

    bool decoded = false;
    for (int packet_index = 0; packet_index < 5000; ++packet_index) {
        const auto packet = demux.read_packet("p10_packet_" + std::to_string(packet_index));
        if (packet.eof) {
            break;
        }
        if (!packet.success || packet.packet.stream_index != video_stream_index_) {
            continue;
        }

        const DecodeResult result =
            decoder.decode(packet.packet, "p10_decode_" + std::to_string(packet_index));
        if (!result.success || result.needs_more_input || result.eof) {
            continue;
        }
        if (result.frame.width <= 0 || result.frame.height <= 0 || result.frame.frame_data.empty()) {
            continue;
        }

        status_.first_frame_decoded = true;
        status_.width = result.frame.width;
        status_.height = result.frame.height;

        const auto rgba = convert_decoded_frame_to_rgba8888(result.frame);
        if (!rgba.valid) {
            status_.error = rgba.error.empty() ? "rgba conversion failed" : rgba.error;
            continue;
        }
        frame_buffer_.rgba8888 = rgba.rgba8888;
        frame_buffer_.valid = true;
        frame_buffer_.width = rgba.width;
        frame_buffer_.height = rgba.height;

        D3d11DeviceContext device;
        if (device.initialize()) {
            D3d11TextureUpload uploader;
            if (uploader.initialize(&device)) {
                const D3D11TextureHandle texture =
                    uploader.create_texture(result.frame.width, result.frame.height, result.frame.pixel_format);
                if (texture && uploader.upload_frame(texture, result.frame, YuvRgbConversion{})) {
                    status_.d3d11_uploaded = true;
                    uploader.release_texture(texture);
                }
            }
        }

        status_.frame_rendered = frame_buffer_.valid;
        decoded = frame_buffer_.valid;
        break;
    }

    decoder.close();
    demux.close();
    FfmpegAdapter::free_format_context(fmt_handle);

    if (!decoded) {
        status_.error = "no video frame decoded";
        return false;
    }
    return true;
#endif
}

void LocalMediaFirstFramePipeline::close() noexcept {
    status_ = {};
    frame_buffer_ = {};
    media_path_.clear();
    video_stream_index_ = -1;
}

}  // namespace kivo::video::playback_graph::local_media