#pragma once

// FFmpeg Adapter — public boundary header.
// Contains ZERO FFmpeg forward declarations. All FFmpeg interactions are behind opaque handles.
// FFmpeg native types (AVFormatContext, AVCodecContext, AVPacket, AVFrame) are
// ONLY allowed inside backend/src/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.cpp.

#include <cstdint>
#include <string>
#include <vector>

namespace kivo::cinema_engine {

// ─── Opaque handle types ───────────────────────────────────────────
struct FfmpegFormatHandle { void* native = nullptr; };
struct FfmpegCodecHandle { void* native = nullptr; };

// ─── Packet metadata (extracted from AVPacket) ─────────────────────
struct FfmpegPacketInfo {
    int stream_index = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int duration = 0;
    bool is_key_frame = false;
};

// ─── Read result (includes compressed data copy) ───────────────────
struct FfmpegReadResult {
    bool success = false;
    bool eof = false;
    std::string error_message;
    FfmpegPacketInfo packet;
    std::vector<uint8_t> packet_data;
};

// ─── Frame result (extracted from AVFrame) ─────────────────────────
struct FfmpegFrameResult {
    bool is_video = false;
    bool is_audio = false;
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t duration = 0;
    int width = 0;
    int height = 0;
    std::string pixel_format;
    int sample_rate = 0;
    int channels = 0;
    std::string sample_format;
};

// ─── Codec info (extracted from AVCodecContext) ────────────────────
struct FfmpegCodecInfo {
    int width = 0;
    int height = 0;
    std::string pixel_format;
};

// ─── Send packet result ────────────────────────────────────────────
enum class FfmpegSendStatus { Success, NeedDrain, EndOfStream, Error };

struct FfmpegSendResult {
    FfmpegSendStatus status = FfmpegSendStatus::Error;
    std::string error_message;
};

class FfmpegAdapter {
public:
    // --- Availability ---
    static bool is_available();

    // --- Format context lifecycle ---
    static FfmpegFormatHandle open_file(const std::string& path);
    static void free_format_context(FfmpegFormatHandle& handle);

    // --- Format context queries ---
    static bool probe_container(FfmpegFormatHandle handle,
                                std::string& container_format,
                                double& duration,
                                int64_t& bitrate_estimate);
    static bool enumerate_streams(FfmpegFormatHandle handle,
                                  std::vector<int>& video_indices,
                                  std::vector<int>& audio_indices,
                                  std::vector<int>& subtitle_indices);

    // --- Stream info extraction ---
    static bool get_stream_codec_name(FfmpegFormatHandle handle, int stream_index,
                                      std::string& codec_name);
    static bool get_stream_video_info(FfmpegFormatHandle handle, int stream_index,
                                      int& width, int& height, double& fps,
                                      std::string& pixel_format);
    static bool get_stream_audio_info(FfmpegFormatHandle handle, int stream_index,
                                      int& sample_rate, int& channels,
                                      std::string& channel_layout,
                                      std::string& sample_format);
    static bool get_stream_metadata(FfmpegFormatHandle handle, int stream_index,
                                    std::string& language, int64_t& duration,
                                    int64_t& bitrate);

    // --- Packet reading (wraps av_read_frame, returns data copy) ---
    static FfmpegReadResult read_packet(FfmpegFormatHandle handle);

    // --- Seeking ---
    static bool seek(FfmpegFormatHandle handle, double seconds);

    // --- Codec lifecycle ---
    static FfmpegCodecHandle open_decoder_by_name(const std::string& codec_name);
    static FfmpegCodecHandle create_codec_context(FfmpegFormatHandle format_handle,
                                                  int stream_index);
    static void free_codec_context(FfmpegCodecHandle& handle);
    static FfmpegCodecInfo get_codec_info(FfmpegCodecHandle handle);

    // --- Decode operations ---
    static FfmpegSendResult send_packet(FfmpegCodecHandle codec_handle,
                                        const FfmpegPacketInfo& info,
                                        const uint8_t* data, int size);
    static bool receive_frame(FfmpegCodecHandle codec_handle,
                              FfmpegFrameResult& result);
    static void flush_decoder(FfmpegCodecHandle codec_handle);

    // --- Utility ---
    static std::string map_error(int ffmpeg_error);
    static std::string get_codec_name_from_id(int codec_id);
    static std::string get_pixel_format_name(int pixel_format);
    static std::string get_sample_format_name(int sample_format);
    static std::string get_channel_layout_description(uint64_t channel_layout);
};

}  // namespace kivo::cinema_engine
