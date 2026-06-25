#include "kivo/cinema_engine/demux_core/real_probe_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

namespace kivo::cinema_engine {

ProbeResult RealProbeRuntime::probe_file(const std::string& path, const std::string& trace_id) {
    ProbeResult result;
    result.trace_id = trace_id;

    if (!is_ffmpeg_available()) {
        result.success = false;
        result.error_code = "ffmpeg_not_available";
        result.inspector_hint = "FFmpeg adapter disabled at build time";
        return result;
    }

    FfmpegFormatHandle handle = FfmpegAdapter::open_file(path);
    if (!handle.native) {
        result.success = false;
        result.error_code = "open_failed";
        result.inspector_hint = "Failed to open file: " + path;
        return result;
    }

    result = probe_format_context(handle.native, trace_id);
    FfmpegFormatHandle free_handle = handle;
    FfmpegAdapter::free_format_context(free_handle);
    return result;
}

ProbeResult RealProbeRuntime::probe_stream(LocalFileByteStream& byte_stream, const std::string& trace_id) {
    ProbeResult result;
    result.trace_id = trace_id;

    if (!is_ffmpeg_available()) {
        result.success = false;
        result.error_code = "ffmpeg_not_available";
        result.inspector_hint = "FFmpeg adapter disabled at build time";
        return result;
    }

    if (!byte_stream.is_open()) {
        result.success = false;
        result.error_code = "stream_not_open";
        result.inspector_hint = "Byte stream is not open";
        return result;
    }

    result.success = false;
    result.error_code = "stream_probe_not_supported";
    result.inspector_hint = "Stream-based probing not yet implemented; use probe_file instead";
    return result;
}

std::vector<std::string> RealProbeRuntime::supported_container_formats() {
    if (!is_ffmpeg_available()) return {};
    return {
        "mov", "mp4", "mkv", "avi", "flv", "wmv", "webm", "ogg",
        "ts", "m2ts", "mpg", "mpeg", "3gp", "m4a", "aac", "flac",
        "wav", "opus", "mp3", "ac3", "dts"
    };
}

std::vector<std::string> RealProbeRuntime::supported_video_codecs() {
    if (!is_ffmpeg_available()) return {};
    return {
        "h264", "hevc", "vp8", "vp9", "av1", "mpeg2video", "mpeg4",
        "theora", "mjpeg", "prores", "dnxhd", "cfhd"
    };
}

std::vector<std::string> RealProbeRuntime::supported_audio_codecs() {
    if (!is_ffmpeg_available()) return {};
    return {
        "aac", "opus", "flac", "vorbis", "mp3", "ac3", "eac3",
        "dca", "pcm_s16le", "pcm_s24le", "pcm_f32le", "alac"
    };
}

bool RealProbeRuntime::is_ffmpeg_available() {
    return FfmpegAdapter::is_available();
}

ProbeResult RealProbeRuntime::probe_format_context(void* format_context, const std::string& trace_id) {
    ProbeResult result;
    result.trace_id = trace_id;

    if (!format_context) {
        result.success = false;
        result.error_code = "invalid_context";
        result.inspector_hint = "Null format context passed to probe";
        return result;
    }

    FfmpegFormatHandle handle;
    handle.native = format_context;

    std::string container_format;
    double duration = 0.0;
    int64_t bitrate_estimate = 0;

    if (!FfmpegAdapter::probe_container(handle, container_format, duration, bitrate_estimate)) {
        result.success = false;
        result.error_code = "probe_failed";
        result.inspector_hint = "Failed to probe container metadata";
        return result;
    }

    result.success = true;
    result.container_format = container_format;
    result.duration = duration;
    result.bitrate_estimate = bitrate_estimate;

    std::vector<int> video_indices, audio_indices, subtitle_indices;
    FfmpegAdapter::enumerate_streams(handle, video_indices, audio_indices, subtitle_indices);

    auto extract_stream_info = [&](int stream_index) -> StreamInfo {
        StreamInfo info;
        info.index = stream_index;
        FfmpegAdapter::get_stream_codec_name(handle, stream_index, info.codec_name);

        int64_t duration_val = 0, bitrate_val = 0;
        std::string language;
        FfmpegAdapter::get_stream_metadata(handle, stream_index, language, duration_val, bitrate_val);
        info.duration = duration_val;
        info.bitrate = bitrate_val;
        info.language = language;

        int width = 0, height = 0;
        double fps = 0.0;
        std::string pixel_fmt;
        if (FfmpegAdapter::get_stream_video_info(handle, stream_index, width, height, fps, pixel_fmt)) {
            info.width = width;
            info.height = height;
            info.fps = fps;
            info.pixel_format = pixel_fmt;
        }

        int sample_rate = 0, channels = 0;
        std::string ch_layout, samp_fmt;
        if (FfmpegAdapter::get_stream_audio_info(handle, stream_index, sample_rate, channels, ch_layout, samp_fmt)) {
            info.sample_rate = sample_rate;
            info.channels = channels;
            info.channel_layout = ch_layout;
            info.sample_format = samp_fmt;
        }

        return info;
    };

    for (int idx : video_indices) {
        result.video_streams.push_back(extract_stream_info(idx));
    }
    for (int idx : audio_indices) {
        result.audio_streams.push_back(extract_stream_info(idx));
    }
    for (int idx : subtitle_indices) {
        result.subtitle_streams.push_back(extract_stream_info(idx));
    }

    return result;
}

}  // namespace kivo::cinema_engine
