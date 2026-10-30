#include "video/video_plane/runtime/local_media/local_video_first_frame_pipeline.hpp"

#include "video/video_plane/runtime/local_media/local_video_rgba_converter.hpp"
#include "video/source_runtime/local_media/local_media_source_session.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#ifdef KIVO_ENABLE_FFMPEG
#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/demux_core/kivo_packet.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_device_context.hpp"
#include "kivo/cinema_engine/video_upload/d3d11_texture_upload.hpp"
#endif

namespace kivo::video::video_plane::runtime::local_media {
namespace {

using kivo::video::source_runtime::local_media::LocalMediaSourceSession;
using kivo::video::video_plane::runtime::local_media::convert_decoded_frame_to_rgba8888;

void copy_stream_indices(const std::vector<int>& streams,
                         std::int32_t* const out,
                         std::size_t& out_count) noexcept {
    out_count = std::min<std::size_t>(streams.size(), LocalMediaPipelineStatus::kMaxStreamTracks);
    for (std::size_t index = 0; index < out_count; ++index) {
        out[index] = static_cast<std::int32_t>(streams[index]);
    }
}

#ifdef KIVO_ENABLE_FFMPEG
void copy_stream_codecs(
    kivo::cinema_engine::FfmpegFormatHandle format_handle,
    const std::int32_t* const stream_indices,
    const std::size_t stream_count,
    std::string* const out_codecs) noexcept {
    for (std::size_t index = 0; index < stream_count; ++index) {
        (void)kivo::cinema_engine::FfmpegAdapter::get_stream_codec_name(
            format_handle, stream_indices[index], out_codecs[index]);
    }
}
#endif

}  // namespace

bool LocalMediaFirstFramePipeline::open(const std::string& media_path) noexcept {
    close();
    status_ = {};
    status_.active = true;
    media_path_ = media_path;

#ifndef KIVO_ENABLE_FFMPEG
    status_.error = "ffmpeg disabled";
    status_.active = false;
    return false;
#else
    using namespace kivo::cinema_engine;

    LocalMediaSourceSession source;
    if (!source.open(media_path_)) {
        status_.error = source.error().empty() ? "source open failed" : source.error();
        status_.active = false;
        return false;
    }
    status_.source_session_connected = true;
    status_.source_provider_kind = source.provider_kind_name();

    FfmpegInputConfig input_config{};
    input_config.callbacks = source.ffmpeg_callbacks();
    FfmpegFormatHandle fmt_handle = FfmpegAdapter::open_custom_io(input_config);
    if (!fmt_handle.native) {
        status_.error = source.error().empty() ? "source-backed probe open failed"
                                               : source.error();
        status_.active = false;
        return false;
    }

    status_.source_opened = true;

    std::string container;
    double duration = 0.0;
    std::int64_t bitrate = 0;
    (void)FfmpegAdapter::probe_container(fmt_handle, container, duration, bitrate);
    status_.container = container;
    status_.duration_ms = static_cast<std::int64_t>(duration * 1000.0);

    std::vector<int> video_streams;
    std::vector<int> audio_streams;
    std::vector<int> subtitle_streams;
    (void)FfmpegAdapter::enumerate_streams(
        fmt_handle, video_streams, audio_streams, subtitle_streams);
    copy_stream_indices(video_streams, status_.video_stream_indices, status_.video_stream_count);
    copy_stream_indices(audio_streams, status_.audio_stream_indices, status_.audio_stream_count);
    copy_stream_indices(
        subtitle_streams, status_.subtitle_stream_indices, status_.subtitle_stream_count);
    copy_stream_codecs(fmt_handle,
                       status_.video_stream_indices,
                       status_.video_stream_count,
                       status_.video_stream_codecs);
    copy_stream_codecs(fmt_handle,
                       status_.audio_stream_indices,
                       status_.audio_stream_count,
                       status_.audio_stream_codecs);
    copy_stream_codecs(fmt_handle,
                       status_.subtitle_stream_indices,
                       status_.subtitle_stream_count,
                       status_.subtitle_stream_codecs);

    if (!video_streams.empty()) {
        video_stream_index_ = video_streams.front();
        (void)FfmpegAdapter::get_stream_codec_name(
            fmt_handle, video_stream_index_, status_.video_codec);
        double fps = 0.0;
        std::string pixel_format;
        (void)FfmpegAdapter::get_stream_video_info(
            fmt_handle, video_stream_index_, status_.width, status_.height, fps, pixel_format);
    } else {
        FfmpegAdapter::free_format_context(fmt_handle);
        status_.error = "no video stream";
        status_.active = false;
        return false;
    }
    if (!audio_streams.empty()) {
        audio_stream_index_ = audio_streams.front();
        (void)FfmpegAdapter::get_stream_codec_name(
            fmt_handle, audio_stream_index_, status_.audio_codec);
        std::string channel_layout;
        std::string sample_format;
        (void)FfmpegAdapter::get_stream_audio_info(
            fmt_handle,
            audio_stream_index_,
            status_.audio_sample_rate,
            status_.audio_channels,
            channel_layout,
            sample_format);
    }

    FfmpegAdapter::free_format_context(fmt_handle);
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

    LocalMediaSourceSession source;
    if (!source.open(media_path_)) {
        status_.error = source.error().empty() ? "source open failed" : source.error();
        return false;
    }
    status_.source_session_connected = true;
    status_.source_provider_kind = source.provider_kind_name();

    FfmpegInputConfig input_config{};
    input_config.callbacks = source.ffmpeg_callbacks();
    FfmpegFormatHandle fmt_handle = FfmpegAdapter::open_custom_io(input_config);
    if (!fmt_handle.native) {
        status_.error = source.error().empty() ? "source-backed format open failed"
                                               : source.error();
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
        FfmpegAdapter::free_codec_context(codec_handle);
        FfmpegAdapter::free_format_context(fmt_handle);
        status_.error = "codec adopt failed";
        return false;
    }

    status_.demux_started = true;
    status_.video_decode_started = true;

    bool decoded = false;
    for (int packet_index = 0; packet_index < 5000; ++packet_index) {
        const auto read_result = FfmpegAdapter::read_packet(fmt_handle);
        if (read_result.eof) {
            break;
        }
        if (!read_result.success || read_result.packet.stream_index != video_stream_index_) {
            continue;
        }

        KivoPacket packet{};
        packet.stream_index = read_result.packet.stream_index;
        packet.pts = read_result.packet.pts;
        packet.dts = read_result.packet.dts;
        packet.duration = read_result.packet.duration;
        packet.is_key_frame = read_result.packet.is_key_frame;
        packet.data = std::move(read_result.packet_data);
        for (auto& side : read_result.side_data) {
            packet.side_data.push_back({side.type, std::move(side.data)});
        }
        packet.packet_id = "pkt_" + std::to_string(packet.stream_index) + "_" +
                           std::to_string(packet.pts);

        const DecodeResult result =
            decoder.decode(packet, "p10_decode_" + std::to_string(packet_index));
        if (!result.success || result.needs_more_input || result.eof) {
            continue;
        }
        if (result.frame.width <= 0 || result.frame.height <= 0 || result.frame.frame_data.empty()) {
            continue;
        }

        status_.first_frame_decoded = true;
        status_.width = result.frame.width;
        status_.height = result.frame.height;

        std::vector<std::uint8_t> rgba_pixels;
        std::string rgba_error;
        if (!convert_decoded_frame_to_rgba8888_into(result.frame, rgba_pixels, rgba_error)) {
            status_.error = rgba_error.empty() ? "rgba conversion failed" : rgba_error;
            continue;
        }
        frame_buffer_.valid = true;
        frame_buffer_.width = result.frame.width;
        frame_buffer_.height = result.frame.height;
        frame_buffer_.rgba8888 =
            std::make_shared<const std::vector<std::uint8_t>>(std::move(rgba_pixels));

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
    audio_stream_index_ = -1;
}

}  // namespace kivo::video::video_plane::runtime::local_media
