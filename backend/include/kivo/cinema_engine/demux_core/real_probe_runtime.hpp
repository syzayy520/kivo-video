#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kivo/cinema_engine/byte_stream_core/local_file_byte_stream.hpp"

namespace kivo::cinema_engine {

struct StreamInfo {
    int index;
    std::string codec_name;
    std::string codec_long_name;
    int width;          // for video
    int height;         // for video
    double fps;         // for video
    int sample_rate;    // for audio
    int channels;       // for audio
    std::string channel_layout;
    std::string pixel_format;  // for video
    std::string sample_format; // for audio
    int64_t duration;
    int64_t bitrate;
    std::string language;
};

struct ProbeResult {
    bool success;
    std::string container_format;
    double duration;
    int64_t bitrate_estimate;
    std::vector<StreamInfo> video_streams;
    std::vector<StreamInfo> audio_streams;
    std::vector<StreamInfo> subtitle_streams;
    std::string error_code;
    std::string inspector_hint;
    std::string trace_id;
};

class RealProbeRuntime {
public:
    // Probe a local file directly by path
    static ProbeResult probe_file(const std::string& path, const std::string& trace_id);
    
    // Probe using LocalFileByteStream
    static ProbeResult probe_stream(LocalFileByteStream& byte_stream, const std::string& trace_id);
    
    // Get supported container formats
    static std::vector<std::string> supported_container_formats();
    
    // Get supported video codecs
    static std::vector<std::string> supported_video_codecs();
    
    // Get supported audio codecs
    static std::vector<std::string> supported_audio_codecs();
    
    // Check if FFmpeg adapter is available
    static bool is_ffmpeg_available();

private:
    // Probe an already-opened AVFormatContext (passed as void* to avoid FFmpeg header dependency)
    static ProbeResult probe_format_context(void* format_context, const std::string& trace_id);
};

}  // namespace kivo::cinema_engine