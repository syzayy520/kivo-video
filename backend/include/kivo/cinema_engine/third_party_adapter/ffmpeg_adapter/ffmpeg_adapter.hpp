#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::cinema_engine {

// Forward declare to avoid FFmpeg headers in public interface
struct AVFormatContext;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;

class FfmpegAdapter {
public:
    // Open a file and return format context
    static AVFormatContext* open_file(const std::string& path);
    
    // Probe container information
    static bool probe_container(AVFormatContext* format_context, 
                               std::string& container_format,
                               double& duration,
                               int64_t& bitrate_estimate);
    
    // Enumerate streams
    static bool enumerate_streams(AVFormatContext* format_context,
                                 std::vector<int>& video_stream_indices,
                                 std::vector<int>& audio_stream_indices,
                                 std::vector<int>& subtitle_stream_indices);
    
    // Create codec context for a stream
    static AVCodecContext* create_codec_context(AVFormatContext* format_context, int stream_index);
    
    // Decode a packet into frame
    static bool decode_packet(AVCodecContext* codec_context,
                             AVPacket* packet,
                             AVFrame* frame);
    
    // Free resources
    static void free_format_context(AVFormatContext* format_context);
    static void free_codec_context(AVCodecContext* codec_context);
    static void free_packet(AVPacket* packet);
    static void free_frame(AVFrame* frame);
    
    // Map FFmpeg error to error code
    static std::string map_error(int ffmpeg_error);
    
    // Get codec name from AVCodecID
    static std::string get_codec_name(int codec_id);
    
    // Get pixel format name
    static std::string get_pixel_format_name(int pixel_format);
    
    // Get sample format name
    static std::string get_sample_format_name(int sample_format);
    
    // Get channel layout description
    static std::string get_channel_layout_description(uint64_t channel_layout);

    // Extract stream info fields from a format context and stream index
    // Returns false if stream_index is invalid
    static bool get_stream_codec_name(AVFormatContext* format_context, int stream_index, std::string& codec_name);
    static bool get_stream_video_info(AVFormatContext* format_context, int stream_index,
                                      int& width, int& height, double& fps, std::string& pixel_format);
    static bool get_stream_audio_info(AVFormatContext* format_context, int stream_index,
                                      int& sample_rate, int& channels, std::string& channel_layout, std::string& sample_format);
    static bool get_stream_metadata(AVFormatContext* format_context, int stream_index,
                                    std::string& language, int64_t& duration, int64_t& bitrate);
};

}  // namespace kivo::cinema_engine